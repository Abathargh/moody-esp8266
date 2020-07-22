#include "MoodyEsp8266.h"
#include <ArduinoJson.h>

#define SWITCH_MODE_TOPIC "moody/actuator/mode"
#define SITUATION_TOPIC "moody/actuator/situation"
#define PUBLISH_IP_TOPIC "moody/actserver"

#define NO_MAPPING_CHANGE 0

const uint8_t numPostArgs = 2;
const char* postArgs[] {"situation", "action"};


// Validates request searching for args arguments in the request, returning true 
// if the request is valid, false otherwise. The parameters are then copied in values.
// If false is returned, nothing can be said about the contents of values.
bool validPostRequest(AsyncWebServerRequest *request, uint8_t *values) {
    int i;
    long intValue;
    String value;
    char* checkInvalidNum;
    
    for(i = 0; i < numPostArgs; i++) {
        if(!request->hasParam(postArgs[i], true)) { return false; }
        
        value = request->getParam(postArgs[i], true)->value();
        intValue = strtol(value.c_str(), &checkInvalidNum, 10);
        
        if(*checkInvalidNum) { return false; }
        if(intValue < 0 || intValue > 255) { return false; }

        values[i] = intValue;  
    }
    return true;
}


mappings MoodyActuator::mapping;
bool MoodyActuator::mappingChangedFlag = NO_MAPPING_CHANGE;
ActuatorWebServer MoodyActuator::server = ActuatorWebServer{};
uint8_t MoodyActuator::actuatorMode = ACTUATION_MODE;
void (*MoodyActuator::actuate)(uint8_t) = 0;

void MoodyActuator::setActuate(void (*actuate)(uint8_t)) {
    MoodyActuator::actuate = actuate;
}

void MoodyActuator::lastSetup() {
    //load mapping from eeprom
    EEPROM.get(MAPPINGS_ADDR, mapping);
    Serial.println("Loading mappings:");
    for(int i = 0; i < mapping.size; i++) {
        Serial.printf("%d: %d\n", mapping.situations[i], mapping.actions[i]);
    }

    client.setCallback(
        [](char* topic, uint8_t* payload, unsigned int length){
            if(strcmp(SITUATION_TOPIC, topic) == 0) {
                char strPayload[length+1];
                memcpy(strPayload, payload, length);
                strPayload[length] = '\0';

                char* checkInvalidSituation;
                long intValue = strtol((const char*)strPayload, &checkInvalidSituation, 10);
                if(*checkInvalidSituation) { Serial.print("Invalid situation: "); Serial.println(strPayload); return; }
                if(intValue < 0 || intValue > 255) { Serial.print("Out of bounds situation "); Serial.println(strPayload); return; }
                uint8_t situation = intValue;

                Serial.print("Received situation ");
                Serial.println(situation);

                uint8_t i;
                for(i = 0; i < mapping.size; i++) {
                    if(MoodyActuator::mapping.situations[i] == situation){ break; }
                }
                if(i < MoodyActuator::mapping.size) {
                    Serial.printf("Preparing to actuate action #%d based on situation #%d\n",
                        MoodyActuator::mapping.actions[i], MoodyActuator::mapping.situations[i]);
                    actuate(MoodyActuator::mapping.actions[i]);
                }
            } else if(strcmp(SWITCH_MODE_TOPIC, topic) == 0) {
                if((char)payload[0] != '1') { Serial.println("Mode change: wrong syntax"); return; }
                
                char msg[MSG_BUFFER_SIZE];
                String ip = WiFi.localIP().toString();
                Serial.println("Act Server ip: " + ip);

                snprintf(msg, MSG_BUFFER_SIZE, "%s", ip.c_str());
                client.publish(PUBLISH_IP_TOPIC, msg);
                delay(200);
                client.disconnect();

                server.begin();
                actuatorMode = WEB_SERVER_MODE;
            } else {
                // DO NOTHING
            }
        }
    );

    client.subscribe(SWITCH_MODE_TOPIC);
    client.subscribe(SITUATION_TOPIC);
}

void MoodyActuator::loop() {
    if(actuatorMode == ACTUATION_MODE){
        if(!client.connected()) {
            connectToBroker();
            lastSetup();
        }
        client.loop();
    }
}


// ActuatorWebServer - Implementation of the server mode of the actuator, where
// mappings can be changed/removed.
ActuatorWebServer::ActuatorWebServer() : server(WEB_SERVER_PORT) {
    // /mapping [POST]
    // required -- (situation: uint8_t, action: uint8_t) 
    server.on("/mapping", HTTP_POST, [](AsyncWebServerRequest *request) {
        uint8_t values[numPostArgs];
        if (MoodyActuator::mapping.size == MAX_ACTION_NUM || !validPostRequest(request, values)) {   
            request->send(422, "application/json", "{\"error\": \"wrong syntax\"}");
            return;
        }
        // Check that the mapping with this situationId doesn't already exist
        for(int i = 0; i < MoodyActuator::mapping.size; i++) {
            if(MoodyActuator::mapping.situations[i] == values[0]) {
                request->send(409, "application/json", "{\"error\": \"resource already exists\"}");
                return;
            }
        }
        // Update in mem, flash only on server exit
        MoodyActuator::mapping.situations[MoodyActuator::mapping.size] = values[0];
        MoodyActuator::mapping.actions[MoodyActuator::mapping.size] = values[1];
        MoodyActuator::mapping.size++;
        if(MoodyActuator::mappingChangedFlag == NO_MAPPING_CHANGE) { MoodyActuator::mappingChangedFlag = 1; }

        String resp;
        StaticJsonDocument<53> jsonResp;
        for(int i = 0; i < numPostArgs; i++) {
            jsonResp[postArgs[i]] = values[i];
        }
        serializeJson(jsonResp, resp);
        request->send(200, "application/json", resp);
    });

    server.on("/mapping", HTTP_GET, [](AsyncWebServerRequest *request) {
        int i;
        String resp;
        StaticJsonDocument<680> respDoc;
        JsonArray jsonMaps = respDoc.createNestedArray("mappings");
        for(i = 0; i < MoodyActuator::mapping.size; i++) {
            JsonObject jsonMap = jsonMaps.createNestedObject();
            jsonMap["situation"] = MoodyActuator::mapping.situations[i];
            jsonMap["action"] = MoodyActuator::mapping.actions[i];
        }
        serializeJson(respDoc, resp);
        request->send(200, "application/json", resp);
    });

    server.on("/mapping", HTTP_DELETE, [](AsyncWebServerRequest *request) {
        MoodyActuator::mapping.size = 0;
        if(MoodyActuator::mappingChangedFlag == NO_MAPPING_CHANGE) { MoodyActuator::mappingChangedFlag = 1; }
    });

    server.on("/end", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"action\": \"ending\"}");
        delay(200);
        if(MoodyActuator::mappingChangedFlag != NO_MAPPING_CHANGE) {
            EEPROM.put(MAPPINGS_ADDR, MoodyActuator::mapping);
            EEPROM.commit();
            ESP.reset();
        }
    });
}