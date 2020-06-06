#include <EEPROM.h>

#define CONN_OK 1
#define CONN_NOT_OK 0
#define AP_SSID "Moody Node" 
#define SSID_LENGTH 32
#define KEY_LENGTH 64
#define BROKER_ADDR_LENGTH 16
#define MAX_ATTEMPTS 5

struct como {
    uint8_t ok;
    char no[80];
} comone;

struct connection_info {
    uint8_t ok;
    char SSID[SSID_LENGTH];
    char KEY[KEY_LENGTH];
} conninfo;



void setup() {
    Serial.begin(9600);
    while(!Serial);
    delay(5000);
    EEPROM.begin(512);
    EEPROM.get(0, conninfo);
    EEPROM.get(8+SSID_LENGTH+KEY_LENGTH, comone);

    if(conninfo.ok != CONN_OK || comone.ok != CONN_OK) {
        Serial.println("not ok");
        conninfo.ok = CONN_OK;
        strcpy(conninfo.SSID, "ciaone culo1");
        strcpy(conninfo.KEY, "wewexD");
        comone.ok = CONN_OK;
        strcpy(comone.no, "devo farmare");
        EEPROM.put(0, conninfo);
        EEPROM.put(8+SSID_LENGTH+KEY_LENGTH, comone);
        EEPROM.commit();
        ESP.reset();
    } else {
        Serial.println(conninfo.ok);
        Serial.println(conninfo.SSID);
        Serial.println(conninfo.KEY);
        Serial.println();
        Serial.println(comone.ok);
        Serial.println(comone.no);
    }
}

void loop(){}

