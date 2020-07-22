#include "MoodyEsp8266.h"
#include <string.h>
#include <stdio.h>

void MoodySensor::registerService(const char *topic, callback callback)
{
    // if this is called more than MAX_SERVICES times, the call is ignored
    if (activeServices < MAX_SERVICES)
    {
        snprintf(serviceTopics[activeServices], MAX_TOPIC_SIZE, "%s%s", BASE_TOPIC, topic);
        callbacks[activeServices] = callback;
        activeServices++;
    }
}

void MoodySensor::lastSetup()
{
    return;
}

void MoodySensor::lastLoop()
{
    int i;
    for (i = 0; i < activeServices; i++)
    {
        String data = callbacks[i]();
        snprintf(msg, MSG_BUFFER_SIZE, "%s", data.c_str());
        Serial.printf("serviceTopics: %s\n", serviceTopics[i]);
        Serial.printf("payload: %s\n", msg);
        bool ok = client.publish(serviceTopics[i], msg);
        delay(2000);
    }
}