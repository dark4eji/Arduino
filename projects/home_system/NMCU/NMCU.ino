#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(D4,D2);

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

unsigned long timer;

struct DataToPost {
  int b_tempUpperSensor;
  int b_tempLowerSensor;
  int m_rawWaterData;
};

struct DataToGet {
  const char* b_tempUpperSensor;
  const char* b_tempLowerSensor;  
};

DataToPost dataToPost;
DataToGet  dataToGet;

void setup(){
  timer = millis();
  Serial.begin(9600);
  setupRadio();
  
  WiFi.begin(ssid, pass); 
  while (WiFi.status() != WL_CONNECTED) {    
    delay(1000);
    Serial.println("Connecting...");    
  }
  timer1 = millis();   
}

void loop() {
  if (millis() - timer1 >= 2000) {
    sendDataToServer();
    Serial.println(dataToGet.b_tempUpperSensor);
    Serial.println(dataToGet.b_tempLowerSensor);
    timer1 = millis();
  }
  if (millis() - timer >= 500) {    
    if (radio.available()) {    
     radio.read(&data, sizeof(data));
        
     if (data.id == 2) {
      temp = data.temp;
      Serial.println("ID 2");
     } else if (data.id == 3) {
      Serial.println("ID 3");
      y = data.x;
     }       
     Serial.println(temp);
     Serial.println(y);    
     timer = millis();
  }
}
}

void sendDataToServer() {    
  if (WiFi.status() == WL_CONNECTED) {   
    HTTPClient http;    
    http.begin(url);
    http.addHeader("Content-Type", contentType);
    int httpCode = http.POST(buildJsonToPost());
                                                                        
    if (httpCode > 0) {      
      const size_t capacity = JSON_OBJECT_SIZE(2) + 310;
      DynamicJsonDocument docToGet(capacity);      
      DeserializationError error = deserializeJson(docToGet, http.getString());         
      if (error) {        
          Serial.print(("deserializeJson() failed: "));
          Serial.println(error.c_str());
          return;
        }        
      dataToGet.b_tempUpperSensor = docToGet["tempResult1"];
      dataToGet.b_tempLowerSensor = docToGet["tempResult2"];   
    }
    http.end();   //Close connection
  }
}

void setupRadio() { 
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(1, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setChannel(0x6b);
  radio.setPayloadSize(32);     //размер пакета, в байтах
  //radio.openReadingPipe(1, address[2]);
  radio.openReadingPipe(1, address[2]);
  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp(); //начать работу
  radio.startListening();  //не слушаем радиоэфир, мы передатчик
}

String buildJsonToPost() {
  String json;
  StaticJsonDocument<200> docToPost;
    
  docToPost["b_tempUpperSensor"] = dataToPost.b_tempUpperSensor;
  docToPost["b_tempLowerSensor"] = dataToPost.b_tempLowerSensor;
  
  serializeJson(docToPost, json);
  return json;
}
