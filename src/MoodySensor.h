#ifndef MOODY_SENSOR_H_
#define MOODY_SENSOR_H_

#define SENSOR_MODE

#define MAX_SERVICES 3
#define BASE_TOPIC "moody/service/"
#define BASE_SIZE 14


using callback = char*(*)();


class MoodySensor : public MoodyNode {
    private:
        uint8_t activeServices;
        const char* serviceTopics[MAX_SERVICES];
        callback callbacks[MAX_SERVICES];
        void lastSetup();
        void lastLoop();

    public:
        MoodySensor();
        void registerService(const char* topic, callback callback);
};

void MoodySensor::registerService(const char* topic, callback callback) {
    // if this is called more than MAX_SERVICES times, the call is ignored
    if(activeServices < MAX_SERVICES) {
        int topicLength = BASE_SIZE + strlen(topic);
        char fullTopic [topicLength];
        sprintf(fullTopic, "%s%s", BASE_TOPIC, topic);
        
        serviceTopics[activeServices] = fullTopic;
        callbacks[activeServices] = callback;
        activeServices++;
    }
}

void MoodySensor::lastSetup() {
    return;
}

void MoodySensor::lastLoop() {
    int i;
    for(i = 0; i < activeServices; i++) {
        char* data = callbacks[i]();
        client.publish(serviceTopics[i], data);
    }
}

#endif