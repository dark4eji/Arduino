#define PIN_TRIG 7
#define PIN_ECHO 8
#define TIME 180000

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(9,10);

unsigned long timer;

struct Data {
  byte id = 3;
  byte temp = 2;
  unsigned short x = 2000;
};

short temp = 2; 

Data data;

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};
 bool rslt;  
void setup(){
  timer = millis();
  Serial.begin(9600);
  setupRadio();
  
}

void loop() {
  
  if (millis() - timer >= 2000) {   
   
    rslt = radio.write(&data, sizeof(data));

    if (rslt) {
      Serial.println("Yesh");  
    } else {
      Serial.println("Nesh");
    }  
   timer = millis();
  }     
}

void setupRadio() { 
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(1, 15);    //(время между попыткой достучаться, число попыток)
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
