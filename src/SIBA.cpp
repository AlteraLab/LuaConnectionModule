/*
 * SIBA.cpp
 * Created by GyoJun Ahn @ DCU ICS Lab. on 05/08/2019
 *  
 */

#include "SIBA.h"
#include <Arduino.h>
#include "includes/PubSubClient/PubSubClient.h"
#include "includes/ArduinoJson/ArduinoJson.h"
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

#if defined(ESP8266) || defined(ESP32)
#include <functional>
#endif

//initialize static member
SIBA SIBA::context;
size_t SIBA::action_cnt = 0;
size_t SIBA::sensing_cnt = 0;
size_t SIBA::state_cnt = 0;
sb_event SIBA::action_store[EVENT_COUNT] = {0};
size_t SIBA::is_reg = 0;
String SIBA::mac_address;
sb_sensing SIBA::sensing_store[SENSING_COUNT];
sb_keypair SIBA::state_store[STATE_COUNT];

SIBA::SIBA()
{
  this->mqtt_server = MQTT_SERVER;
  this->mqtt_port = MQTT_PORT;

  this->espClient = WiFiClient();
  this->client = PubSubClient(espClient);
  this->is_reg = 0;
  this->msg = "";
  this->sensing_time = DEFAULT_SENSING_ACT_TIME;
  this->mode = 0;
  this->add_event(REGISTER_EVENT_CODE, SIBA::register_event); //디바이스가 등록됬을 때 수행 될 이벤트
}

size_t SIBA::register_event(size_t before, sb_dataset dwrap[2], size_t len)
{
  //디바이스 정상 등록 시 로그 출력 및 LED 점등
  Serial.println(F("device register is finished"));
  context.is_reg = 1;

  //시간 비교 값 설정
  context.internal_prev = millis();
  context.internal_curr = millis();

  /*if(context.mode){
    SoftwareSerial BTSerial(2, 0); // RX | TX
    String result_st = String("{\"name\":\"")+context.dev_name+"\"}";
    BTSerial.print(result_st);
  }*/
  return is_reg;
}

size_t SIBA::init(const char *auth_key, const char *dev_name)
{
  this->mode = 1;
  this->dev_name = String(dev_name);

  SoftwareSerial BTSerial(2, 0); // RX | TX

  String ble_msg = "";
  pinMode(4, OUTPUT);    // this pin will change EN state
  digitalWrite(4, HIGH); //EN state make HIGH
  BTSerial.begin(9600);  // HC-06 default speed in AT command more

  //set name alias and pin
  String a_set_cmd = String("AT+NAME$siba_") + String(dev_name);
  BTSerial.print(a_set_cmd);
  delay(1000);

  String pin = String(BLUETOOTH_PIN);
  String p_set_cmd = "AT+PIN" + pin;
  BTSerial.print(p_set_cmd);
  delay(1000);

  //while(!BTSerial.available()){BTSerial.read();} //buffer clear

  BTSerial.flush();

  while (1)
  {
    if (BTSerial.available())
    {
      char temp = BTSerial.read();
      ble_msg += temp;
      if (temp == '}')
        break;
    }
    delay(200);
  }

  Serial.println();
  Serial.println(ble_msg);
  Serial.println(ble_msg.length());
  digitalWrite(4, LOW); //EN state make LOW

  StaticJsonDocument<256> doc;
  auto error = deserializeJson(doc, ble_msg);
  if (error)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return 0;
  }

  const char *ssid = doc["ssid"];
  const char *pwd = doc["pwd"];

  this->init(ssid, pwd, auth_key);

  return 1;
}

size_t SIBA::init(const char *ssid, const char *pwd, const char *dev_type)
{
  //static function내에서 멤버 함수를 실행 할 수 없으므로 트릭 사용
  context = *this;

  //instance field value init
  this->ssid = const_cast<char *>(ssid);
  this->pwd = const_cast<char *>(pwd);
  this->dev_type = const_cast<char *>(dev_type);

  //basic setup function call
  this->init_wifi(this->ssid, this->pwd);
  this->client.setServer(this->mqtt_server, this->mqtt_port);

#if defined(ESP8266) || defined(ESP32)
  std::function<void(char *, uint8_t *, unsigned int)> callback = SIBA::mqtt_callback;
#else
  void (*callback)(char *, uint8_t *, unsigned int);
  callback = SIBA::mqtt_callback;
#endif

  this->client.setCallback(callback);

  return 1;
}

void SIBA::init_wifi(char *ssid, char *pwd)
{

  delay(10);
  // We start by connecting to a WiFi network
  Serial.print(F("\nConnecting to "));
  Serial.println(ssid);

  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(F("."));
  }

  randomSeed(micros());

  //this->cur_ip = const_cast<char*>(WiFi.localIP().toString().c_str());
  //this->mac_address = const_cast<char*>(WiFi.macAddress().c_str());

  this->cur_ip = WiFi.localIP().toString();
  this->mac_address = WiFi.macAddress();

  Serial.println("");
  Serial.println(F("WiFi connected"));
  Serial.print(F("IP address: "));
  Serial.println(this->cur_ip);
  Serial.print(F("MAC address: "));
  Serial.println(this->mac_address);
}

size_t SIBA::add_event(int code, SB_ACTION)
{
  size_t ret = 0;

  //0번 코드는 특수한 코드, 단 한 번만 등록될 수 있음, 생성자에서 등륵됨
  //hw 개발자는 0번 코드를 등록할 수 없음.
  if (code == REGISTER_EVENT_CODE && action_cnt)
  {
    Serial.println(F("cannot register this code and event"));
    return ret;
  }
  if (ret = action_cnt != EVENT_COUNT - 1)
  { //액션 스토어의 범위 내라면
    action_store[action_cnt++] = {code, sb_action};
  }
  return ret;
}

// size_t func(size_t) 타입의 함수 포인터 반환 함수
size_t (*SIBA::grep_event(int code))(size_t, sb_dataset[2], size_t)
{
  //sequential search
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

size_t SIBA::exec_event(SB_ACTION, size_t before, sb_dataset d_wrap[2], size_t len)
{

  size_t ret = 0;

  if (ret = sb_action != NULL)
  {
    ret = sb_action(before, d_wrap, len); //액션 수행
  }
  else
  {
#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO)
    Serial.println(F("action is empty"));
#endif
  }

  return ret;
}

size_t SIBA::pub_result(size_t action_res, int type)
{
  StaticJsonDocument<256> doc;
  char buffer[256];

  JsonObject object = doc.to<JsonObject>();
  object[F("dev_mac")] = this->mac_address;
  object[F("t")] = type;

  if (action_res)
  {
    object[F("status")] = 200;
  }
  else
  {
    object[F("status")] = 500;
  }

  uint16_t n = serializeJson(doc, buffer);

  this->publish_topic(DEV_CTRL_END, buffer, n);
}

void SIBA::regist_dev()
{
  StaticJsonDocument<256> doc;
  char buffer[256];

  JsonObject object = doc.to<JsonObject>();
  Serial.println(this->mac_address);
  object[F("dev_mac")] = this->mac_address;
  object[F("dev_type")] = this->dev_type;

  uint16_t n = serializeJson(doc, buffer);

#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO)
  Serial.println(F("device registration request send to hub"));
#endif

  //허브에게 디바이스 등록 정보 전송
  this->publish_topic(DEV_REG, buffer, n);
}

void SIBA::subscribe_topic(char *topic)
{
#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO)
  Serial.println(F("subscribe topic."));
#endif
  this->client.subscribe(topic);
}

void SIBA::publish_topic(char *topic, char *buffer, uint16_t n)
{
  Serial.println(F("publish topic"));
  this->client.publish(topic, buffer, n);
}

void SIBA::mqtt_reconnect()
{
  // Loop until we're reconnected
  while (!this->client.connected())
  {
    Serial.print(F("Attempting MQTT connection..."));

    // Create a random client ID
    String clientId = "ESP8266Client-";
    //clientId += String(random(0xffff), HEX);
    clientId += this->mac_address;

    String willMessage = this->mac_address + "," + this->dev_type;

    // Attempt to connect
    if (this->client.connect(clientId.c_str(), DEV_WILL, 1, 1, willMessage.c_str()))
    {
      Serial.println(F("connected"));

      //토픽 subscribe
      String buf = String(DEV_CTRL) + "/" + this->mac_address;
      char *topic = const_cast<char *>(buf.c_str());
      this->subscribe_topic(topic);

      // 허브에게 장비 등록 요청
      this->regist_dev();
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

void SIBA::parse_call()
{
  //받아온 json문자열 파싱, 파싱 실패시 함수 수행 중단
  StaticJsonDocument<512> doc;
  auto error = deserializeJson(doc, this->msg);
  if (error)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return;
  }

  Serial.println(this->msg);

  //json deserialize 수행
  JsonArray cmd_list = doc[F("c")].as<JsonArray>();
  int code = -1;
  int type = 0;
  size_t action_result = 0;
  for (int i = cmd_list.size() - 1; i >= 0; i--)
  {
    JsonObject elem = cmd_list.getElement(i).as<JsonObject>();

    code = elem[F("e")];
    type = elem[F("t")];
    JsonArray dataset = elem[F("d")];

    sb_dataset d_wrap[2];
    for (size_t j = 0; j < dataset.size(); j++)
    {
      JsonObject d_arg = dataset.getElement(j).as<JsonObject>();
      d_wrap[j].type = d_arg[F("type")];
      d_wrap[j].value = d_arg[F("value")].as<String>();
    }

    SB_ACTION = context.grep_event(code);
    action_result = context.exec_event(sb_action, action_result, d_wrap, dataset.size());
  }
  //code가 -1번이 아니라면 허브에게 결과 전송
  if (code != -1)
  {
    context.pub_result(action_result, type);
  }
  else
  {
    //-1이라면 상태 값 생성 요청
    if (context.init_call != NULL)
    {
      context.init_call();
    };
  }
}

void SIBA::mqtt_callback(char *topic, uint8_t *payload, unsigned int length)
{
  size_t p_cnt = 0;
  size_t p_max_cnt = 0;
  char temp;

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
    char cur = (char)payload[i];
    if (p_max_cnt)
      context.msg += cur;

    if (cur == '/')
    {
      if (!p_cnt)
      {
        p_cnt = temp - 48;
      }
      else if (!p_max_cnt)
      {
        p_max_cnt = temp - 48;
      }
    }
    else
    {
      temp = cur;
    }
  }

  //마지막 패킷이라면 문자열 parse후 act 수행
  if (p_cnt == p_max_cnt)
  {
    context.parse_call();
    context.msg = "";
  }
}

void SIBA::verify_connection()
{
  if (!this->client.connected())
  {
    this->mqtt_reconnect();
  }
  this->client.loop();

  //등록이 됬다면
  if (context.is_reg && sensing_cnt)
  {
    context.internal_curr = millis();
    if (context.internal_curr - context.internal_prev > this->sensing_time)
    {
      Serial.println("sensing event occured");
      context.internal_prev = millis();
      this->act_sensing();
    }
  }
}

void SIBA::send_to_hub(char *key, sb_data temp, size_t type)
{
  StaticJsonDocument<256> doc;
  char buffer[256];

  JsonObject object = doc.to<JsonObject>();
  object[F("key")] = key;

  switch(type){
      case SB_BYTE:
        object[F("val")] = temp.sb_byte;
        break;
      case SB_INT:
        object[F("val")] = temp.sb_int;
        break;
      case SB_DOUBLE:
        object[F("val")] = temp.sb_double;
        break;
      case SB_CHAR:
        object[F("val")] = temp.sb_char;
        break;
      default:
        //object[F("val")] = temp.sb_string;
        break;
  }
  object[F("mac")] = this->mac_address;

  uint16_t n = serializeJson(doc, buffer);

  this->publish_topic(DEV_INIT_STATE, buffer, n);
}

int SIBA::find_state_idx(char* key){
  String temp = String(key);
  for(int i=0; i<state_cnt; i++){
    if(state_store[i].key==temp)
      return i;
  }
  return -1;
}

void SIBA::init_state_local(char *key, size_t type, sb_data temp){
  state_store[state_cnt++] = {key, type, temp};
}

void SIBA::set_state(char *data_key, byte value, size_t option=1)
{
  if(option){
    size_t idx = find_state_idx(data_key);
    if(idx!=-1){
      state_store[idx].value.sb_byte = value;
    }
  }
  sb_data temp = { value };
  this->send_to_hub(data_key, temp, SB_BYTE);
}

void SIBA::set_state(char *data_key, int value, size_t option=1)
{
  if(option){
    size_t idx = find_state_idx(data_key);
    if(idx!=-1){
      state_store[idx].value.sb_int = value;
    }
  }
  sb_data temp = { value };
  this->send_to_hub(data_key, temp, SB_INT);
}

void SIBA::set_state(char *data_key, double value, size_t option=1)
{
  if(option){
    size_t idx = find_state_idx(data_key);
    if(idx!=-1){
      state_store[idx].value.sb_double = value;
    }
  }
  sb_data temp = { value };
  this->send_to_hub(data_key, temp, SB_DOUBLE);
}

void SIBA::set_state(char *data_key, char value, size_t option=1)
{
  if(option){
    size_t idx = find_state_idx(data_key);
    if(idx!=-1){
      state_store[idx].value.sb_char = value;
    }
  }
  sb_data temp = { value };
  this->send_to_hub(data_key, temp, SB_CHAR);
}

void SIBA::init_state(char *key, byte value, size_t option=1)
{
  if(state_cnt != STATE_COUNT_LIMIT){
    sb_data temp = { value };
    if(option) this->init_state_local(key, SB_INT, temp);
    this->send_to_hub(key, temp, SB_BYTE);
  }
}

void SIBA::init_state(char *key, int value, size_t option=1)
{
  if(state_cnt != STATE_COUNT_LIMIT){
    sb_data temp = { value };
    if(option) this->init_state_local(key, SB_INT, temp);
    this->send_to_hub(key, temp, SB_INT);
  }
}

void SIBA::init_state(char *key, double value, size_t option=1)
{
  if(state_cnt != STATE_COUNT_LIMIT){
    sb_data temp = { value };
    if(option) this->init_state_local(key, SB_INT, temp);
    this->send_to_hub(key, temp, SB_DOUBLE);
  }
}

void SIBA::init_state(char *key, char value, size_t option=1)
{
  if(state_cnt != STATE_COUNT_LIMIT){
    sb_data temp = { value };
    if(option) this->init_state_local(key, SB_INT, temp);
    this->send_to_hub(key, temp, SB_CHAR);
  }
}

size_t SIBA::init_regist(INIT_GROUP)
{

  this->init_call = init_group;

  return 1;
}

void SIBA::add_sensing(String key, size_t type, sb_sensing_func func)
{
  if (this->sensing_cnt != SENSING_COUNT_LIMIT)
  {
    sensing_store[this->sensing_cnt++] = {
        key,
        type,
        func
    };
  }
}

void SIBA::reserve_sensing(char *key, RESERVE_BYTE)
{
  sb_sensing_func temp;
  temp.reserve_byte = {reserve_byte};
  this->add_sensing(key, SB_BYTE, temp);
}

void SIBA::reserve_sensing(char *key, RESERVE_INTEGER)
{
  sb_sensing_func temp;
  temp.reserve_int = {reserve_int};
  this->add_sensing(key, SB_INT, temp);
}

void SIBA::reserve_sensing(char *key, RESERVE_DOUBLE)
{
  sb_sensing_func temp;
  temp.reserve_double={reserve_double};
  this->add_sensing(key, SB_DOUBLE, temp);
}

void SIBA::reserve_sensing(char *key, RESERVE_CHAR)
{
  sb_sensing_func temp;
  temp.reserve_char={reserve_char};
  this->add_sensing(key, SB_CHAR, temp);
}

// void SIBA::reserve_sensing(char *key, RESERVE_STRING)
// {
//   sb_sensing_func temp;
//   temp.reserve_str={reserve_str};
//   this->add_sensing(key, SB_STRING, temp);
// }

void SIBA::act_sensing()
{
  for (int i = 0; i < this->sensing_cnt; i++)
  {
    StaticJsonDocument<256> doc;
    JsonObject object = doc.to<JsonObject>();
    object[F("mac")] = this->mac_address;
    object[F("key")] = sensing_store[i].key;
    char buffer[256];
    switch(sensing_store[i].type){
      case SB_BYTE:
        object[F("val")] = sensing_store[i].func.reserve_byte();
        break;
      case SB_INT:
        object[F("val")] = sensing_store[i].func.reserve_int();
        break;
      case SB_DOUBLE:
        object[F("val")] = sensing_store[i].func.reserve_double();
        break;
      case SB_CHAR:
        object[F("val")] = sensing_store[i].func.reserve_char();
        break;
      default:
        object[F("val")] = sensing_store[i].func.reserve_str();
        break;
    }
    uint16_t n = serializeJson(doc, buffer);
    this->publish_topic(DEV_SENSING_RECV, buffer, n);
  }
}

void SIBA::set_sensing_time(int time)
{
  //5s 밑으로는 설정 불가
  if (time >= 5000)
  {
    this->sensing_time = time;
  }
}