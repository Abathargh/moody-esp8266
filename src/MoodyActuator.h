#ifndef MOODY_ACTUATOR_H
#define MOODY_ACTUATOR_H

#include <stdint.h>
#include <stdlib.h>

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
    client.setCallback(
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
    public:
        ActuatorWebServer();
        void begin(){ server.begin(); };
};

ActuatorWebServer::ActuatorWebServer() : server(WEB_SERVER_PORT) {


}

#endif