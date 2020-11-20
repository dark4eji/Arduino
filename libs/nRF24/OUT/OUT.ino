#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio              (D4, D2);
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

int x = 31000;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  setupRadio();  

}

void loop() {
  // put your main code here, to run repeatedly: 
  radio.write(&x, sizeof(x));
  
    Serial.println(x);
}

void setupRadio() {
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(1, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setChannel(0x5f);
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.openWritingPipe(address[4]); 
  radio.openReadingPipe(1, address[3]);
  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  radio.powerUp(); //начать работу
  radio.stopListening();  //не слушаем радиоэфир, мы передатчик
}
