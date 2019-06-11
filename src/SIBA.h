#ifndef SIBA_H
#define SIBA_H

#include "base/base.h"
#include "mqtt/mqtt.h"
#include <ESP8266WiFi.h>

class SIBA {
    private:
        //MQTT mqtt;
    public:
        SIBA();
        size_t siba_init_wifi(const char* ssid, const char* password, const char* devId);
        void siba_verify_connect();
        void siba_event_add(int code, SB_ACTION);
        void siba_event_register();
};

#endif