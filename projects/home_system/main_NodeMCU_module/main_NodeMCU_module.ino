#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>
#include <BlynkSimpleEsp8266.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(D4, D2);
HTTPClient http;

// WiFi Parameters
char auth[] = "3e883JyisvmmRpfE9RrWlw_Qoa-B0vCX";
char ssid[] = "RT-WiFi_95B8";
char pass[] = "9edt3JgT";

String url = "http://192.168.0.18:8080/data";
String contentType = "application/json";

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

unsigned long timer_rxtx;
unsigned long timer_json;

struct OutData {
  float outTempUpperSensor;
  float outTempLowerSensor;
  float outHumUpperSensor;
  float outHumLowerSensor;
  short outRawWaterData;
  byte blynkButtonState;
  byte moduleState[3];
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
  byte stateFlag = 0;
  short ledState[3];
};

struct Data {
  short id; // id 1 = модуль температуры в сарае, id 2 = модуль воды
  short data1;
  float data2;
  float data3;
  float data4;
  float data5;
};

OutData outData;
InData  inData;
Data data;

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);

  setupRadio();

  timer_json = millis();
  timer_rxtx = millis();
}

void loop() {
  Blynk.run();

  if (millis() - timer_rxtx >= 500) {
    processTXData();
    processRXData();
    timer_rxtx = millis();
  }

  if (millis() - timer_json >= 200) {
    sendDataToServer();
    setBlynk();
    if (inData.waterNotifyFlag == 1) {
      Blynk.notify(inData.waterNotificationMessage);
    }
    timer_json = millis();
  }
}

void sendDataToServer() {
    http.begin(url);
    http.addHeader("Content-Type", contentType);
    int httpCode = http.POST(buildJsonToPost());  //Для POST добавить аргумент buildJsonToPost()
    if (httpCode > 0) {
        buildJsonToGet();
    }
    http.end(); //Close connection
    outData.moduleState[0] = 0;
    outData.moduleState[1] = 0;
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
  inData.ledState[0]              = docToGet["ledState"][0];
  inData.ledState[1]              = docToGet["ledState"][1];
  inData.ledState[2]              = docToGet["ledState"][2];
}

void processRXData(){
  radio.startListening();
    if (radio.available()) {
       radio.read(&data, sizeof(data));
       Serial.println(data.id);
       Serial.println(data.data1);
       if (data.id == 1) { // модуль температуры
           outData.outTempUpperSensor = data.data2;
           outData.outTempLowerSensor = data.data3;
           outData.outHumUpperSensor =  data.data4;
           outData.outHumLowerSensor =  data.data5;
           outData.moduleState[0] = 1;
       } else if (data.id == 2) {
          outData.outRawWaterData = data.data1;
          outData.moduleState[1] = 1;
       }
    } else {
      Serial.println("NA");
    }
}

void processTXData() {
    radio.stopListening();
    outData.moduleState[2] = radio.write(&inData.stateFlag, sizeof(inData.stateFlag));
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

void setupRadio() {
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(1, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.setChannel(0x6b);
  radio.openWritingPipe(address[0]);   //мы - труба 0, открываем канал для передачи данных
  radio.openReadingPipe(1, address[1]);
  radio.setPALevel (RF24_PA_MAX);
  radio.setDataRate (RF24_250KBPS);
  radio.powerUp(); //начать работу
  radio.startListening();  //не слушаем радиоэфир, мы передатчик
}
