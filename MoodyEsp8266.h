#ifndef MOODYESP8266_H_
#define MOODYESP8266_H_

#define BAUD_RATE 115200

#define ACTUATION_MODE 0
#define WEB_SERVER_MODE 1


#define CONN_OK 1
#define CONN_NOT_OK 0

// conninfo defines
#define OK_LENGTH 1
#define SSID_LENGTH 32
#define KEY_LENGTH 64
#define BROKER_ADDR_LENGTH 16

#ifndef HTTP_ONLY
#define FINGERPRINT_LENGTH 60
#define CERT_LENGTH 1300
#else
#define FINGERPRINT_LENGTH 0
#define CERT_LENGTH 0
#endif

#define CONN_INFO_LENGTH (OK_LENGTH+SSID_LENGTH+KEY_LENGTH+BROKER_ADDR_LENGTH+FINGERPRINT_LENGTH+CERT_LENGTH)

// mappings defines
#define MAPPING_SIZE 1
#define MAX_ACTION_SIZE 5
#define MAX_ACTION_NUM 10
#define MAPPING_LENGTH (MAPPING_SIZE+MAX_ACTION_SIZE+MAX_ACTION_NUM)

#define MAX_ATTEMPTS 5

#define AP_SSID "MoodyNode"
#define WEB_SERVER_PORT 80
#define MQTT_PORT 8883

// EEPROM defines
#define CONNINFO_ADDR 0
#define MAPPINGS_ADDR CONN_INFO_LENGTH
#define EEPROM_SIZE_SENSOR CONN_INFO_LENGTH
#define EEPROM_SIZE_ACTUATOR (CONN_INFO_LENGTH+MAPPING_LENGTH)

#define MSG_BUFFER_SIZE (50)

#define MAX_SERVICES 3
#define BASE_TOPIC "moody/service/"
#define BASE_SIZE 14
#define MAX_TOPIC_SIZE 30
#define DEFAULT_PERIOD 2000

#ifdef DEBUG_ESP_PORT
#define DEBUG_MSG(...) DEBUG_ESP_PORT.printf(__VA_ARGS__)
#else
#define DEBUG_MSG(...)
#endif

#include <EEPROM.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#else
#include <WiFiClientSecure.h>
#include <AsyncTCP.h>
#endif

#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <stdint.h>
#include <stdlib.h>


// Stores and retrieves connection information in EEPROM
// (emulated via flash memory)
struct connection_info
{
    uint8_t ok;
    char SSID[SSID_LENGTH];
    char KEY[KEY_LENGTH];
    char BROKER_ADDR[BROKER_ADDR_LENGTH];
#ifndef HTTP_ONLY
    char FINGERPRINT[FINGERPRINT_LENGTH];
    char CERT[CERT_LENGTH];
#endif
};

struct mappings
{
    // iterate over situations, find index of received situation
    // access action and actuate using it.
    uint8_t size; // how many mappings are currently present?
    uint8_t situations[MAX_ACTION_NUM];
    uint8_t actions[MAX_ACTION_NUM];
};

bool validPostConnect(AsyncWebServerRequest *, const char **);
AsyncWebServer createAPServer(int port);

// Base class for nodes, containing connection/AP mechanisms
class MoodyNode
{
private:
    static AsyncWebServer apServer;
#if defined(ESP8266)
#ifndef HTTP_ONLY
    X509List caCertX509;
#endif
#endif
protected:
    bool apMode;
#ifndef HTTP_ONLY
    static WiFiClientSecure wifiClient;
#else
    static WiFiClient wifiClient;
#endif
    static PubSubClient client;
    char msg[MSG_BUFFER_SIZE];

    MoodyNode() : apMode(false){};
    void activateAPMode();
    bool connectToWifi();
    bool connectToBroker();
    virtual void lastSetup() = 0; // implemented by heirs to add setup steps
    virtual void
    lastLoop() = 0; // implemented by heirs to add actions in the main loop

public:
    static connection_info conninfo;
    virtual void begin();
    virtual void loop();
};

using callback = String (*)();
class MoodySensor : public MoodyNode
{
private:
    uint8_t activeServices;
    char serviceTopics[MAX_SERVICES][MAX_TOPIC_SIZE];
    callback callbacks[MAX_SERVICES];
    uint32_t loopPeriod;
    void lastSetup();
    void lastLoop();

public:
    MoodySensor() : MoodyNode(), loopPeriod{DEFAULT_PERIOD} {};
    void registerService(const char *topic, callback callback);
    void setLoopPeriod(uint32_t period_ms);
};

class ActuatorWebServer
{
private:
    AsyncWebServer server;

public:
    ActuatorWebServer();
    void begin() { server.begin(); };
};

class MoodyActuator : public MoodyNode
{
private:
    static uint8_t actuatorMode;
    static ActuatorWebServer server;
    void lastSetup();
    void lastLoop() { return; };
    static void (*actuate)(uint8_t);

public:
    static bool mappingChangedFlag;
    static mappings mapping;
    MoodyActuator() : MoodyNode() {}
    void loop();
    static void setActuate(void (*actuate)(uint8_t));
    static void addMapping();
    static void removeMapping();
};

bool validPostRequest(AsyncWebServerRequest *, uint8_t *);
char *encodePostResponse(uint8_t *);

#endif