#define BLYNK_PRINT Serial
#define TANK_HEIGHT 165

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(2,4);

WidgetLED led0(V0);  //Создание класса для светодиода
WidgetLED led3(V3);
WidgetLED led4(V4);

char auth[] = "3e883JyisvmmRpfE9RrWlw_Qoa-B0vCX";
char ssid[] = "RT-WiFi_95B8";
char pass[] = "9edt3JgT";

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};  //1Node - главный модуль; 2Node - модуль реле; 3Node - модуль сенсора воды

byte isCompressorActive = 0;  //Хранит 0 или 1 как состояние компрессора
byte pinCheck = 0;  //Хранит 0 или 1 как сигнал с кнопки Blynk


int wSensorData;  //Хранит сырые данные с сенсора


void setup(){
  Serial.begin(9600); //открываем порт для связи с ПК 
  Blynk.begin(auth, ssid, pass);  

  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    //(время между попыткой достучаться, число попыток)
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
  radio.stopListening();  //не слушаем радиоэфир, мы передатчик
}

void loop() { 
  Blynk.run();
  int waterLevel = TANK_HEIGHT - wSensorData;
  Blynk.virtualWrite(V2, waterLevel);  //Отправка данных на шкалу в Blynk
  Serial.println(wSensorData);
  Serial.println(waterLevel);
    
  if (waterLevel >= 130 && isCompressorActive == 1 && wSensorData != 0) {
    Blynk.virtualWrite(V1, "offLabel");    
    pinCheck = 0;    
  }  
  processTXData(); 
  processRXData();
}

//--------------------------------------
//Отправка управляющего сигнала на реле
void processTXData() {
  if (isCompressorActive != pinCheck) {             
      radio.stopListening();
      isCompressorActive = pinCheck;         
      Serial.println(isCompressorActive);  
      radio.write(&isCompressorActive, sizeof(isCompressorActive));
  }     
}
//--------------------------------------
//Прием сырых данных уровня воды
void processRXData(){  
  radio.startListening();
    if (radio.available()) {
       radio.read(&wSensorData, sizeof(wSensorData));      
    }
  Serial.println(wSensorData);     
}

//--------------------------------------
//--------------------------------------
//ФУНКЦИИ BLYNK
//Бинд кнопки включения реле
BLYNK_WRITE(V1) {
  pinCheck = param.asInt();
    if (pinCheck == 1) {
      led0.on();
    } else {
      led0.off();
    } 
}
