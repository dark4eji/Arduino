#define RELAY 7
#define CE 9
#define CSN 10

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(CE, CSN);

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

byte compressor = 0;

unsigned long timer_ttl;
unsigned long timer_general;

void setup(){
  Serial.begin(9600); //открываем порт для связи с ПК
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
  setupRadio();
  timer_ttl = millis();
  timer_general = millis();
}

void loop() {
  if (millis() - timer_general >= 10) {
    if (radio.available()) {
      radio.read(&compressor, sizeof(compressor));
      timer_ttl = millis();
    } else {
      if (millis() - timer_ttl >= 10000) {
        compressor = 0;
        timer_ttl = millis();
      }
    }

    if (compressor == 1) {
      digitalWrite(RELAY, HIGH);
    } else {
      digitalWrite(RELAY, LOW);
    }
    Serial.println(compressor);
  }
}

void setupRadio() {
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(1, 15);     //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.setChannel(0x6b);
  radio.openReadingPipe(1, address[2]);
  radio.setPALevel (RF24_PA_MAX);
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS,
  radio.powerUp(); //начать работу
  radio.startListening();  //начинаем слушать эфир, мы приёмный модуль
}
