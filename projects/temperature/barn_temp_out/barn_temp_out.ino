#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

#define DHTPIN1 2
#define DHTPIN2 4

unsigned long timer;
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

RF24 radio(9, 10);

DHT dht1(DHTPIN1, DHT11); //Инициация датчика
DHT dht2(DHTPIN2, DHT11); //Инициация датчика

/*
struct Data {
  float humidity1;
  float temperature1;
  float humidity2;
  float temperature2;
};
*/

struct Data {
  short id = 1;
  short data1;
  float data2;
  float data3;
  float data4;
  float data5;
};

Data data;

void setup() {
  Serial.begin(9600);
  timer = millis();
  setupRadio();
  dht1.begin();
  dht2.begin();
}

void loop() {
  if (millis() - timer >= 2000) {
    getDHT();
    radio.write(&data, sizeof(data));
    timer = millis();
  }
}

void getDHT() {
  data.data2 = dht1.readTemperature() - 1;
  data.data3 = dht2.readTemperature();
  data.data4 = dht1.readHumidity();
  data.data5 = dht2.readHumidity();

  /*
  data.humidity1 = dht1.readHumidity(); //Измеряем влажность
  data.temperature1 = dht1.readTemperature() - 1; //Измеряем температуру
  data.humidity2 = dht2.readHumidity(); //Измеряем влажность
  data.temperature2 = dht2.readTemperature(); //Измеряем температуру
   */

   if (isnan(data.data4) || isnan(data.data2)) {  // Проверка. Если не удается считать показания, выводится «Ошибка считывания», и программа завершает работу
     data.data4 = 0;
     data.data2 = 0;
   }

   /*
  if (isnan(data.humidity1) || isnan(data.temperature1)) {  // Проверка. Если не удается считать показания, выводится «Ошибка считывания», и программа завершает работу
    data.humidity1 = 0;
    data.temperature1 = 0;
  }
*/

  if (isnan(data.data5) || isnan(data.data3)) {
    data.data5 = 0;
    data.data3 = 0;
  }
}
  /*if (isnan(data.humidity2) || isnan(data.temperature2)) {
    data.humidity2 = 0;
    data.temperature2 = 0;
  }
  */


void setupRadio() {
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(1, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setChannel(0x6b);
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.openWritingPipe(address[1]);   //мы - труба 0, открываем канал для передачи данных
  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  radio.powerUp(); //начать работу
  radio.stopListening();  //не слушаем радиоэфир, мы передатчик
}
