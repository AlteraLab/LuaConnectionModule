#ifndef SIBA_BASE_H
#define SIBA_BASE_H

#define EVENT_COUNT             30
#define DEV_CTRL                "dev/control"
#define DEV_CTRL_END            "dev/control/end"
#define DEV_REG                 "dev/register"
#define MQTT_SERVER             "192.168.2.1"
#define ADDRESS                 0x72646f4e31LL
#define MQTT_PORT               1883
#define REGISTER_EVENT_CODE     0
#define RADIO_CNT               2
#define CE                      4
#define CSN                     15
#define BAUDRATE                115200
#define SWITCH                  5
#define SB_ACTION               void (*sb_action)()

typedef struct BASE {
    char*                       ssid;
    char*                       password;
    char*                       dev_type;
    char*                       cur_ip;
    char*                       mac_address;
    bool                        is_reg;
    char*                       mqttServer;
    int                         mqttPort;
};

typedef struct BASE_EVENT {
    int                         sb_code;
    SB_ACTION;
};

typedef struct BASE_KEYPAIR {
    char*                       key;
    char*                       value;
};

extern BASE                     sb_base;
extern BASE_EVENT               base_event;
extern BASE_KEYPAIR             base_keypair;
extern int                      action_cnt;
extern BASE_EVENT               action_store[EVENT_COUNT];

#endif