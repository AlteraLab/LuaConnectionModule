# ```c++
#include <SIBA.h>
/*
* Environment information:
* development is 1
* production is 0
*/
#define ENV 1
SIBA siba;
//define your router information
const char* ssid = ""; //your SIBA hub ssid
const char* pwd = ""; //your SIBA hub password
//your device's key for authentication
const char* hw_auth_key = "1b48ed73b91846e29728e39521382ac6";
//your device's name (this name will be bluetooth alias)
const char* dev_name = "test";
void add_ctrl_cmd_group() {
       
}
void add_sensing_group() {
    
}
void init_device_state() {
    /* define device state model
    * example) siba.init_state("key", value);
    * ----------------------------------------------
    */
}
void setup() {
    Serial.begin(115200); //board's baud rate
    /* put your other setup code here 
    * ----------------------------------------------
    */

    
    /* ---------------------------------------------*/
    add_ctrl_cmd_group(); //add all control command
    add_sensing_group(); //add all sensing event
    siba.init_regist(init_device_state);
    //connect SIBA IoT platform
    #if ENV
    siba.init(ssid, pwd, hw_auth_key);
    #else
    siba.init(hw_auth_key, dev_name);
    #endif
}
void loop() {
    //keep alive your device and SIBA platform
    siba.verify_connection();
}
```
