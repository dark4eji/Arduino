#define BLYNK_PRINT Serial
#define TANK_HEIGHT 165
#define FULL 130
#define MIDDLE 82
#define LITTLE 41

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(D3, D0);

WidgetLED led0(V0);  //Создание класса для светодиода

char auth[] = "3e883JyisvmmRpfE9RrWlw_Qoa-B0vCX";
char ssid[] = "RT-WiFi_95B8";
char pass[] = "9edt3JgT";

char buffer[20];

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};  //1Node - главный модуль; 2Node - модуль реле; 3Node - модуль сенсора воды

byte isCompressorActive = 0;  //Хранит 0 или 1 как состояние компрессора
byte pinCheck = 0;  //Хранит 0 или 1 как сигнал с кнопки Blynk

int wSensorData;  //Хранит сырые данные с сенсора

float scale = 0;

struct SFlags {
  byte full;
  byte half;
  byte low;
  byte clearFlag;
};

SFlags flags = {0, 0, 0, 0};

LiquidCrystal_I2C lcd(0x26, 20, 4);

void setup(){ 
  lcd.init();
  // включаем подсветку
  lcd.backlight();
  // устанавливаем курсор в колонку 0, строку 0
  lcd.setCursor(0, 0);
  // печатаем первую строку
  lcd.print("Starting...");  
       
  Serial.begin(9600); //открываем порт для связи с ПК    
  Blynk.begin(auth, ssid, pass);  
  setupRadio();
   
}

void loop() {   
  Blynk.run();  
  int waterLevel = TANK_HEIGHT - wSensorData;  
  notifyLevel(waterLevel);  
  Blynk.virtualWrite(V3, scale); 
  Blynk.virtualWrite(V2, waterLevel);  //Отправка данных на шкалу в Blynk
 
  lcd.setCursor(0, 0);
  lcd.print(String("Water level: ") + String(waterLevel));
  
  Serial.println(String(waterLevel));
  
  lcd.setCursor(0, 1);
  lcd.print(String("Scale level: ") + String(scale));  
  
  String compr;
  if (isCompressorActive == 0) {
    flags.clearFlag = 0;    
    compr = "Off";
  } else if ((flags.clearFlag == 0) && (isCompressorActive == 1)) {
    flags.clearFlag = 1;
    lcd.clear();   
    compr = "On";    
  }
  
  lcd.setCursor(0, 2);  
  lcd.print(String("Compr. state: ") + compr);
    
  Serial.println(wSensorData);
  Serial.println(waterLevel);
  Serial.println(scale);
    
  if (waterLevel >= FULL && isCompressorActive == 1 && wSensorData != 0) {
    Blynk.virtualWrite(V1, "offLabel");    
    pinCheck = 0;    
  }  
  processTXData(); 
  processRXData();
  
  if ((waterLevel == TANK_HEIGHT) || (waterLevel < 0)) {
    delay(3000);
    lcd.clear();
  }  
}

//--------------------------------------
//Отправка управляющего сигнала на реле
void processTXData() {
  if (isCompressorActive != pinCheck) {             
      radio.stopListening();
      isCompressorActive = pinCheck;         
      Serial.println(isCompressorActive);  
      radio.write(&isCompressorActive, sizeof(isCompressorActive));
  }     
}
//--------------------------------------
//Прием сырых данных уровня воды
void processRXData(){  
  radio.startListening();
    if (radio.available()) {
       radio.read(&wSensorData, sizeof(wSensorData));      
    }
  Serial.println(wSensorData);     
}

//--------------------------------------
void notifyLevel(int waterLevel) {  
  if (waterLevel >= FULL && flags.full != 1) {
    flags.full = 1;
    Blynk.notify("!ВОДА! -- Бак полон");   
  } else if (waterLevel == (FULL - 4)) {
    flags.full = 0;    
  }

  if (waterLevel == MIDDLE && flags.half != 1) {
    flags.half = 1;
    Blynk.notify("!ВОДА! -- Пол бака");        
  } else if (waterLevel == (MIDDLE - 4)) {
    flags.half = 0;   
  } else if ((waterLevel == (MIDDLE + 4)) && (flags.half == 1)) {
    flags.half = 0;
  }

   if ((waterLevel == LITTLE) && (flags.low != 1)) {
    flags.low = 1;
    Blynk.notify("!ВОДА! -- !НИЗКИЙ УРОВЕНЬ ВОДЫ!");   
  } else if ((waterLevel == (LITTLE + 4)) && (flags.low == 1)) {
    flags.low = 0;
  } 
  checkScale(waterLevel);
}

//--------------------------------------
void checkScale(int waterLevel) { 
  if (waterLevel == 138) {
    scale = 9;
  } else if (waterLevel == 129) {
    scale = 8.5;
  } else if (waterLevel == 124) {
    scale = 7;
  } else if (waterLevel == 116) {
    scale = 7.5;
  } else if (waterLevel == 110) {
    scale = 7;
  } else if (waterLevel == 102) {
    scale = 6.5;
  } else if (waterLevel == 97) {
    scale = 6;
  } else if (waterLevel == 89) {
    scale = 5.5;
  } else if (waterLevel == 82) {
    scale = 5;
  } else if (waterLevel == 75) {
    scale = 4.5;
  } else if (waterLevel == 70) {
    scale = 4;
  } else if (waterLevel == 68) {
    scale = 4;
  } else if (waterLevel == 61) {
    scale = 3.5;
  } else if (waterLevel == 56) {
    scale = 3;
  } else if (waterLevel == 47) {
    scale = 2.5;
  } else if (waterLevel == 42) {
    scale = 2;
  } else {
    scale = scale;
  }    
}

void setupRadio() {
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.setChannel(0x6b);
  radio.openWritingPipe(address[0]);   //мы - труба 0, открываем канал для передачи данных
  radio.openReadingPipe(1, address[2]);

  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp(); //начать работу
  radio.stopListening();  //не слушаем радиоэфир, мы передатчик
  Serial.println("CONFGURED");
  delay(2000);
}

//--------------------------------------
//--------------------------------------
//ФУНКЦИИ BLYNK
//Бинд кнопки включения реле
BLYNK_WRITE(V1) {
  pinCheck = param.asInt();
    if (pinCheck == 1) {
      led0.on();
    } else {
      led0.off();
    } 
}
