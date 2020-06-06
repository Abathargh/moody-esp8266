## **moody-esp8266**
An implementation of the Moody Architecture for the end-devices (sensors/actuators) based on ESP8266-01 as hardware. It's based on the ESP running with the NodeMCU firmware, following the instructions contained in this antimait article (in italian) http://antima.it/costruisci-la-tua-rete-domotica-con-esp8266-e-raspberry-pi-caricare-il-firmware-nodemcu/.

Dependencies:
- ESP8266WiFi.h
- PubSubClient.h
- ESPAsyncTCP.h
- ESPAsyncWebServer.h

Since There's a regex route used in the Actuator web server, ypu hav to enable a custom flag while compiling as written in the README for the ESPAsyncWebServer project. Instructions at the bootom of this page https://github.com/me-no-dev/ESPAsyncWebServer.