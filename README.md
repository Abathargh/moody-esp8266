# *moody-esp8266*

<p style='text-align: justify;'>
An implementation of the Moody Architecture for the end-devices (sensors/actuators) using the ESP8266-01 board. It's based on the ESP being programmed through the Arduino IDE Board Manager for ESP8266.

**EXPERIMENTAL** ESP32 support is implemented but not fully tested yet.

**TODO** Add deep sleep capabilities, working on how to implement it via hw mod on pin
</p>

## Contents

- [Dependencies:](#dependencies)
- [Installation (Arduino IDE)](#installation-arduino-ide)
- [Functionalities](#functionalities)
- [Sketches](#sketches)
    - [Adding the broker TLS certificates](#adding-the-broker-tls-certificates)
    - [Writing a sensor sketch](#writing-a-sensor-sketch)
    - [Writing an actuator sketch](#writing-an-actuator-sketch)
    
## Dependencies:

- ESP8266WiFi.h
- PubSubClient.h
- ESPAsyncTCP.h
- ESPAsyncWebServer.h
- ArduinoJson.h

When using this library on an ESP32, the following dependencies substitute ESP826WiFi.h and ESPAsyncTCP.h respectively:

- WiFiClientSecure.h
- AsyncTCP.h

## Installation (Arduino IDE)

<p style="text-align: justify;">
Clone this repository and zip it, or click download zip from the upper right corner of this github page if you want to have a release with the latest features.</p>
<p style='text-align: justify;'>
If you want to work with the latest stable release download the related zip file from https://github.com/Abathargh/moody-esp8266/releases, then add it to your environment by clicking <b>Sketch -> Include Library -> Add -> .ZIP Library...</b> and selecting the zip file you just downloaded.</p>

<p style="text-align: justify;">
You can also just clone the repo into your Arduino/libraries directory.
</p>


## Functionalities 
<p style='text-align: justify;'>
A node, sensor or actuator, can be used in Access Point mode or in normal mode. When used for the first time, the node starts in AP mode and is reachable under the 192.168.4.1 IP by connecting to its open WiFi network, that will have be named MoodyNode-xxx, with xxx being 3 random numbers.
</p>

<p style='text-align: justify;'>
Once you reach the node, you can insert the SSID and the key of your local router alongside the IP of the   gateway node within your network. The node will save the information, reboot and try to connect to it; if it fails it will enter AP mode once again.
</p>

<p style='text-align: justify;'>
After connecting to the network, the node will start its normal activity: a sensor will read data and forward it to the moody broker. An actuator can be used in two different modes: actuation, where it receives data to control a device, or server mode. This last one is used so that its information about its mappings can be queried or modified.
</p>

## Sketches

### Adding the broker TLS certificates

Since version 0.2 MoodyEsp8266, and the Moody project in general, requires TLS for its MQTT communications. In order to set it up on the ESP8266, you need to pick the ca.crt generated following the instructions included in the main [Moody repository](https://github.com/Abathargh/moody-go#configuration-and-certificates-setup) and upload it via the web interface.


### Writing a sensor sketch

<p style='text-align: justify;'>
Sensor sketches are based around the idea of a service, which is a way to obtain data from a physical (or virtual) sensor and expose it through an MQTT topic. You can have up to 3 services 
within a MoodySensor; first, you create a function that acquires data from the sensor and returns it as a <i>String</i>. Then you register it to the MoodySensor object and you start it up:
</p> 

```c++
#include <MoodyEsp8266.h>

uint8_t c = 0;
MoodySensor sensor;

// Simple function that emulates data acquisition from a source
String countService() {
    return String(++c);
}

void setup() {
    // Pass the function to the registerService function alongside 
    // the topic related to the service 
    sensor.register("count", countService);
    ...
}

void loop() {
    sensor.loop();
}
```
<p style='text-align: justify;'>
This way, the countService function will be periodically called, and its result will be 
forwarded using the moody/service/<b>service-name</b> topic.
</p>

### Writing an actuator sketch

<p style='text-align: justify;'>
The MoodyActuator class has almost the same API as MoodySensor, but this time you have to pass to it a function that controls the device during the setup stage. This function must accept an unsigned 8 bit number that corresponds to a single action that can be performed.
</p>

<p style='text-align: justify;'>
When an MQTT message containing a situation is received from the gateway, the MoodyActuator object scans a table of situation/action mappings and, if a mapping exists for the received situation, it calls the function passed during the setup with the corresponding action.
An object of this class can have up to 10 mappings, that can be controlled through the server mode.
</p>

<p style='text-align: justify;'>
The following is an example of how to setup the actuator with a simple function to control the device:
</p>

```c++
#include <MoodyEsp8266.h>

void actuate(uint8_t action) {
    switch(action) {
        case 0:
            Serial.println("Exec action 0");
            break;
        case 1:
            Serial.println("Exec action 1");
            break;
        default:
            Serial.println("Not implemented");    
    }
}

void setup() {
    MoodyActuator::setActuate(actuate);
    ...
}
```
