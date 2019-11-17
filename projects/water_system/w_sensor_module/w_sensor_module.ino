#define PIN_TRIG 7
#define PIN_ECHO 8

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(9,10);

int clockTime = 0;

int start = 0;

int arithmetic[10];
int preciseArithmetic[10];

int rawData;
int clearData;

int lesserValue;
int higherValue;

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};
void setup(){
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

void loop() {   
    if (start == 0) {      
      calibrateSensor();
      start = 1;
    } else {     
      if (clockTime <= 10) {
        rawData = processSensor();
        setStraightData(rawData);      
        if (rawData <= higherValue && rawData >= lesserValue) {
          clearData = rawData;
          arithmetic[clockTime] = sensorData;
          clockTime++; 
        }           
      } else if (clockTime == 11) {       
        Serial.println(dataToSend);    
        processData(returnMeasures(arithmetic));      
        clockTime = 0;       
      }      
    } 
    delay(200);  
}

void processData(int dataToSend) {            
   radio.write(&dataToSend, sizeof(dataToSend));   
}

void calibrateSensor() {
  for (int i = 0; i <= 10; i++) {
    clearData = processSensor();
    delay(1000);
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

int returnMeasures(int arith[]) {
  int sum = 0;
  for (int i = 0; i <= 9; i++) {
    sum += arith[i];
  }
  return sum * 0.1;
}

void setStraightData(int rawData){
   higherValue = clearData + 1;
   lesserValue = clearData - 1;   
}

//int getPercentageValue(float coeff, int sign) {
//    int value;
//    if (sign == 0) {
//      value = clearData - (clearData * coeff);
//    } else {
//      value = clearData + (clearData * coeff);
//    }
//    return value;
//}
//
//void setPercVars(float coeffPos, float coeffNeg) {
//   percentagePos = getPercentageValue(coeffPos, 1);
//   percentageNeg = getPercentageValue(coeffNeg, 0);
//}
//
//void setGeneralPercValues(int sensorData) {    
//    if ((sensorData >= 10) && (sensorData <= 50)) {
//      setPercVars(0.10, 0.10);       
//  } else if ((sensorData >= 51) && (sensorData <= 80)) {
//      setPercVars(0.05, 0.02);
//  } else if ((sensorData >= 81) && (sensorData <= 180)) {
//      setPercVars(0.03, 0.02);
//  }
//}
