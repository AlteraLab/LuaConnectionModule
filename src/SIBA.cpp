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
sb_event SIBA::action_store[EVENT_COUNT] = {0};
size_t SIBA::is_reg = 0;
String SIBA::mac_address;

SIBA::SIBA()
{
  this->mqtt_server = MQTT_SERVER;
  this->mqtt_port = MQTT_PORT;

  this->espClient = WiFiClient();
  this->client = PubSubClient(espClient);
  this->is_reg = 0;
  this->msg = "";


  this->add_event(REGISTER_EVENT_CODE, SIBA::register_event); //디바이스가 등록됬을 때 수행 될 이벤트
}

size_t SIBA::register_event(size_t before, sb_dataset dwrap[2], size_t len)
{
  //디바이스 정상 등록 시 로그 출력 및 LED 점등
  Serial.println(F("device register is finished"));
  is_reg = 1;
  return is_reg;
}

size_t SIBA::init(const char* auth_key, const char* dev_name){

  SoftwareSerial BTSerial(2, 0); // RX | TX

  String ble_msg="";
  pinMode(4, OUTPUT);  // this pin will change EN state
  digitalWrite(4, HIGH);  //EN state make HIGH
  BTSerial.begin(9600);  // HC-06 default speed in AT command more
  
  //set name alias and pin
  String a_set_cmd = String("AT+NAME$siba_")+String(dev_name);
  BTSerial.print(a_set_cmd);
  delay(1000);
  
  String pin = String(BLUETOOTH_PIN);
  String p_set_cmd = "AT+PIN"+pin;
  BTSerial.print(p_set_cmd);
  delay(1000);

  while(!BTSerial.available()){BTSerial.read();} //buffer clear

  while(1){
    if(BTSerial.available()){
      char temp = BTSerial.read();
      ble_msg += temp;
      if(temp=='}')
        break;
    }
    delay(200);
  }

  Serial.println(ble_msg);
  digitalWrite(4, LOW); //EN state make LOW

  StaticJsonDocument<256> doc;
  auto error = deserializeJson(doc, ble_msg);
  if (error)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return 0;
  }

  const char* ssid = doc["ssid"];
  const char* pwd = doc["pwd"];

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
  else{
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

void SIBA::publish_topic(char *topic, char* buffer, uint16_t n)
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

void SIBA::parse_call(){
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
  size_t action_result=0;
  for (int i = cmd_list.size() - 1; i >= 0; i--)
  {
    JsonObject elem = cmd_list.getElement(i).as<JsonObject>();

    code = elem[F("e")];
    type = elem[F("t")];
    JsonArray dataset = elem[F("d")];

    sb_dataset d_wrap[2];
    for(size_t j=0; j<dataset.size(); j++){
      JsonObject d_arg = dataset.getElement(j).as<JsonObject>();
      d_wrap[j].type = d_arg[F("type")];
      d_wrap[j].value = d_arg[F("value")].as<String>();
    }

    SB_ACTION = context.grep_event(code);
    action_result = context.exec_event(sb_action, action_result, d_wrap, dataset.size());

  }
  //code가 -1번이 아니라면 허브에게 결과 전송
  if (code != -1){
    context.pub_result(action_result, type);
  }
  else{
    //-1이라면 상태 값 생성 요청
    if(context.init_call!=NULL){
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
    if(p_max_cnt)
      context.msg += cur;

    if(cur=='/'){
      if(!p_cnt){
        p_cnt = temp-48;  
      }
      else if(!p_max_cnt){
        p_max_cnt = temp-48;
      }  
    }
    else{
      temp =cur;
    }
  }

  //마지막 패킷이라면 문자열 parse후 act 수행
  if(p_cnt == p_max_cnt){
    context.parse_call();
    context.msg="";
  }
}

void SIBA::verify_connection()
{
  if (!this->client.connected())
  {
    this->mqtt_reconnect();
  }
  this->client.loop();
}

void SIBA::send_to_hub(char* data_key, int value, char* topic){
  StaticJsonDocument<256> doc;
  char buffer[256];

  JsonObject object = doc.to<JsonObject>();
  object[F("key")] = data_key;
  object[F("val")] = value;
  object[F("mac")] = this->mac_address;

  uint16_t n = serializeJson(doc, buffer);

  this->publish_topic(topic, buffer, n);
}

void SIBA::set_state(char* data_key, int value)
{
  this->send_to_hub(data_key, value, DEV_SET_STATE);
}

void SIBA::init_state(char* data_key, int value)
{
  Serial.println("init state-------");
  this->send_to_hub(data_key, value, DEV_INIT_STATE);
}

size_t SIBA::init_regist(INIT_GROUP){

  this->init_call = init_group;

  return 1;
}