/*
 * SIBA.h
 * Created by GyoJun Ahn @ Altera. on 05/08/2019
 *  
 */

#ifndef SIBA_h
#define SIBA_h

#include "includes/PubSubClient/PubSubClient.h"
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

#define EVENT_COUNT 30
#define SENSING_COUNT 10
#define STATE_COUNT 20
#define STATE_COUNT_LIMIT 20
#define EVENT_COUNT_LIMIT 30
#define SENSING_COUNT_LIMIT 10

/* topic name managements */
#define DEV_CTRL "dev/control"
#define DEV_CTRL_END "dev/control/end"
#define DEV_SET_STATE "dev/state/update"
#define DEV_INIT_STATE "dev/state/create"
#define DEV_REG "dev/register"
#define DEV_WILL "dev/will"
#define DEV_SENSING_RECV "dev/sensing"

#define MQTT_SERVER "192.168.2.1"
#define MQTT_PORT 1883
#define REGISTER_EVENT_CODE -1
#define BLUETOOTH_PIN "1234"
#define DEFAULT_SENSING_ACT_TIME 5000
//#define SIBA_RX 17
//#define SIBA_TX 18

#define SB_ACTION size_t (*sb_action)(size_t, sb_dataset[2], size_t)
#define INIT_GROUP void (*init_group)()
#define RESERVE_BYTE byte (*reserve_byte)()
#define RESERVE_INTEGER int (*reserve_int)()
#define RESERVE_DOUBLE double (*reserve_double)()
#define RESERVE_CHAR char (*reserve_char)()
#define RESERVE_STRING String (*reserve_str)()

#define SB_BYTE 1
#define SB_INT 2
#define SB_DOUBLE 3
#define SB_CHAR 4
#define SB_STRING 5

typedef struct sb_dataset{
  size_t type;
  String value;
}sb_dataset;

typedef struct sb_event{
    int sb_code;
    SB_ACTION;
}sb_event;

typedef union sb_data{
  byte sb_byte;
  int sb_int;
  double sb_double;
  char sb_char;
  char* sb_string;
  //String sb_string;
}siba_data;

typedef struct sb_keypair{
  String key;
  size_t type;
  sb_data value;
}siba_keypair;

typedef union sb_sensing_func{
  RESERVE_BYTE;
  RESERVE_INTEGER;
  RESERVE_DOUBLE;
  RESERVE_CHAR;
  RESERVE_STRING;
}siba_sensing_func;

typedef struct sb_sensing{
  String key;
  size_t type;
  sb_sensing_func func;
}siba_sensing;


class SIBA{

    private:
        char* ssid;
        char* pwd;
        char* mqtt_server;

        long internal_prev;
        long internal_curr;
        int sensing_time;
        size_t mode;
        String dev_name;

        uint16_t mqtt_port;
        char* dev_type;
        static String mac_address;
        String cur_ip;
        String msg;
        static size_t is_reg;

        WiFiClient espClient;
        PubSubClient client;
        static size_t action_cnt;
        static size_t sensing_cnt;
        static size_t state_cnt;
        static sb_event action_store[EVENT_COUNT]; //이벤트를 담는 배열
        static sb_sensing sensing_store[SENSING_COUNT]; //센싱 함수를 담는 배열
        static sb_keypair state_store[STATE_COUNT];

        static SIBA context;

        size_t (*grep_event(int code))(size_t,  sb_dataset[2], size_t);
        size_t exec_event(SB_ACTION, size_t before, sb_dataset d_wrap[2], size_t len);
        size_t pub_result(size_t action_res, int type);
        void regist_dev();
        void subscribe_topic(char* topic);
        void publish_topic(char* topic, char* buffer, uint16_t len);
        void init_wifi(char* ssid, char* pwd);
        void mqtt_reconnect();
        void parse_call();
        static void mqtt_callback(char *topic, uint8_t *payload, unsigned int length);

        static size_t register_event(size_t before, sb_dataset d_wrap[2], size_t len); //0번 코드에 대응되는 이벤트

        void send_to_hub(char* data_key, sb_data temp, size_t type, char* topic);

        void act_sensing();

        void add_sensing(String key, size_t type, sb_sensing_func func);

        int find_state_idx(char* key);

        void init_state_local(char *key, size_t type, sb_data temp);
    public:
        SIBA();

        void (*init_call)() = NULL;

        //production 환경에서 사용하는 init
        size_t init(const char* auth_key, const char* dev_name); 

        //development 환경에서 사용하는 init
        size_t init(const char* ssid, const char* pwd, const char* dev_type); 
        size_t add_event(int code, SB_ACTION);
        void verify_connection();

        //등록 여부 검증
        size_t init_regist(INIT_GROUP);

        //상태 값 추출
        sb_data get_state(char* key);

        //상태 설정
        void set_state(char* data_key, bool value, size_t option);
        void set_state(char* data_key, int value, size_t option);
        void set_state(char* data_key, double value, size_t option);
        void set_state(char* data_key, char value, size_t option);
        void set_state(char* data_key, char* value, size_t option);
        //void set_state(char* data_key, String value, size_t option);

        //상태 초기화
        void init_state(char* key, bool value, size_t option);
        void init_state(char* key, int value, size_t option);
        void init_state(char* key, double value, size_t option);
        void init_state(char* key, char value, size_t option);
        void init_state(char* key, char* value, size_t option);
        //void init_state(char* key, String value, size_t option);

        //센싱
        void reserve_sensing(char* key, RESERVE_BYTE);
        void reserve_sensing(char* key, RESERVE_INTEGER);
        void reserve_sensing(char* key, RESERVE_DOUBLE);
        void reserve_sensing(char* key, RESERVE_CHAR);
        //void reserve_sensing(char* key, RESERVE_STRING);

        void set_sensing_time(int time);

        //parse
        byte parse_byte(sb_data data);
        int parse_int(sb_data data);
        double parse_double(sb_data data);
        char parse_char(sb_data data);
        //String parse_string(sb_data data);
};

#endif