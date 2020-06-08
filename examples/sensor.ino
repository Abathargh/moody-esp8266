#include <MoodyEsp8266.h>

MoodySensor sensor;
uint8_t c = 0;

String countService() {
    return String(++c);
}

void setup() {
    delay(8000);
    sensor.registerService("count", countService);
    sensor.begin(9600);
}

void loop() {
    sensor.loop();
}

