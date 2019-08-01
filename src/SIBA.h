/*
 * SIBA.h
 * Created by GyoJun Ahn @ DCU ICS Lab. on 05/08/2019
 *  
 */

#ifndef SIBA_h
#define SIBA_h

#define EVENT_COUNT 30

/* topic name managements */
#define DEV_CTRL "dev/control"
#define DEV_CTRL_END "dev/control/end"
#define DEV_REG "dev/register"
#define DEV_WILL "dev/will"
#define MQTT_SERVER "192.168.2.1"
#define MQTT_PORT 1883
#define REGISTER_EVENT_CODE -1



#include "includes/PubSubClient/PubSubClient.h"
#include <ESP8266WiFi.h>

typedef struct sb_dataset{
  size_t type;
  String value;
}sb_dataset;

#define SB_ACTION size_t (*sb_action)(size_t, sb_dataset[2], size_t)

typedef struct sb_event{
    int sb_code;
    SB_ACTION;
}sb_event;

typedef struct sb_keypair{
  String key;
  String value;
}siba_keypair;


class SIBA{

    private:
        char* ssid;
        char* pwd;
        char* mqtt_server;
        uint16_t mqtt_port;
        char* dev_type;
        static String mac_address;
        String cur_ip;
        String msg;
        static size_t is_reg;

        WiFiClient espClient;
        PubSubClient client;
        static size_t action_cnt;
        static sb_event action_store[EVENT_COUNT]; //이벤트를 담는 배열

        static SIBA context;

        size_t (*grep_event(int code))(size_t,  sb_dataset[2], size_t);
        size_t exec_event(SB_ACTION, size_t before, sb_dataset d_wrap[2], size_t len);
        size_t pub_result(size_t action_res);
        void regist_dev();
        void subscribe_topic(char* topic);
        void publish_topic(char* topic, char* buffer, uint16_t len);
        void init_wifi(char* ssid, char* pwd);
        void mqtt_reconnect();
        void parse_call();
        static void mqtt_callback(char *topic, uint8_t *payload, unsigned int length);

        static size_t register_event(size_t before, sb_dataset d_wrap[2], size_t len); //0번 코드에 대응되는 이벤트

    public:
        SIBA();

        //production 환경에서 사용하는 init
        size_t init(const char* auth_key); 

        //development 환경에서 사용하는 init
        size_t init(const char* ssid, const char* pwd, const char* dev_type); 
        size_t add_event(int code, SB_ACTION);
        void verify_connection();
};

#endif