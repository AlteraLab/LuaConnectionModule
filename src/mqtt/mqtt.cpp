#include <String.h>

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include "mqtt.h"
#include "base/base.h"
#include "event/event.h"
#include "includes/PubSubClient/PubSubClient.h"
#include "includes/ArduinoJson/ArduinoJson.h"

class WiFiClient espClient = WiFiClient();
class PubSubClient client = PubSubClient(espClient);

void mqtt_sub_topic(char *topic)
{
    #if defined(ESP8266) || defined(ESP32) || defined(ARDUINO)
        Serial.println(F("subscribe topic."));
    #endif
    
    client.subscribe(topic);
}

void mqtt_pub_topic(char *topic, BASE_KEYPAIR sets[], uint16_t len)
{
    StaticJsonDocument<256> doc;
    char buffer[256];

    while (len--)
    {
        doc[sets[len].key] = sets[len].value;
    }

    size_t n = serializeJson(doc, buffer);
    Serial.println(F("publish topic"));
    client.publish(topic, buffer, n);
}

void mqtt_regist_dev()
{
    BASE_KEYPAIR sets[] = {
        {"dev_mac", sb_base.mac_address},
        {"cur_ip", sb_base.cur_ip},
        {"dev_type", sb_base.dev_type}
    };
    #if defined(ESP8266) || defined(ESP32) || defined(ARDUINO)
        Serial.println(F("device registration request send to hub"));
    #endif

    //허브에게 디바이스 등록 정보 전송
    mqtt_pub_topic(DEV_REG, sets, sizeof(sets) / sizeof(sets[0]));
}

size_t mqtt_pub_result(size_t action_res)
{
    //이벤트의 수행 종료 후 결과를 허브에게 전송
    BASE_KEYPAIR sets[] = {
        {"status", "false"},
        {"dev_mac", sb_base.mac_address}
    };
    if (action_res)  sets[0].value = "true";

    mqtt_pub_topic(DEV_CTRL_END, sets, sizeof(sets) / sizeof(sets[0]));
}

void mqtt_reconnect()
{
    // Loop until we're reconnected
    while (client.connected())
    {
        Serial.print(F("Attempting MQTT connection..."));

        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);

        // Attempt to connect
        if (client.connect(clientId.c_str()))
        {
            Serial.println(F("connected"));

            //토픽 subscribe
            String buf = String(DEV_CTRL) + "/" + sb_base.mac_address;
            char *topic = const_cast<char *>(buf.c_str());
            mqtt_sub_topic(topic);

            // 허브에게 장비 등록 요청
            mqtt_regist_dev();
        }
        else
        {
            Serial.print(F("failed, rc="));
            Serial.print(client.state());
            Serial.println(F(" try again in 5 seconds"));

            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void mqtt_callback(char* topic, uint8_t* payload, unsigned int length)
{
    String msg = "";

    Serial.print(F("Message arrived ["));
    Serial.print(topic);
    Serial.print(F("] "));

    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    for (int i = 0; i < length; i++)
    {
        msg += (char)payload[i];
    }

    //받아온 json문자열 파싱, 파싱 실패시 함수 수행 중단
    StaticJsonDocument<256> doc;
    auto error = deserializeJson(doc, msg);
    if (error)
    {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(error.c_str());
        return;
    }

    Serial.println(msg);

    JsonArray cmd_list = doc[F("cmd")].as<JsonArray>();
    for (int i = cmd_list.size() - 1; i >= 0; i--)
    {
        JsonObject elem = cmd_list.getElement(i).as<JsonObject>();

        int code = elem[F("cmd_code")];

        SB_ACTION = event_grep(code);
        size_t action_result = event_exec(sb_action);
        if(code) mqtt_pub_result(action_result); 
    }
}

void mqtt_verify_connect()
{
    if(client.connected()) mqtt_reconnect();
    client.loop();
}