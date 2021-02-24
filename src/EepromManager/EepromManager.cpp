#include "EepromManager.hpp"

bool EepromManager::started = false;
bool EepromManager::conninfoStarted = false;
bool EepromManager::mappingsStarted = false;

ConnectionInfo EepromManager::conninfo = ConnectionInfo{};
Mappings EepromManager::map = Mappings{};

void EepromManager::start(Mode mod) {
    if(started){
        return;
    }
#if defined(ESP8266)
    EEPROM.begin(mod == Mode::Sensor ? EEPROM_SIZE_SENSOR : EEPROM_SIZE_ACTUATOR);
#elif defined(ESP32)
    pref.begin("conninfo", false);
    if(mod == Mode::Actuator) {
        pref.begin("mappings", false);
    }
#else
#endif
    started = true;
}

void EepromManager::writeConnectionInfo(ConnectionInfo *info) {
    memcpy(&conninfo, info, sizeof(ConnectionInfo));
    writeConnToEeprom();
    conninfoStarted = true;
}

ConnectionInfo EepromManager::readConnectionInfo() {
    readConnFromEeprom();
    return conninfo;
}

ConnectionInfo& EepromManager::getConnectionInfoRef() {
    readConnFromEeprom();
    return conninfo;
}


void EepromManager::writeMappings(Mappings *mapping) {
    memcpy(&map, mapping, sizeof(Mappings));
    writeMappingsToEeprom();
    mappingsStarted = true;
}

void EepromManager::saveMappings(Mappings *mapping) {
    memcpy(&map, mapping, sizeof(Mappings));
}

void EepromManager::flashMappings() {
    writeMappingsToEeprom();
}

Mappings EepromManager::readMappings() {
    readMappingsFromEeprom();
    return map;
}


void EepromManager::readConnFromEeprom() {
    if(!conninfoStarted) {
#if defined(ESP8266)
        EEPROM.get(CONNINFO_ADDR, conninfo);
#elif defined(ESP32)
        size_t len = pref.getBytesLength("conninfo");
        if(len == sizeof(ConnectionInfo)) {
            pref.readBytes("conninfo", &conninfo, len);
        }
#endif
        conninfoStarted = true;
    }
}

void EepromManager::readMappingsFromEeprom() {
        if(!mappingsStarted) {
#if defined(ESP8266)
        EEPROM.get(MAPPINGS_ADDR, map);
#elif defined(ESP32)
        size_t len = pref.getBytesLength("mappings");
        if(len == sizeof(Mappings)) {
            pref.readBytes("mappings", &map, len);
        }
#endif
        conninfoStarted = true;
    }
}

void EepromManager::writeConnToEeprom() {
#if defined(ESP8266)
    EEPROM.put(CONNINFO_ADDR, conninfo);
    EEPROM.commit();
#elif defined(ESP32)
    pref.putBytes("conninfo", &conninfo, sizeof(ConnectionInfo));
#endif
}

void EepromManager::writeMappingsToEeprom() {
#if defined(ESP8266)
    EEPROM.put(MAPPINGS_ADDR, map);
    EEPROM.commit();
#elif defined(ESP32)
    pref.putBytes("mappings", &map, sizeof(Mappings));
#endif
}