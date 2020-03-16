#define RELAY 7
#define CE 9
#define CSN 10

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(CE, CSN);

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

byte isCompressorActive = 0;

void setup(){
  Serial.begin(9600); //открываем порт для связи с ПК
  
  pinMode(RELAY, OUTPUT);  
  digitalWrite(RELAY, LOW);
     
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0,15);     //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.setChannel(0x6b);

  radio.openReadingPipe(1, address[0]);      //хотим слушать трубу 0
 
  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!
  
  radio.powerUp(); //начать работу
  radio.startListening();  //начинаем слушать эфир, мы приёмный модуль
}

void loop() {  
  if (radio.available()) {    
    radio.read(&isCompressorActive, sizeof(isCompressorActive));     
  }
  
  if (isCompressorActive == 1) {       
    digitalWrite(RELAY, HIGH);     
  } else {
    digitalWrite(RELAY, LOW);  
  }  
  Serial.println(isCompressorActive);    
}
