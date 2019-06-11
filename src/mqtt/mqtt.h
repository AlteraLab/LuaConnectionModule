#ifndef SIBA_MQTT_h
#define SIBA_MQTT_h

#include <ESP8266WiFi.h>
#include <SPI.h>
#include "base/base.h"
#include "includes/PubSubClient/PubSubClient.h"

/*
class MQTT{
private:
    
public:
    MQTT();
    
    void mqtt_reconnect();
    void mqtt_callback(char* topic, uint8_t* payload, unsigned int length);
    void mqtt_sub_topic(char *topic);
    void mqtt_pub_topic(char *topic, BASE_KEYPAIR base_keypair[], uint16_t len);
    void mqtt_regist_dev();
    size_t mqtt_pub_result(size_t action_res);
    void mqtt_verify_connect();
};
*/

void mqtt_reconnect();
void mqtt_callback(char* topic, uint8_t* payload, unsigned int length);
void mqtt_sub_topic(char *topic);
void mqtt_pub_topic(char *topic, BASE_KEYPAIR base_keypair[], uint16_t len);
void mqtt_regist_dev();
size_t mqtt_pub_result(size_t action_res);
void mqtt_verify_connect();

extern WiFiClient espClient;
extern PubSubClient client;

#endif