#include "MoodyEsp8266.h"
#include "EepromManager/EepromManager.hpp"
#include <ArduinoJson.h>

#define SWITCH_MODE_TOPIC "moody/actuator/mode"
#define SITUATION_TOPIC "moody/actuator/situation"
#define PUBLISH_IP_TOPIC "moody/actserver"
#define NO_MAPPING_CHANGE 0

// Comment to disable CORS
#define CORS

const uint8_t numPostArgs = 2;
const char *postArgs[]{"situation", "action"};


// Validates request searching for args arguments in the request, returning true
// if the request is valid, false otherwise. The parameters are then copied in values.
// If false is returned, nothing can be said about the contents of values.
bool validPostRequest(AsyncWebServerRequest *request, uint8_t *values) {
    int i;
    long intValue;
    String value;
    char *checkInvalidNum;

    for (i = 0; i < numPostArgs; i++) {
        if (!request->hasParam(postArgs[i], true)) {
            return false;
        }

        value = request->getParam(postArgs[i], true)->value();
        intValue = strtol(value.c_str(), &checkInvalidNum, 10);

        if (*checkInvalidNum) {
            return false;
        }

        if (intValue < 0 || intValue > 255) {
            return false;
        }

        values[i] = intValue;
    }
    return true;
}

bool MoodyActuator::mappingChangedFlag = NO_MAPPING_CHANGE;
ActuatorWebServer MoodyActuator::server = ActuatorWebServer{};
uint8_t MoodyActuator::actuatorMode = ACTUATION_MODE;
void (*MoodyActuator::actuate)(uint8_t) = 0;

MoodyActuator::MoodyActuator() : MoodyNode() {
    EepromManager::start(EepromManager::Mode::Actuator);
    wrapperRef = wifiWrapper;
}

void MoodyActuator::setActuate(void (*actuate)(uint8_t)) {
    MoodyActuator::actuate = actuate;
}

void MoodyActuator::lastSetup() {
    //load mapping from eeprom
    Mappings mapping = EepromManager::readMappings();
    DEBUG_MSG("Mappings size: %d:", mapping.size);

    wifiWrapper.setCallback(
        [](char *topic, uint8_t *payload, unsigned int length) {
            Mappings mapping = EepromManager::readMappings();

            DEBUG_MSG("%s - %s - %d\n", topic, payload, length);
            if (strcmp(SITUATION_TOPIC, topic) == 0)
            {
                char strPayload[length + 1];
                memcpy(strPayload, payload, length);
                strPayload[length] = '\0';
                DEBUG_MSG("Payload: %s\n", strPayload);

                char *checkInvalidSituation;
                long intValue = strtol(strPayload, &checkInvalidSituation, 10);
                if (*checkInvalidSituation != '\0') {
                    DEBUG_MSG("Invalid situation: %s\n", strPayload);
                    return;
                }
                if (intValue < 0 || intValue > 255) {
                    DEBUG_MSG("Out of bounds situation: %s\n", strPayload);
                    return;
                }

                uint8_t situation = intValue;
                DEBUG_MSG("Received situation %d\n", situation);
                
                uint8_t i;
                for (i = 0; i < mapping.size; i++) {
                    if (mapping.situations[i] == situation){
                        break;
                    }
                }

                if (i < mapping.size) {
                    DEBUG_MSG("Preparing to actuate action #%d based on situation #%d\n",
                              mapping.actions[i], mapping.situations[i]);
                    actuate(mapping.actions[i]);
                } else {
                    DEBUG_MSG("A situation was received that has no mapping: %d", situation);
                }
            } else if (strcmp(SWITCH_MODE_TOPIC, topic) == 0) {
                if ((char)payload[0] != '1') {
                    DEBUG_MSG("Mode change: wrong syntax\n");
                    return;
                }

                char msg[MSG_BUFFER_SIZE];
                const char* ip = WiFi.localIP().toString().c_str();
                DEBUG_MSG("Act Server ip: %s\n", ip);

                snprintf(msg, MSG_BUFFER_SIZE, "%s", ip);
                wrapperRef.publish(PUBLISH_IP_TOPIC, msg);

                // Let's wait a bit before disconnecting, so that the message is published
                delay(200);
                wrapperRef.mqttDisconnect();

                actuatorMode = WEB_SERVER_MODE;
                server.begin();
            } else {
                // DO NOTHING explicitly, which is better than implicitly
            }
        });

    wifiWrapper.subscribe(SWITCH_MODE_TOPIC);
    wifiWrapper.subscribe(SITUATION_TOPIC);
}

void MoodyActuator::loop() {
    if (!wifiWrapper.getApMode() && actuatorMode == ACTUATION_MODE) {
        bool connOk = wifiWrapper.checkConn();
        if(!connOk) {
            return;
        }
        wifiWrapper.loop();
    }
}

// ActuatorWebServer - Implementation of the server mode of the actuator, where
// mappings can be changed/removed.
ActuatorWebServer::ActuatorWebServer() : server(WEB_SERVER_PORT)
{
// Allows CORS
#ifdef CORS
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "content-type");
#endif

    // /mapping [POST]
    // required -- (situation: uint8_t, action: uint8_t)
    server.on("/mapping", HTTP_POST, [](AsyncWebServerRequest *request) {
        Mappings mapping = EepromManager::readMappings();

        uint8_t values[numPostArgs];
        if (!validPostRequest(request, values)) {
            request->send(422, "application/json", "{\"error\": \"wrong syntax\"}");
            return;
        }

        // Stops another mapping add when we reach the max number of mappings
        if (mapping.size == MAX_ACTION_NUM) {
            request->send(422, "application/json", "{\"error\": \"can't have more mappings\"}");
            return;
        }

        // Check that the mapping with this situationId doesn't already exist
        for (int i = 0; i < mapping.size; i++) {
            if (mapping.situations[i] == values[0]) {
                request->send(409, "application/json", "{\"error\": \"resource already exists\"}");
                return;
            }
        }
        // Update in mem, flash only on server exit
        mapping.situations[mapping.size] = values[0];
        mapping.actions[mapping.size] = values[1];
        mapping.size++;
        EepromManager::saveMappings(&mapping);
        if (MoodyActuator::mappingChangedFlag == NO_MAPPING_CHANGE) {
            MoodyActuator::mappingChangedFlag = 1;
        }

        String resp;
        StaticJsonDocument<53> jsonResp;
        for (int i = 0; i < numPostArgs; i++) {
            jsonResp[postArgs[i]] = values[i];
        }
        serializeJson(jsonResp, resp);
        request->send(200, "application/json", resp);
    });

    server.on("/mapping", HTTP_GET, [](AsyncWebServerRequest *request) {
        int i;
        String resp;
        StaticJsonDocument<680> respDoc;
        Mappings mapping = EepromManager::readMappings();
        JsonArray jsonMaps = respDoc.createNestedArray("mappings");
        for (i = 0; i < mapping.size; i++) {
            JsonObject jsonMap = jsonMaps.createNestedObject();
            jsonMap["situation"] = mapping.situations[i];
            jsonMap["action"] = mapping.actions[i];
        }
        serializeJson(respDoc, resp);
        request->send(200, "application/json", resp);
    });

    server.on("/mapping", HTTP_DELETE, [](AsyncWebServerRequest *request) {
        Mappings mapping = EepromManager::readMappings();
        mapping.size = 0;
        if (MoodyActuator::mappingChangedFlag == NO_MAPPING_CHANGE) {
            MoodyActuator::mappingChangedFlag = 1;
        }
        EepromManager::saveMappings(&mapping);
        request->send(204, "application/json", "");
    });

    server.on("/end", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"action\": \"ending\"}");
        delay(200);
        if (MoodyActuator::mappingChangedFlag != NO_MAPPING_CHANGE) {
            EepromManager::flashMappings();
        }
#if defined(ESP8266)
        ESP.reset();
#else
        ESP.restart();
#endif
    });
}