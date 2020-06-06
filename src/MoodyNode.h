#ifndef MOODY_NODE_H_
#define MOODY_NODE_H_

#include <stdint.h>

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>


const long APRand = random(100, 500);

// Stores and retrieves connection information in EEPROM
// (emulated via flash memory)
struct connection_info {
    uint8_t ok;
    char SSID[SSID_LENGTH];
    char KEY[KEY_LENGTH];
} conninfo;


WiFiClient wifiClient;
PubSubClient client(wifiClient);
        

// Base class for nodes, containing connection/AP mechanisms 
class MoodyNode {
    private:
        void activateAPMode();
        void connectToWifi();

    protected:
        void connectToBroker();
        virtual void lastSetup() = 0; // implemented by heirs to add setup steps
        virtual void lastLoop() = 0; // implemented by heirs to add actions in the main loop

    public:
        virtual void begin(int baudRate);
        virtual void loop();
};

void MoodyNode::activateAPMode() {
    APWebServer server;
    server.begin();
}

void MoodyNode::connectToWifi() {
    Serial.println(conninfo.SSID);
    Serial.println(conninfo.KEY);
    
    WiFi.mode(WIFI_STA);
    uint8_t attempt = 0;

    while(attempt < MAX_ATTEMPTS) {
        Serial.print("Connecting to the WiFi - Attempt n.");
        Serial.println(++attempt);
        WiFi.begin(conninfo.SSID, conninfo.KEY);
        if(WiFi.waitForConnectResult() != WL_CONNECTED) {
            delay(200);
        } else {
            break;
        }
    }

    if (attempt >= MAX_ATTEMPTS) {
        Serial.println("The connection data may be wrong, re-activating AP mode");
        conninfo.ok = CONN_NOT_OK;
        EEPROM.put(0, conninfo);
        EEPROM.commit();
        ESP.reset();
    }
    Serial.print("Connected with ip: ");
    Serial.print(WiFi.localIP());
    Serial.print(" MAC: ");
    Serial.println(WiFi.macAddress());
}

void MoodyNode::connectToBroker() {
    while (!client.connected()) {
        Serial.printf("Trying to connect to the broker @%s\n", MQTT_BROKER);
        char  clientId[13];
        sprintf(clientId, "MoodyNode-%d", APRand);
        if (client.connect(clientId)) {
            Serial.println("Connected!");
        }else{   
            Serial.print("Connection failed, rc=");
            Serial.print(client.state());
            Serial.println(" trying again in 5 seconds.");
            delay(5000);
        }
    }
}

void MoodyNode::begin(int baudRate) {
    Serial.begin(baudRate);

    #ifdef ACTUATOR_MODE
    EEPROM.begin(EEPROM_SIZE_ACTUATOR);
    #else
    EEPROM.begin(EEPROM_SIZE_SENSOR);
    #endif
    EEPROM.get(CONNINFO_ADDR, conninfo);

    if(conninfo.ok != CONN_OK) {
        activateAPMode();
    } else {
        connectToWifi();
        client.setServer(MQTT_BROKER, MQTT_PORT);
        lastSetup();
    }
}

void MoodyNode::loop() {
    if(!client.connected()) {
        connectToBroker();
    }
    client.loop();
    lastLoop();
}

class APWebServer {
    private:
        AsyncWebServer server;
    
    public:
        APWebServer() : server(80) {}
        void begin();
};

APWebServer::APWebServer() : server(WEB_SERVER_PORT) {
    char randSSID[12];
    sprintf(randSSID, "%s%d", AP_SSID, APRand);

    // Init ESP WiFi as AP
    WiFi.softAP(randSSID);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", login_html);
    });

    server.on("/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
        String ssid, key;
        if (request->hasParam("ssid", true) && request->hasParam("key", true)) {
            ssid = request->getParam("ssid", true)->value();
            key = request->getParam("key", true)->value();
            Serial.print("Received: ");
            Serial.println(ssid);
            Serial.println("Received: ");
            Serial.print(key);
            if (ssid == "" || key == "" || ssid.length() > SSID_LENGTH || key.length() > KEY_LENGTH) {
                request->send(422, "application/json", "{\"outcome\": \"error1\"}");
            } else {
                request->send(200, "application/json", "{\"outcome\": \"ok\"}");
                conninfo.ok = CONN_OK;
                strcpy(conninfo.SSID, ssid.c_str());
                strcpy(conninfo.KEY, key.c_str());
                Serial.println(conninfo.SSID);
                Serial.println(conninfo.KEY);
                EEPROM.put(CONNINFO_ADDR, conninfo);
                EEPROM.commit();
                ESP.reset();
            }
        } else {
            request->send(422, "application/json", "{\"outcome\": \"error2\"}");
        }
    });
}

void APWebServer::begin() {
        server.begin();
}

const char login_html[] PROGMEM = R"===(
<html>
    <head>
        <title>
            Moody Node - Connect to WiFi
        </title>
        <style>
            .login {
                margin: auto;
                width: 330px;
                height: 120px;
                font-family: Courier New;
                border-radius: 25px;
                background-color: #ebeeff;
                text-align: left;
                padding-left: 2%;
                padding-top: 2%;
                padding-bottom: 3%;
                text-decoration: none;
                font-size: 15px;
                border: 1px solid #af7bc7;
            }
        </style>
        <script>
            function save_credentials() {
                let xhttp = new XMLHttpRequest();
                xhttp.onreadystatechange = function() {
                    if(this.readyState == 4 && this.status == 200){
                        alert("The esp will now reset and try to connect to the specified network.");
                    } else {
                        alert("Insert both an ssid and a key");
                    }
                };
                xhttp.open("POST", "/connect", true);
                xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
                let ssid = document.getElementById("ssid").value;
                let key = document.getElementById("key").value;
                let params = "ssid="+ssid+"&key="+key;
                xhttp.send(params);
            }
        </script>
    </head>
    <body>
        <div class="login">
            <h3>Connect to your home WiFi</h3>
            <table>
                <tr>
                    <td>SSID:</td>
                    <td><input type="text" name="ssid" id="ssid" /></td>
                </tr>
                <tr>
                    <td>KEY:</td>
                    <td><input type="password" name="key" id="key"/></td>
                </tr>
                <tr>
                    <td><input type="submit" value="Connect" onclick="save_credentials()" /></td>
                </tr>
            </table>
        </div>
    </body>
</html>
)===";
#endif