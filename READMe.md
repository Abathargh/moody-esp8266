## **moody-esp8266**
An implementation of the Moody Architecture for the end-devices (sensors/actuators) based on ESP8266-01 as hardware. It's based on the ESP running with the NodeMCU firmware, following the instructions contained in this antimait article (in italian) http://antima.it/costruisci-la-tua-rete-domotica-con-esp8266-e-raspberry-pi-caricare-il-firmware-nodemcu/.

Dependencies:
- ESP8266WiFi.h
- PubSubClient.h
- ESPAsyncTCP.h
- ESPAsyncWebServer.h
- ArduinoJson.h

Since There's a regex route used in the Actuator web server, you have to enable a custom flag while compiling as written in the README for the ESPAsyncWebServer project. Instructions at the bootom of this page https://github.com/me-no-dev/ESPAsyncWebServer.

### ***writing a sensor sketch***

Sensor sketches are based around the idea of a service, which is a way to obtain data from a physical (or virtual) sensor and expose it through a MQTT message. You can have up to 3 services 
within a MoodySensor; first, you create a function that captures data from the sensor and returns it as a <i>String</i>. then you register it to the MoodySensor object and you start it up: 
```c++
uint8_t n = 0;
MoodySensor sensor;

// Simple function that emulates a data source
String countService() {
    return String(++c);
}

void setup() {
    // Pass the function to the registerService function alongside 
    // the topic related to the service 
    sensor.register("count",countService);
    ...
}
```
This way, the countService function will be periodically called, and its result will be 
forwarded using the mody/service/\<service-name\> topic.