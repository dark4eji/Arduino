#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>
#include <BlynkSimpleEsp8266.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(D3, D0);
HTTPClient http;

// WiFi Parameters
char auth[] = "3e883JyisvmmRpfE9RrWlw_Qoa-B0vCX";
char ssid[] = "TP-LINK_E196";
char pass[] = "5bLf7678a4_V1a";

String url = "http://192.168.0.105:8080/data";
String contentType = "application/json";

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

unsigned long timer1;

struct OutData {
  float outTempUpperSensor;
  float outTempLowerSensor;
  float outHumUpperSensor;
  float outHumLowerSensor;
  short outRawWaterData;
  byte blynkButtonState;  
};

struct InData {
  float inTempUpperSensor;
  float inTempLowerSensor;
  float inHumUpperSensor;
  float intHumLowerSensor;
  short actualWaterLevel;
  float scale;
  byte waterNotifyFlag;
  const char* waterNotificationMessage;
  byte stateFlag;
  byte relayPermission; //флаг
  short ledState[3];
};

struct SensorData {
  byte id;
  float data1;
  float data2;
  float data3;
  float data4;
  short data5; 
};

OutData outData;
InData  inData;
SensorData sData;

void setup() {  
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass); 
  
  ArduinoOTA.setHostname("NodeMCU-1");
  ArduinoOTA.begin();

  timer1 = millis();
}

void loop() {
  ArduinoOTA.handle();
  Blynk.run();
  
  processRXData();
     
  if (inData.relayPermission == 1) {
      processTXData();
  }
    
  if (millis() - timer1 >= 200) {
    sendDataToServer();
    setBlynk();
    
    if (inData.waterNotifyFlag == 1) {
      Blynk.notify(inData.waterNotificationMessage);
    } 
    
    timer1 = millis();
  }  
}

void sendDataToServer() {
    http.begin(url);
    http.addHeader("Content-Type", contentType);
    int httpCode = http.GET();  //Для POST добавить аргумент buildJsonToPost()
    if (httpCode > 0) {
        buildJsonToGet();
    }
    http.end();   //Close connection
}

String buildJsonToPost() {
  const size_t capacity = JSON_OBJECT_SIZE(7) + 130;
  String json;
  DynamicJsonDocument docToPost(capacity);
    
  docToPost["outTempUpperSensor"] = outData.outTempUpperSensor;
  docToPost["outTempLowerSensor"] = outData.outTempLowerSensor;
  docToPost["outHumUpperSensor"]  = outData.outHumUpperSensor;
  docToPost["outHumLowerSensor"]  = outData.outHumLowerSensor;
  docToPost["outRawWaterData"]    = outData.outRawWaterData;
  docToPost["blynkButtonState"]   = outData.blynkButtonState;
  
  serializeJson(docToPost, json);
  return json;
}

void buildJsonToGet() {
  const size_t capacity = JSON_ARRAY_SIZE(3) + JSON_OBJECT_SIZE(10) + 220;
  DynamicJsonDocument docToGet(capacity);      
  DeserializationError error = deserializeJson(docToGet, http.getString());         
  if (error) {        
      Serial.print(("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }    
        
  inData.inTempUpperSensor        = docToGet["inTempUpperSensor"];
  inData.inTempLowerSensor        = docToGet["inTempLowerSensor"];
  inData.inHumUpperSensor         = docToGet["inHumUpperSensor"];
  inData.intHumLowerSensor        = docToGet["intHumLowerSensor"];
  inData.actualWaterLevel         = docToGet["actualWaterLevel"];
  inData.scale                    = docToGet["scale"];
  inData.waterNotifyFlag          = docToGet["waterNotifyFlag"];
  inData.waterNotificationMessage = docToGet["waterNotificationMessage"];
  inData.stateFlag                = docToGet["stateFlag"]; 
  inData.relayPermission          = docToGet["relayPermission"]; 
  inData.ledState[0]              = docToGet["ledState"][0];
  inData.ledState[1]              = docToGet["ledState"][1];
  inData.ledState[2]              = docToGet["ledState"][2];
}

void setupRadio() { 
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(1, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.setChannel(0x6b);
  radio.openWritingPipe(address[0]);   //мы - труба 0, открываем канал для передачи данных
  radio.openReadingPipe(1, address[2]);

  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp(); //начать работу
  radio.startListening();  //не слушаем радиоэфир, мы передатчик   
}

void processRXData(){  
  radio.startListening();
    if (radio.available()) {
       radio.read(&sData, sizeof(sData));
       if (sData.id == 1) {
           outData.outTempUpperSensor = sData.data1;
           outData.outTempLowerSensor = sData.data2;
           outData.outHumUpperSensor =  sData.data3;
           outData.outHumLowerSensor =  sData.data4;
       } else if (sData.id == 2) {
           outData.outRawWaterData = sData.data5; 
       }
    }
}

void processTXData() {                 
    radio.stopListening();
    radio.write(&inData.stateFlag, sizeof(inData.stateFlag));
}

void setBlynk() {
  Blynk.virtualWrite(V5, inData.scale); 
  Blynk.virtualWrite(V6, inData.actualWaterLevel);
  Blynk.virtualWrite(V7, inData.stateFlag); 
  Blynk.virtualWrite(V9, inData.ledState[0]);
}

BLYNK_WRITE(V7) {
  outData.blynkButtonState = param.asInt();
}
