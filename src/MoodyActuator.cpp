#include "MoodyEsp8266.h"
#include <ArduinoJson.h>

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

// Validates delete requests, returning true if it is well posed and putting the correctly formatted 
// situation id into situationId. The contents of this variable are well defined only if this returns true.
// A false as return value implies a HTTP 422 error code.
bool validDeleteRequest(AsyncWebServerRequest *request, uint8_t *situationId) {
    long intValue;
    char* checkInvalidNum;
    
    String situationIdStr = request->pathArg(0);
    intValue = strtol(situationIdStr.c_str(), &checkInvalidNum, 10);
    
    if(*checkInvalidNum) { return false; }
    if(intValue < 0 || intValue > 255) { return false; }
    *situationId = intValue; 
    return true;
}


mappings MoodyActuator::mapping;
ActuatorWebServer MoodyActuator::server = ActuatorWebServer{};
uint8_t MoodyActuator::actuatorMode = ACTUATION_MODE;

void MoodyActuator::setActuable(Actuable *device) {
    MoodyActuator::device = device;
}

void MoodyActuator::lastSetup() {
    MoodyNode::client.setCallback(
        [](char* topic, uint8_t* payload, unsigned int length){
            if(strcmp(SITUATION_TOPIC, topic) == 0) {
                // TODO atoi returns an int, this can only handle a byte
                // and will overflow
                uint8_t situation = atoi((const char*)payload);
                uint8_t i;
                for(i = 0; i < mapping.size; i++) {
                    if(mapping.situations[i] == situation)
                        break;
                }
                if(i != mapping.size) 
                    MoodyActuator::device->actuate(mapping.actions[i]);
            } else if(strcmp(SWITCH_MODE_TOPIC, topic) == 0) {
                const char* ip = WiFi.localIP().toString().c_str();
                client.publish(PUBLISH_IP_TOPIC, ip);
                client.disconnect();

                server.begin();
                actuatorMode = WEB_SERVER_MODE;
            } else {
                // DO NOTHING
            }
        }
    );
}

void MoodyActuator::loop() {
    if(actuatorMode == ACTUATION_MODE){
        if(!client.connected()) {
            connectToBroker();
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
        // Update in mem, flash only on server exit
        MoodyActuator::mapping.situations[MoodyActuator::mapping.size] = values[0];
        MoodyActuator::mapping.actions[MoodyActuator::mapping.size] = values[1];
        MoodyActuator::mapping.size++;

        int i;
        String resp;
        StaticJsonDocument<53> jsonResp;
        for(i = 0; i < numPostArgs; i++) {
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

    server.on("^\\/mapping\\/([0-9]{1,3})$", HTTP_DELETE, [](AsyncWebServerRequest *request) {
        // check that id is long and in uint8_t bounds like in post before
        // then find element in list (if present, otheerwise send error not found 404)
        // then delete it and the corresponding action and shift every element to the left
        // to cover the hole (don't if it was the only element)
        uint8_t situationId;
        if(!validDeleteRequest(request, &situationId)) {
            request->send(422, "application/json", "{\"error\": \"wrong syntax\"}");
            return;
        }

        int i;
        for(i = 0; i < MoodyActuator::mapping.size; i++) { if(situationId == MoodyActuator::mapping.situations[i]) { break; } }
        if(i >= MoodyActuator::mapping.size) { request->send(404, "application/json", "{\"error\": \"not found\"}"); }

        for(;i < MoodyActuator::mapping.size - 1; i++) {
            MoodyActuator::mapping.situations[i] = MoodyActuator::mapping.situations[i+1];
            MoodyActuator::mapping.actions[i] = MoodyActuator::mapping.actions[i+1];
        }
        MoodyActuator::mapping.size--;
    });

    server.on("/end", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"action\": \"ending\"}");
        EEPROM.put(MAPPINGS_ADDR, MoodyActuator::mapping);
        EEPROM.commit();
        ESP.reset();
    });
}