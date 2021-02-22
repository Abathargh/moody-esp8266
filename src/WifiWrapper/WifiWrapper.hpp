#ifndef WIFIWRAPPER_H_
#define WIFIWRAPPER_H_

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#else
#include <WiFiClientSecure.h>
#include <AsyncTCP.h>
#endif

#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>


#ifndef DEBUG_MSG
#ifdef DEBUG_ESP_PORT
#define DEBUG_MSG(...) DEBUG_ESP_PORT.printf(__VA_ARGS__)
#else
#define DEBUG_MSG(...)
#endif
#endif

// WiFi/MQTT local configurations
#define AP_SSID "MoodyNode"
#define WEB_SERVER_PORT 80
#define MQTT_PORT 8883
#define MAX_ATTEMPTS 5
#define CONN_OK 1
#define CONN_NOT_OK 0


#include "../EepromManager/EepromManager.hpp"


class WifiWrapper {
    protected:
        bool apMode;
        PubSubClient client;
        AsyncWebServer apServer;
        WifiWrapper();
        bool connectToWifi();
        bool connectToBroker();
        void activateAPMode();
    public:
        // wrapper methods for the mqtt client
        void loop();
        bool publish(const char*, const char*);
        bool subscribe(const char*);
        void setCallback(MQTT_CALLBACK_SIGNATURE);
        void mqttDisconnect();

        bool getApMode();
        bool checkConn();
        virtual bool begin() = 0;
        virtual void createAPServer(AsyncWebServer&) = 0;
        virtual WifiWrapper& operator=(const WifiWrapper&) = 0;
};

class WifiWrapperTLS : public WifiWrapper {
    private:
#if defined(ESP8266)
        X509List caCertX509;
#endif
        WiFiClientSecure wifiClient;
        static bool validPostConnect(AsyncWebServerRequest *request);
    public:
        WifiWrapperTLS();
        bool begin() override;
        void createAPServer(AsyncWebServer&) override;
        WifiWrapper& operator=(const WifiWrapper&) override { return *this; };

};

class WifiWrapperNoTLS : public WifiWrapper {
    private:
        WiFiClient wifiClient;
        static bool validPostConnect(AsyncWebServerRequest *request);
    public:
        WifiWrapperNoTLS();
        bool begin() override;
        void createAPServer(AsyncWebServer&) override;
        WifiWrapper& operator=(const WifiWrapper&) override { return *this; };
};

#endif