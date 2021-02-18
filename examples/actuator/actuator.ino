#include <MoodyEsp8266.h>


MoodyActuator actuator;

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
    actuator.begin();
}

void loop() {
    actuator.loop();
}