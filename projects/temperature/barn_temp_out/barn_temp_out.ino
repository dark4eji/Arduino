#define RELAY_LL 3
#define RELAY_RL 5
#define RELAY_GL 6
#define RELAY_HE 7
#define RELAY_SO 8

#define DHTPIN1 2
#define DHTPIN2 4

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

unsigned long timer;
unsigned long timer_tx;
unsigned long sync_timer;

int syncChecker = 0;
int dhtChecker = 1;

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

RF24 radio(9, 10);

DHT dht1(DHTPIN1, DHT11); //Инициация датчика
DHT dht2(DHTPIN2, DHT11); //Инициация датчика

struct Data {
  short id = 1;
  short data1;
  float data2; 
  float data3;
  float data4;
  float data5;
};

struct TXMessage {
  short id = 2; // id 1 = реле, id 2 = сарай, id 10 = синхронизатор
  short data1; // левая лампа
  short data2; // правая лампа
  short data3; // общий свет
  short data4; // нагрев
  short data5; // розетка  
  short data6; // бесполезная нагрузка от компрессора 
};

  short leftLamp = 1; //страховочные единицы на случай отключения света
  short rightLamp = 1;
  short generalLight = 0;
  short heater = 0; 
  short socket = 0;   

Data data;
TXMessage TXm;

int timerValue = 100;

void setup() { 
  Serial.begin(9600);
  
  timer = millis();
  timer_tx = millis();
  sync_timer = millis();

  initDevices();  
  setupRadio(); 
}

void loop() {
  float voltage = (float)(analogRead(0) * 5.0) / 1024;

  if (voltage < 1) {
    data.data1 = 1;
  } else {
    data.data1 = 0;
  }
  
  if (millis() - timer_tx >= 3000) {   
    processTXData();
    
    if (dhtChecker = 1) {
      getDHT(); 
    }
    
    timer_tx = millis();
  }       
    processRXData();
    setData();
    Serial.print("data.id: ");
    Serial.println(data.id);    
  
  manageRelays();
}

void processRXData() {
  if (radio.available()) {        
     radio.read(&TXm, sizeof(TXm));     
  }  
}

void processTXData() {
  radio.stopListening();
  radio.write(&data, sizeof(data)); 
  
  if (data.id != 1) {
    data.id = 1;
  }
  radio.startListening();
}

// Обработчик входящих данных для формирования ответа-синхронизатора, либо выставление значений для реле
void setData() { 
  syncChecker = 0; 
  if (TXm.id == 10) { 
    syncChecker = 1;
    dhtChecker = 0; 
    data.id = 10;   
    data.data1 = leftLamp;
    data.data2 = rightLamp;
    data.data3 = generalLight;
    data.data4 = heater;
    data.data5 = socket;
        
  }

  if (syncChecker == 1) {
    sync_timer = millis();
  } else if ((syncChecker == 0) && (millis() - sync_timer >= 5000) 
                                && TXm.id == 2) { 
    dhtChecker = 1;                                  
    leftLamp = TXm.data1;
    rightLamp = TXm.data2;
    generalLight = TXm.data3;
    heater = TXm.data4;
    socket = TXm.data5;    
  }
  
   Serial.print("leftLamp: ");
   Serial.println(leftLamp); 
   Serial.print("rightLamp: ");
   Serial.println(rightLamp); 
   Serial.print("generalLight: ");
   Serial.println(generalLight); 
   Serial.print("heater: ");
   Serial.println(heater); 
   Serial.print("socket: ");
   Serial.println(socket); 
   Serial.print("TXm.id: ");
   Serial.println(TXm.id); 
   TXm.id = 0;  
}

void manageRelays() {  
    if (leftLamp == 1) {
      digitalWrite(RELAY_LL, HIGH);
    } else {
      digitalWrite(RELAY_LL, LOW);
    }

    if (rightLamp == 1) {
      digitalWrite(RELAY_RL, HIGH);
    } else {
      digitalWrite(RELAY_RL, LOW);
    }  

    if (generalLight == 1) {
      digitalWrite(RELAY_GL, HIGH);
    } else {
      digitalWrite(RELAY_GL, LOW);
    }

    if (heater == 1) {
      digitalWrite(RELAY_HE, HIGH);
    } else {
      digitalWrite(RELAY_HE, LOW);
    } 

    if (socket == 1) {
      digitalWrite(RELAY_SO, HIGH);
    } else {
      digitalWrite(RELAY_SO, LOW);
    } 
}

void setupRadio() {
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(1, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setChannel(0x6b);
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.openWritingPipe(address[1]); 
  radio.openReadingPipe(1, address[2]);
  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  radio.powerUp(); //начать работу
  radio.stopListening();  //не слушаем радиоэфир, мы передатчик
}

void getDHT() {
  data.data2 = dht1.readTemperature() - 1;
  data.data3 = dht2.readTemperature() - 1;
  data.data4 = dht1.readHumidity();
  data.data5 = dht2.readHumidity();

   if (isnan(data.data4) || isnan(data.data2)) {  // Проверка. Если не удается считать показания, выводится «Ошибка считывания», и программа завершает работу
     data.data4 = 0;
     data.data2 = 0;
   }

  if (isnan(data.data5) || isnan(data.data3)) {
    data.data5 = 0;
    data.data3 = 0;
  }
}

void initDevices() {
  dht1.begin();
  dht2.begin();

  pinMode(RELAY_LL, OUTPUT);
  digitalWrite(RELAY_LL, LOW);

  pinMode(RELAY_RL, OUTPUT);
  digitalWrite(RELAY_RL, LOW);

  pinMode(RELAY_GL, OUTPUT);
  digitalWrite(RELAY_GL, LOW);

  pinMode(RELAY_HE, OUTPUT);
  digitalWrite(RELAY_HE, LOW);
  
  pinMode(RELAY_SO, OUTPUT);
  digitalWrite(RELAY_SO, LOW);
  
}
