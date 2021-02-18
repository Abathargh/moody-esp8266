#include <MoodyEsp8266.h>


MoodySensor sensor;
uint8_t c = 0;

String countService() {
    return String(++c);
}

void setup() {
    sensor.registerService("count", countService);
    sensor.begin();
}

void loop() {
    sensor.loop();
}

