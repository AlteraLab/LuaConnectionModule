#include <String.h>

#include <Arduino.h>
#include "includes/ArduinoJson/ArduinoJson.h"
#include "includes/RF24-1.3.2/RF24.h"
#include "includes/PubSubClient/PubSubClient.h"
#include <ESP8266WiFi.h>
#include <SPI.h>
#include "SIBA.h"
#include "base/base.h"
#include "event/event.h"
#include "mqtt/mqtt.h"

#include <functional>


RF24 radio(CE,CSN);

SIBA::SIBA()
{
    sb_base.mqttPort = MQTT_PORT;
    sb_base.mqttServer = MQTT_SERVER;
}

size_t SIBA::siba_init_wifi(const char *ssid, const char *password, const char *dev_type)
{
    sb_base.ssid = const_cast<char *>(ssid);
    sb_base.password = const_cast<char *>(password);
    sb_base.dev_type = const_cast<char *>(dev_type);

    Serial.print(F("\nConnection to "));
    Serial.println(sb_base.ssid);

    WiFi.begin(sb_base.ssid,sb_base.password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(F("."));
    }
    
    randomSeed(micros());

    String macAddress = WiFi.macAddress();
    char mac_buf[18];
    strncpy(mac_buf,WiFi.macAddress().c_str(), 18);
    sb_base.mac_address = mac_buf;
    String ipv4 = WiFi.localIP().toString();
    char ipv4_buf[16];
    strncpy(ipv4_buf, ipv4.c_str(),16);
    sb_base.cur_ip = ipv4_buf;

    Serial.println(F("\nWiFi connected"));
    Serial.print(F("IP address :"));
    Serial.println(sb_base.cur_ip);
    Serial.print(F("MAC address : "));
    Serial.println(sb_base.mac_address);

    client.setServer(sb_base.mqttServer, sb_base.mqttPort);
    /*
#if defined(ESP8266) || defined(ESP32)
    std::function<void(char *, uint8_t *, unsigned int)> callback = mqtt.mqtt_callback;
#else
    void (*callback)(char *, uint8_t *, unsigned int);    
    callback = mqtt.mqtt_callback();
#endif
    client.setCallback(callback);
    */
    void (*callback)(char*, uint8_t*, unsigned int);
    callback = mqtt_callback;
    client.setCallback(callback);

    return 1;
}


void SIBA::siba_verify_connect()
{
    mqtt_verify_connect();
}

void SIBA::siba_event_add(int code, SB_ACTION)
{
    event_add(code, sb_action);
}

void SIBA::siba_event_register()
{
    Serial.println(F("Device register is finished"));
    sb_base.is_reg = true;
}