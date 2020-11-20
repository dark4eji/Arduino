#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio              (9, 10);
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

int x = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  setupRadio();  

}

void loop() {
  // put your main code here, to run repeatedly: 
    
      radio.read(&x, sizeof(x));     
  

    Serial.println(x);
}

void setupRadio() {
  radio.begin             (); //активировать модуль
  radio.setAutoAck        (1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries        (1, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload  ();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize    (32);     //размер пакета, в байтах
  radio.setChannel        (0x5f);
  radio.openWritingPipe   (address[3]);   //мы - труба 0, открываем канал для передачи данных
  radio.openReadingPipe   (1, address[4]);
  radio.setPALevel        (RF24_PA_MAX);
  radio.setDataRate       (RF24_250KBPS);
  radio.powerUp           (); //начать работу
  radio.startListening    ();  //не слушаем радиоэфир, мы передатчик
}
