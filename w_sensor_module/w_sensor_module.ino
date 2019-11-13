#define PIN_TRIG 7
#define PIN_ECHO 6

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(9,10);

int sensorTime = 2;
int sensorMsTime = 2 * 1000;

bool checker = false;

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};
void setup(){
  Serial.begin(9600); //открываем порт для связи с ПК
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);  

  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
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
    
    int sensorData = processSensor();
    processData(sensorData);    
    delay(sensorMsTime);  
}

void processData(int data) {
   int sensorData = data;            
   radio.write(&data, sizeof(data));
}

int processSensor() {
   int duration, distance;
  // для большей точности установим значение LOW на пине Trig
  digitalWrite(PIN_TRIG, LOW); 
  delayMicroseconds(2); 
  // Теперь установим высокий уровень на пине Trig
  digitalWrite(PIN_TRIG, HIGH);
  // Подождем 10 μs 
  delayMicroseconds(10); 
  digitalWrite(PIN_TRIG, LOW); 
  // Узнаем длительность высокого сигнала на пине Echo
  duration = pulseIn(PIN_ECHO, HIGH);
  Serial.println(duration); 
  // Рассчитаем расстояние
  return duration / 58;
  // Выведем значение в Serial Monitor
}
