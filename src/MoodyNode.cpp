#include "MoodyEsp8266.h"

void MoodyNode::begin() {
    bool okConn = wifiWrapper.begin();
    DEBUG_MSG("conn: %d", okConn ? 1 : 0);
    if (!okConn) {
        return;
    }
    lastSetup();
}

void MoodyNode::loop() {
    if (!wifiWrapper.getApMode()) {
        bool connOk = wifiWrapper.checkConn();
        if(!connOk) {
            return;
        }
        lastLoop();
        wifiWrapper.loop();
    }
}
