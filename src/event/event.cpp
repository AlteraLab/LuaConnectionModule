#include <Arduino.h>
#include "base/base.h"
#include "event.h"

void (*event_grep(int code))()
{
    
    size_t temp_index = action_cnt;
    SB_ACTION = NULL;

    while (temp_index--)
    {
        if (action_store[temp_index].sb_code == code)
        {
            sb_action = action_store[temp_index].sb_action;
            break;
        }
    }

    return sb_action;
}

size_t event_exec(SB_ACTION)
{
    size_t ret = 0;

    if (ret = sb_action != NULL) sb_action(); //액션 수행
    else
    {
    #if defined(ESP8266) || defined(ESP32) || defined(ARDUINO)
        Serial.println(F("action is empty"));
    #endif
    }

    return ret;
}

size_t event_add(int code, SB_ACTION)
{
    size_t ret = 0;

    if (code == 0 && action_cnt)
    {
        Serial.println(F("cannot register this code and event"));
        return ret;
    }
    if (ret = action_cnt != EVENT_COUNT - 1) action_store[action_cnt++] = {code, sb_action};
    return ret;
}