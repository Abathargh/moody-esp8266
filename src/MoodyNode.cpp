#include "MoodyEsp8266.h"

#define MQTT_PORT 1883
#define MQTT_BROKER "192.168.1.191"


connection_info MoodyNode::conninfo;

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
                    if(this.readyState === 4 && this.status === 200){
                        alert("The esp will now reset and try to connect to the specified network.");
                    } else if(this.status === 422) {
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

const long APRand = random(100, 500);

// Validates a post request coming from the /connect endpoint, putting the received ssid and key
// into values and returning true if the content of the request was valid.
// If false is returned, nothing can be said about the contents of values;
bool validPostConnect(AsyncWebServerRequest *request) {
    bool p1 = request->hasParam("ssid", true);
    bool p2 = request->hasParam("key", true);
    if (!p1 || !p2) { return false; }
    String ssid = request->getParam("ssid", true)->value();
    String key = request->getParam("key", true)->value();

    unsigned int l1 = ssid.length();
    unsigned int l2 = key.length();
    
    return l1 > 0 && l1 < SSID_LENGTH && l2 > 0 && l2 < KEY_LENGTH;
}
AsyncWebServer createAPServer(int port) {
    char randSSID[12];
    sprintf(randSSID, "%s%d", AP_SSID, APRand);

    // Init ESP WiFi as AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(randSSID);
    IPAddress IP = WiFi.softAPIP();

    AsyncWebServer server(port);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", login_html);
    });

    server.on("/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!validPostConnect(request)) {
            request->send(422, "text/plain", "wrong syntax");
            return;
        }
        request->send(200, "text/plain", "ok");
        delay(200);
        MoodyNode::conninfo.ok = CONN_OK;
        String ssid = request->getParam("ssid", true)->value();
        String key = request->getParam("key", true)->value();
        strcpy(MoodyNode::conninfo.SSID, ssid.c_str());
        strcpy(MoodyNode::conninfo.KEY, key.c_str());
        EEPROM.put(CONNINFO_ADDR, MoodyNode::conninfo);
        EEPROM.commit();
        ESP.reset();
    });
    return server;
}



WiFiClient MoodyNode::wifiClient = WiFiClient();
PubSubClient MoodyNode::client = PubSubClient(wifiClient);
AsyncWebServer MoodyNode::apServer = createAPServer(WEB_SERVER_PORT);

void MoodyNode::activateAPMode() {
    apMode = true;
    apServer.begin();
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
        EEPROM.put(CONNINFO_ADDR, conninfo);
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
        String clientId = "MoodyNode-" + String(random(100, 1000));
        if (client.connect(clientId.c_str())) {
            Serial.println("Connected!");
        } else {   
            Serial.print("Connection failed, rc=");
            Serial.print(client.state());
            Serial.println(" trying again in 5 seconds.");
            delay(5000);
        }
    }
}

void MoodyNode::begin(int baudRate) {
    Serial.begin(baudRate);
    EEPROM.begin(EEPROM_SIZE_ACTUATOR);
    EEPROM.get(CONNINFO_ADDR, conninfo);

    Serial.println(conninfo.ok);
    Serial.println(conninfo.SSID);
    Serial.println(conninfo.KEY);

    if(conninfo.ok != CONN_OK) {
        activateAPMode();
    } else {
        connectToWifi();
        client.setServer(MQTT_BROKER, MQTT_PORT);
        lastSetup();
    }
}

void MoodyNode::loop() {
    if(!apMode) {
        bool conn = client.connected();
        if(!conn) {
            connectToBroker();
            delay(1000);
        }
        client.loop();
        lastLoop();
    }
}

