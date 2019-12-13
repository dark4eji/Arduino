#define PIN_TRIG 7
#define PIN_ECHO 8
#define TIME 180000

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(9,10);

unsigned long timer;

int start;

int rawData;
int currentData;
int prevData;

int lesserValue;
int higherValue;

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

void setup(){
  Serial.begin(9600);
  setupRadio();
  start = 0;
  timer = millis();
}

void loop() {   
    if (start == 0) {
      Serial.println("Clibarating...");      
      calibrateSensor();      
      prevData = currentData;
      start = 1;
      setStraightData(currentData);
      Serial.println("Calibrated!");      
    } else {        
        if (currentData <= higherValue && currentData >= lesserValue) {        
          processData(currentData);
          Serial.println(currentData);
       }      
    } 
    
    if (millis() - timer >= TIME) {
      if (prevData == currentData) {
        start = 0;        
      }
      timer = millis();
    }    
    delay(1000);  
}

void processData(int dataToSend) {            
   radio.write(&dataToSend, sizeof(dataToSend));   
}

void calibrateSensor() {
  for (int i = 0; i <= 10; i++) {
    currentData = processSensor();
    delay(1500);
  }
}

int processSensor() {  
  // для большей точности установим значение LOW на пине Trig
  digitalWrite(PIN_TRIG, LOW); 
  delayMicroseconds(2); 
  // Теперь установим высокий уровень на пине Trig
  digitalWrite(PIN_TRIG, HIGH);
  // Подождем 10 μs 
  delayMicroseconds(10); 
  digitalWrite(PIN_TRIG, LOW); 
  // Узнаем длительность высокого сигнала на пине Echo
  int duration = pulseIn(PIN_ECHO, HIGH); 
  return duration / 58;
  // Выведем значение в Serial Monitor
}

void setStraightData(int rawData){
   higherValue = prevData + 1;
   lesserValue = prevData - 1;   
}

void setupRadio() {
  Serial.begin(9600); //открываем порт для связи с ПК
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setChannel(0x6b);
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.openWritingPipe(address[2]);   //мы - труба 0, открываем канал для передачи данных

  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp(); //начать работу
  radio.stopListening();  //не слушаем радиоэфир, мы передатчик
}
