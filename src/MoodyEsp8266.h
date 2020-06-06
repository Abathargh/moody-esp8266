#ifndef MOODYESP8266_H_
#define MOODYESP8266_H_

#define CONN_OK 1
#define CONN_NOT_OK 0

#define SSID_LENGTH 32
#define KEY_LENGTH 64
#define MAX_ATTEMPTS 5

#define AP_SSID "MoodyNode" 
#define WEB_SERVER_PORT 80

#define CONNINFO_ADDR 0
#define MAPPINGS_ADDR 104
#define EEPROM_SIZE_SENSOR 104
#define EEPROM_SIZE_ACTUATOR 300 // 272 should be enough 

#define MQTT_PORT 1883
#define MQTT_BROKER "moody-broker"
#define MQTT_MAX_PACKET_SIZE 256
#define MQTT_KEEPALIVE       60
#define MQTT_SOCKET_TIMEOUT  60

#include "MoodyNode.h"
#include "MoodySensor.h"
#include "MoodyActuator.h"

#endif