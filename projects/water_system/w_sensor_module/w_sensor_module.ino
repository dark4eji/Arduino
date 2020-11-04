#define PIN_TRIG 7
#define PIN_ECHO 8
#define CE 9
#define CSN 10
#define TIME 180000

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(CE, CSN);

unsigned long timer;

byte sendFlag;
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

struct Data {
  short id = 2; // id 1 = модуль температуры в сарае, id 2 = модуль воды
  short data1;
  float data2;
  float data3;
  float data4;
  float data5;
};

Data data;
void setup(){
  Serial.begin(9600);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  setupRadio();
  timer = millis();
}

void loop() {
  if (millis() - timer >= 1000) {    
    short val = processSensor();
    data.data1 = val;
    
    if (data.data1 > 300) {
      data.data1 = -1;
    }
    
    Serial.println(data.data1);
    sendFlag = radio.write(&data, sizeof(data));
    Serial.println(sendFlag);
    timer = millis();
  }
}

//void calibrateSensor() {
//  Serial.println("Clibarating...");
//  for (int i = 0; i <= 10; i++) {
//    dataToSend = processSensor();
//    delay(1000);
//  }
//  Serial.println("Calibrated!");
//}

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

void setupRadio() {
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(1, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setChannel(0x6b);
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.openWritingPipe(address[1]);   //мы - труба 0, открываем канал для передачи данных
  radio.setPALevel (RF24_PA_MAX);
  radio.setDataRate (RF24_250KBPS);
  radio.powerUp(); //начать работу
  radio.stopListening();  //не слушаем радиоэфир, мы передатчик
}
