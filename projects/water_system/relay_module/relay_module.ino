#define RELAY 7
#define CE 9
#define CSN 10

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(CE, CSN);

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

short compressor = 0;
short relayHealthcheck = 10;

unsigned long timer_ttl;
unsigned long timer_general;
unsigned long timer_healthcheck;

struct TXMessage {
  short id = 1; // id 1 = реле, id 2 = сарай, id 10 = синхронизатор
  short data1; // левая лампа
  short data2; // правая лампа
  short data3; // общий свет
  short data4; // нагрев
  short data5; // розетка  
  short data6; // бесполезная нагрузка от компрессора 
};

struct Data {
  short id = 3;
  short data1 = 0;
  float data2 = 0;
  float data3 = 0;
  float data4 = 0;
  float data5 = 0;
};

Data data;
TXMessage TXm;

void setup(){
  Serial.begin(9600); //открываем порт для связи с ПК
  
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
  
  setupRadio();
  
  timer_ttl = millis();
  timer_general = millis();
  timer_healthcheck = millis();
}

void loop() {       
  
  processRXData();  
    
  if (millis() - timer_healthcheck >= 2049) { 
    processTXData();    
    timer_healthcheck = millis();
  }  
  
  if (TXm.data6 == 1) {
    digitalWrite(RELAY, HIGH);
  } else {
    digitalWrite(RELAY, LOW);
  } 
       
    Serial.println(TXm.data6);    
}

void processRXData() {  
  if (radio.available()) {
      radio.read(&TXm, sizeof(TXm));            
      timer_ttl = millis();
    } else if (millis() - timer_ttl >= 20000) {  
      TXm.data6 = 0;
      timer_ttl = millis();     
    }
}

void processTXData() {
 radio.stopListening();
 radio.write(&data, sizeof(data));
 radio.startListening();
}

void setupRadio() {
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(1, 15);     //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.setChannel(0x6b);
  radio.openWritingPipe(address[1]);
  radio.openReadingPipe(1, address[2]);
  radio.setPALevel (RF24_PA_MAX);
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS,
  radio.powerUp(); //начать работу
  radio.startListening();  //начинаем слушать эфир, мы приёмный модуль
}
