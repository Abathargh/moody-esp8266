#ifndef MOODYESP8266_H_
#define MOODYESP8266_H_

#define ACTUATION_MODE 0
#define WEB_SERVER_MODE 1

#define MAX_ACTION_SIZE 5
#define MAX_ACTION_NUM 10

#define CONN_OK 1
#define CONN_NOT_OK 0

#define SSID_LENGTH 32
#define KEY_LENGTH 64
#define MAX_ATTEMPTS 5

#define AP_SSID "MoodyNode" 
#define WEB_SERVER_PORT 80

#define CONNINFO_ADDR 0
#define MAPPINGS_ADDR 104
#define EEPROM_SIZE_SENSOR 104
#define EEPROM_SIZE_ACTUATOR 300 // 272 should be enough 

#define MSG_BUFFER_SIZE	(50)

#define MAX_SERVICES 3
#define BASE_TOPIC "moody/service/"
#define BASE_SIZE 14
#define MAX_TOPIC_SIZE 30


#include <stdint.h>
#include <stdlib.h>

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Stores and retrieves connection information in EEPROM
// (emulated via flash memory)
struct connection_info {
    uint8_t ok;
    char SSID[SSID_LENGTH];
    char KEY[KEY_LENGTH];
};

struct mappings {
    // iterate over situations, find index of received situation
    // access action and actuate using it.
    uint8_t size; // how many mappings are currently present?
    uint8_t situations[MAX_ACTION_NUM];
    uint8_t actions[MAX_ACTION_NUM];
};

bool validPostConnect(AsyncWebServerRequest*, const char**);
AsyncWebServer createAPServer(int port);

// Base class for nodes, containing connection/AP mechanisms 
class MoodyNode {
    private:
        static AsyncWebServer apServer;
        bool apMode;
        void activateAPMode();
        void connectToWifi();

    protected:
        static WiFiClient wifiClient;
        static PubSubClient client; 
        char msg[MSG_BUFFER_SIZE];

        
        MoodyNode() : apMode(false) {};
        void connectToBroker();
        virtual void lastSetup() = 0; // implemented by heirs to add setup steps
        virtual void lastLoop() = 0; // implemented by heirs to add actions in the main loop

    public:
        static connection_info conninfo;
        virtual void begin(int baudRate);
        virtual void loop();
};


using callback = String (*)();
class MoodySensor : public MoodyNode {
    private:
        uint8_t activeServices;
        char serviceTopics[MAX_SERVICES][MAX_TOPIC_SIZE];
        callback callbacks[MAX_SERVICES];
        void lastSetup();
        void lastLoop();

    public:
        MoodySensor() : MoodyNode(){};
        void registerService(const char* topic, callback callback);
};

class ActuatorWebServer {
    private:
        AsyncWebServer server;
    public:
        ActuatorWebServer();
        void begin(){ server.begin(); };
};

class MoodyActuator : public MoodyNode {
    private:
        static uint8_t actuatorMode; 
        static ActuatorWebServer server;
        void lastSetup();
        void lastLoop() { return; };
        static void (*actuate)(uint8_t);    
        
    public:
        static bool mappingChangedFlag;
        static mappings mapping;
        MoodyActuator() {}
        void loop();
        static void setActuate(void (*actuate)(uint8_t));
        static void addMapping();
        static void removeMapping();
};


bool validPostRequest(AsyncWebServerRequest*, uint8_t*);
char* encodePostResponse(uint8_t*);

#endif