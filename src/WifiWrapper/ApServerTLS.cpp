#include "WifiWrapper.hpp"

const char login_html[] PROGMEM = R"===(
<html>
    <head>
        <title>
            Moody Node - Connect to WiFi
        </title>
        <style>
            .login {
                margin: auto;
                width: 450px;
                height: auto;
                font-family: Courier New;
                border-radius: 25px;
                background-color: #ebeeff;
                text-align: left;
                padding: 1%;
                padding-right: 0%;
                text-decoration: none;
                font-size: 15px;
                border: 1px solid #af7bc7;
            }

            .login h3 {
                text-align: center;
            }

            .login table {
                margin: auto;
                padding: 0;
            }

            .login td {
                font-size: 15px;
            }
        </style>
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
                    <td>Broker Address:</td>
                    <td><input type="text" name="broker" id="broker"/></td>
                </tr>
                <tr>
                    <td>Broker Fingerprint:</td>
                    <td><input type="text" name="fingerprint" id="fingerprint"/></td>
                </tr>
                <tr>
                    <td>GW cert:</td>
                    <td><input type="file" id="cert" value="Gw cert" accept=".crt"></td>
                </tr>
                <tr>
                    <td><input type="submit" value="Connect" onclick="save_credentials()" /></td>
                </tr>
            </table>
        </div>
    </body>
    <script>
        function save_credentials() {
            let xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if(this.readyState === 4 && this.status === 200){
                    alert("The esp will now reset and try to connect to the specified network.");
                } else if(this.status === 422) {
                    alert("Invalid or missing parameter!");
                }
            };
            xhttp.open("POST", "/connect", true);
            xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
            let ssid = document.getElementById("ssid").value;
            let key = document.getElementById("key").value;
            let broker = document.getElementById("broker").value;
            let fingerprint = document.getElementById("fingerprint").value;
            if(!ssid || !key || !broker || !fingerprint) {
                alert("A field is missing!");
            }
            var file = document.getElementById("cert").files[0];
            if(file) {
                var reader = new FileReader();
                reader.readAsText(file, "UTF-8");
                reader.onload = function (evt) {
                    let cert = evt.target.result;
                    console.log(cert);
                    console.log(file);
                    let params = "ssid="+ssid+"&key="+key+"&broker="+broker+"&fingerprint="+fingerprint+"&cert="+cert;
                    xhttp.send(params);
                }
                reader.onerror = function (evt) {
                    alert(evt.target.error);
                }
            }
        }

    </script>
</html>
)===";

// Validates a post request coming from the /connect endpoint, putting the received ssid and key
// into values and returning true if the content of the request was valid.
// If false is returned, nothing can be said about the contents of values;
bool WifiWrapperTLS::validPostConnect(AsyncWebServerRequest *request)
{
    bool p1 = request->hasParam("ssid", true);
    bool p2 = request->hasParam("key", true);
    bool p3 = request->hasParam("broker", true);
    bool p4 = request->hasParam("fingerprint", true);
    bool p5 = request->hasParam("cert", true);
    bool isInvalid = !p1 || !p2 || !p3 || !p4 || !p5;

    if (isInvalid)
    {
        return false;
    }
    String ssid = request->getParam("ssid", true)->value();
    String key = request->getParam("key", true)->value();
    String broker = request->getParam("broker", true)->value();
    String fingerprint = request->getParam("fingerprint", true)->value();
    String cert = request->getParam("cert", true)->value();

    unsigned int l1 = ssid.length();
    unsigned int l2 = key.length();
    unsigned int l3 = broker.length();
    unsigned int l4 = fingerprint.length();
    unsigned int l5 = cert.length();

    return l1 > 0 && l1 <= SSID_LENGTH && l2 > 0 && l2 <= KEY_LENGTH && l3 > 0 && 
        l3 <= BROKER_ADDR_LENGTH && l4 > 0 && l4 <= FINGERPRINT_LENGTH && l5 > 0 && l5 <= CERT_LENGTH;
}

void WifiWrapperTLS::createAPServer(AsyncWebServer &server)
{
    IPAddress IP = WiFi.softAPIP();
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", login_html);
    });

    server.on("/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!WifiWrapperTLS::validPostConnect(request))
        {
            request->send(422, "text/plain", "wrong syntax");
            return;
        }
        request->send(200, "text/plain", "ok");
        delay(1000);

        ConnectionInfo info;
        
        info.ok = CONN_OK;
        String ssid = request->getParam("ssid", true)->value();
        String key = request->getParam("key", true)->value();
        String broker = request->getParam("broker", true)->value();
        String fingerprint = request->getParam("fingerprint", true)->value();
        String cert = request->getParam("cert", true)->value();

        strcpy(info.SSID, ssid.c_str());
        strcpy(info.KEY, key.c_str());
        strcpy(info.BROKER_ADDR, broker.c_str());
        strcpy(info.FINGERPRINT, fingerprint.c_str());
        strcpy(info.CERT, cert.c_str());

        EepromManager::writeConnectionInfo(&info);
#if defined(ESP8266)
        ESP.reset();
#else
        ESP.restart();
#endif
    });
}