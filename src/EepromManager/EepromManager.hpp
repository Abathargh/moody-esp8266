#ifndef EEPROM_MANAGER_H_
#define EEPROM_MANAGER_H_

#if defined(ESP8266)
#include <EEPROM.h>
#else
#include <Preferences.h>
#endif

#include <stdint.h>

// conninfo defines
#define OK_LENGTH 1
#define SSID_LENGTH 32
#define KEY_LENGTH 64
#define BROKER_ADDR_LENGTH 16

#if !defined(HTTP_ONLY)
#define FINGERPRINT_LENGTH 60
#define CERT_LENGTH 1300
#else
#define FINGERPRINT_LENGTH 0
#define CERT_LENGTH 0
#endif

#define CONN_INFO_LENGTH (OK_LENGTH+SSID_LENGTH+KEY_LENGTH+BROKER_ADDR_LENGTH+FINGERPRINT_LENGTH+CERT_LENGTH)

// mappings defines
#define MAPPING_SIZE 1
#define MAX_ACTION_SIZE 5
#define MAX_ACTION_NUM 10
#define MAPPING_LENGTH (MAPPING_SIZE+MAX_ACTION_SIZE+MAX_ACTION_NUM)

// EEPROM defines
#define CONNINFO_ADDR 0
#define MAPPINGS_ADDR CONN_INFO_LENGTH
#define EEPROM_SIZE_SENSOR CONN_INFO_LENGTH
#define EEPROM_SIZE_ACTUATOR (CONN_INFO_LENGTH+MAPPING_LENGTH)


struct ConnectionInfo {
    uint8_t ok;
    char SSID[SSID_LENGTH];
    char KEY[KEY_LENGTH];
    char BROKER_ADDR[BROKER_ADDR_LENGTH];
#ifndef HTTP_ONLY
    char FINGERPRINT[FINGERPRINT_LENGTH];
    char CERT[CERT_LENGTH];
#endif
};

struct Mappings {
    // iterate over situations, find index of received situation
    // access action and actuate using it.
    uint8_t size; // how many mappings are currently present?
    uint8_t situations[MAX_ACTION_NUM];
    uint8_t actions[MAX_ACTION_NUM];
};

class EepromManager {
    public:
        enum class Mode { Sensor, Actuator };
        static void start(Mode);
        static void writeConnectionInfo(ConnectionInfo*);
        static void saveConnectionInfo(ConnectionInfo*);
        static ConnectionInfo readConnectionInfo();
        static ConnectionInfo& getConnectionInfoRef();

        static void writeMappings(Mappings*);
        static void saveMappings(Mappings*);
        static void flashMappings();
        static Mappings readMappings();
    private:
        static bool started;
        static bool conninfoStarted;
        static bool mappingsStarted;
        static Mappings map;
        static ConnectionInfo conninfo;

        static void readConnFromEeprom();
        static void readMappingsFromEeprom();
        static void writeConnToEeprom();
        static void writeMappingsToEeprom();
#if defined(ESP32)
        static Preferences pref;
#endif
        EepromManager(){};
        ~EepromManager(){};
};

#endif