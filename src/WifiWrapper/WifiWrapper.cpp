#include "WifiWrapper.hpp"

WifiWrapper::WifiWrapper() : apMode(false), client(), apServer(WEB_SERVER_PORT) {}

bool WifiWrapper::getApMode() {
    return apMode;
}

void WifiWrapper::activateAPMode() {
    const long APRand = random(100, 500);
    char randSSID[13];
    snprintf(randSSID, sizeof(randSSID), "%s%d", AP_SSID, APRand);
    // Init ESP WiFi as AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(randSSID);
    apMode = true;
    apServer.begin();
}

bool WifiWrapper::connectToWifi() {
    WiFi.mode(WIFI_STA);
    uint8_t attempt = 0;

    ConnectionInfo info = EepromManager::readConnectionInfo();

    while (attempt < MAX_ATTEMPTS) {
        attempt++;
        DEBUG_MSG("Connecting to the WiFi - Attempt n.%d\n", attempt);
        WiFi.begin(info.SSID, info.KEY);
        if (WiFi.waitForConnectResult() == WL_CONNECTED) {
            return true;
        }
        delay(1000);
    }
    return false;
}

bool WifiWrapper::connectToBroker() {
    // client.setServer does not allocate memory for the id, so a reference is necessary
    ConnectionInfo &info = EepromManager::getConnectionInfoRef();
    client.setServer(info.BROKER_ADDR, MQTT_PORT);
    int attempt = 0;
    while (!client.connected() && attempt < MAX_ATTEMPTS) {
        DEBUG_MSG("Trying to connect to the broker @%s - Attempt n.%d\n", info.BROKER_ADDR, ++attempt);
        char buf_id[14];
        snprintf(buf_id, sizeof(buf_id), "MoodyNode-%ld", random(100, 1000));
        if (client.connect((const char*)buf_id)) {
            return true;
        }
        DEBUG_MSG("Connection failed, rc=%d trying again in 5 seconds\n", client.state());
        delay(2000);
    }

    return false;
}

bool WifiWrapper::checkConn() {
    bool wifiConn = WiFi.isConnected();
    if (!wifiConn) {
        bool okWifi = connectToWifi();
        if (!okWifi) {
            activateAPMode();
            return false;
        }
    }

    bool mqttConn = client.connected();
    if (!mqttConn) {
        bool okMqtt = connectToBroker();
        if (!okMqtt) {
            DEBUG_MSG("The broker address may be wrong");
            activateAPMode();
            return false;
        }
    }
    return true;
}

void WifiWrapper::loop() {
    client.loop();
}

bool WifiWrapper::publish(const char* topic, const char* payload) {
    return client.publish(topic, payload);
}

bool WifiWrapper::subscribe(const char* topic){
    return client.subscribe(topic);
}

void WifiWrapper::setCallback(MQTT_CALLBACK_SIGNATURE) {
    client.setCallback(callback);
}

void WifiWrapper::mqttDisconnect() {
    client.disconnect();
}