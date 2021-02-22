#ifndef MOODYESP8266_H_
#define MOODYESP8266_H_


#define BAUD_RATE 115200

#define ACTUATION_MODE 0
#define WEB_SERVER_MODE 1

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

#include <stdint.h>
#include <stdlib.h>

#include "WifiWrapper/WifiWrapper.hpp"

using callback = String (*)();

class ActuatorWebServer {
private:
    AsyncWebServer server;

public:
    ActuatorWebServer();
    void begin() { server.begin(); };
};


// Base class for nodes, containing connection/AP mechanisms
class MoodyNode {
protected:
#if !defined(HTTP_ONLY)
    WifiWrapperTLS wifiWrapper;
#else
    WifiWrapperNoTLS wifiWrapper;
#endif
    MoodyNode() : wifiWrapper() {};
    virtual void lastSetup() = 0; // implemented by heirs to add setup steps
    virtual void
    lastLoop() = 0; // implemented by heirs to add actions in the main loop

public:
    virtual void begin();
    virtual void loop();
};


class MoodySensor : public MoodyNode {
private:
    uint8_t activeServices;
    char serviceTopics[MAX_SERVICES][MAX_TOPIC_SIZE];
    callback callbacks[MAX_SERVICES];
    uint32_t loopPeriod;
    void lastSetup();
    void lastLoop();

public:
    MoodySensor();
    void registerService(const char *topic, callback callback);
    void setLoopPeriod(uint32_t period_ms);
};


class MoodyActuator : public MoodyNode {
private:
    static WifiWrapper &wrapperRef;
    static uint8_t actuatorMode;
    static ActuatorWebServer server;
    void lastSetup();
    void lastLoop() { return; };
    static void (*actuate)(uint8_t);

public:
    static bool mappingChangedFlag;
    MoodyActuator();
    void loop();
    static void setActuate(void (*actuate)(uint8_t));
    static void addMapping();
    static void removeMapping();
};

bool validPostRequest(AsyncWebServerRequest *, uint8_t *);
char *encodePostResponse(uint8_t *);

#endif