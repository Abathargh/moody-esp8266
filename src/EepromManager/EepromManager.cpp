#include "EepromManager.hpp"

#include <EEPROM.h>

bool EepromManager::started = false;
bool EepromManager::conninfoStarted = false;
bool EepromManager::mappingsStarted = false;

ConnectionInfo EepromManager::conninfo = ConnectionInfo{};
Mappings EepromManager::map = Mappings{};

void EepromManager::start(Mode mod) {
    if(started){
        return;
    }
    EEPROM.begin(mod == Mode::Sensor ? EEPROM_SIZE_SENSOR : EEPROM_SIZE_ACTUATOR);
    started = true;
}

void EepromManager::writeConnectionInfo(ConnectionInfo *info) {
    memcpy(&conninfo, info, sizeof(ConnectionInfo));
    EEPROM.put(CONNINFO_ADDR, conninfo);
    EEPROM.commit();
    conninfoStarted = true;
}

ConnectionInfo EepromManager::readConnectionInfo() {
    if(!conninfoStarted) {
        EEPROM.get(CONNINFO_ADDR, conninfo);
        conninfoStarted = true;
    }
    return conninfo;
}

ConnectionInfo& EepromManager::getConnectionInfoRef() {
    if(!conninfoStarted) {
        EEPROM.get(CONNINFO_ADDR, conninfo);
        conninfoStarted = true;
    }
    return conninfo;
}


void EepromManager::writeMappings(Mappings *mapping) {
    memcpy(&map, mapping, sizeof(Mappings));
    EEPROM.put(MAPPINGS_ADDR, map);
    EEPROM.commit();
    mappingsStarted = true;
}

void EepromManager::saveMappings(Mappings *mapping) {
    memcpy(&map, mapping, sizeof(Mappings));
}

void EepromManager::flashMappings() {
    EEPROM.put(MAPPINGS_ADDR, map);
    EEPROM.commit();
}

Mappings EepromManager::readMappings() {
    if(!conninfoStarted) {
        EEPROM.get(MAPPINGS_ADDR, map);
        mappingsStarted = true;
    }
    return map;
}

