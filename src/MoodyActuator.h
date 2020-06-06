#ifndef MOODY_ACTUATOR_H
#define MOODY_ACTUATOR_H

#include <stdint.h>
#include <stdlib.h>
#include <ArduinoJson.h>

#define ACTUATOR_MODE

#define ACTUATION_MODE 0 
#define WEB_SERVER_MODE 1

#define MAX_ACTION_SIZE 5
#define MAX_ACTION_NUM 10

#define SWITCH_MODE_TOPIC "moody/actuator/mode"
#define SITUATION_TOPIC "moody/actuator/situation"
#define PUBLISH_IP_TOPIC "moody/actserver"


struct mappings {
    // iterate over situations, find index of received situation
    // access action and actuate using it.
    uint8_t size; // how many mappings are currently present?
    uint8_t situations[MAX_ACTION_NUM];
    uint8_t actions[MAX_ACTION_NUM];
} mapping;

class Actuable {
    public:
        virtual void actuate(uint8_t) = 0;
};

uint8_t actuatorMode = ACTUATION_MODE;

class MoodyActuator : public MoodyNode {
    private:
        static Actuable *device;
        void lastSetup();
        void lastLoop() { return; };

    public:
        MoodyActuator(Actuable *device) {}
        void setActuable(Actuable *device);
        void loop();
};


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

                ActuatorWebServer server;
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


class ActuatorWebServer {
    private:
        AsyncWebServer server;
        static const uint8_t numPostArgs;
        static const char* postArgs[2]; 
        static bool validPostRequest(AsyncWebServerRequest*, uint8_t*);
        static bool validDeleteRequest(AsyncWebServerRequest*, uint8_t*);
        static char* encodePostResponse(uint8_t*);
    public:
        ActuatorWebServer() : server(WEB_SERVER_MODE) {}
        void begin(){ server.begin(); };
};

const uint8_t ActuatorWebServer::numPostArgs = 2;
const char* ActuatorWebServer::postArgs[2] = {"situation", "action"};

// Validates request searching for args arguments in the request, returning true 
// if the request is valid, false otherwise. The parameters are then copied in values.
// If false is returned, nothing can be said about the contents of values.
bool ActuatorWebServer::validPostRequest(AsyncWebServerRequest *request, uint8_t *values) {
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
bool ActuatorWebServer::validDeleteRequest(AsyncWebServerRequest *request, uint8_t *situationId) {
    long intValue;
    char* checkInvalidNum;
    
    String situationIdStr = request->pathArg(0);
    intValue = strtol(situationIdStr.c_str(), &checkInvalidNum, 10);
    
    if(*checkInvalidNum) { return false; }
    if(intValue < 0 || intValue > 255) { return false; }
    *situationId = intValue; 
    return true;
}


// Encodes the values received via HTTP POST into a json object as a response for the post requests.
char* ActuatorWebServer::encodePostResponse(uint8_t* values) {
    int i;
    char* resp;
    StaticJsonDocument<53> jsonResp;
    for(i = 0; i < numPostArgs; i++) {
        jsonResp[postArgs[i]] = values[i];
    }
    serializeJson(jsonResp, resp);
    return resp;
}



ActuatorWebServer::ActuatorWebServer() : server(WEB_SERVER_PORT) {
    server.on("/mapping", HTTP_POST, [](AsyncWebServerRequest *request) {
        uint8_t values[numPostArgs];
        if (mapping.size == MAX_ACTION_NUM || !validPostRequest(request, values)) {   
            request->send(422, "application/json", "{\"error\": \"wrong syntax\"}");
        }
        // Update in mem, flash only on server exit
        mapping.situations[mapping.size] = values[0];
        mapping.actions[mapping.size] = values[1];
        mapping.size++;

        char* resp = encodePostResponse(values);
        request->send(200, "application/json", resp);
    });

    server.on("/mapping", HTTP_GET, [](AsyncWebServerRequest *request) {
        int i;
        char* resp;
        StaticJsonDocument<680> respDoc;
        JsonArray jsonMaps = respDoc.createNestedArray("mappings");
        for(i = 0; i < mapping.size; i++) {
            JsonObject jsonMap = jsonMaps.createNestedObject();
            jsonMap["situation"] = mapping.situations[i];
            jsonMap["action"] = mapping.actions[i];
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
        }

        int i;
        for(i = 0; i < mapping.size; i++) { if(situationId == mapping.situations[i]) { break; } }
        if(i >= mapping.size) { request->send(404, "application/json", "{\"error\": \"not found\"}"); }

        for(;i < mapping.size - 1; i++) {
            mapping.situations[i] = mapping.situations[i+1];
            mapping.actions[i] = mapping.actions[i+1];
        }
        mapping.size--;
    });

    server.on("/end", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"action\": \"ending\"}");
        EEPROM.put(MAPPINGS_ADDR, mapping);
        EEPROM.commit();
        ESP.reset();
    });
}

#endif