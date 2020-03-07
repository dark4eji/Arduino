#define BLYNK_PRINT Serial
#define TANK_HEIGHT 165
#define FULL 130
#define MIDDLE 82
#define LITTLE 41
#define BUTTON D4
#define GREEN     "#23C48E"
#define RED       "#D3435C"

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(D3, D0);
WidgetLED led0(V0);  //Создание класса для светодиода
WidgetLED ledRelay(V4);
WidgetLED ledWSensor(V5);

char auth[] = "3e883JyisvmmRpfE9RrWlw_Qoa-B0vCX";
char ssid[] = "RT-WiFi_95B8";
char pass[] = "9edt3JgT";

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};  //1Node - главный модуль; 2Node - модуль реле; 3Node - модуль сенсора воды

byte isCompressorActive = 0;  //Хранит 0 или 1 как состояние компрессора; зависит от pinCheck
byte pinCheck = 0;  //Хранит 0 или 1 как сигнал с кнопки Blynk или обычной кнопки

int wSensorData;  //Хранит сырые данные с сенсора
int waterLevel;

float scale = 0;

unsigned long timer1;
unsigned long timer2;
unsigned long timer3;
unsigned long timer4;
unsigned long timer5;

int period = 120000;

struct Chicken {
  float temp1;
  float temp2;
};

struct Data {
  int id;
  int data1;
  int data2;
};

struct SFlags {
  byte full;
  byte half;
  byte low;
  byte pressButFlag; 
  byte clearFlag;
};

SFlags flags = {0, 0, 0, 0, 0};
Data data;
Chicken chicken;

LiquidCrystal_I2C lcd(0x26, 20, 4);

void setup(){  
  initDisplay();       
  Serial.begin(9600); //открываем порт для связи с ПК    
  Blynk.begin(auth, ssid, pass);  
  setupRadio();
  
  timer1 = millis();
  timer2 = millis(); 
  timer3 = millis();
  timer4 = millis();
  timer5 = millis();    
  
  ledRelay.on();
  ledWSensor.on();  
}

void loop() {

   if (millis() - timer5 >= 500) {
    relayHealthcheck();    
    operateLcd(waterLevel);
    setLcdCopressorState(waterLevel);   
    timer5 = millis();
  }


  if (wSensorData != 0) {
    waterLevel = TANK_HEIGHT - wSensorData;  
  } else {
    waterLevel = 0;
  }
    
  Blynk.run();  
  
  if (isCompressorActive == 1) {
     period = 5000;
  } else {
     period = 120000;
  }
  
  handleButton();  
   
  if (millis() - timer1 >= period) {
    Serial.println("-- Notify and etc.");
    notifyLevel(waterLevel); 
    
    Blynk.virtualWrite(V3, scale);    
    Blynk.virtualWrite(V2, waterLevel);  //Отправка данных на шкалу в Blynk 
       
    timer1 = millis();   
  } 
  
  if (isCompressorActive != pinCheck) {
     processTXData();
  }
  
  if (millis() - timer2 >= period) {
    processRXData();
    wSensorHealthCheck();
    handleData();
    timer2 = millis();
  }  
  
  if (millis() - timer3 >= 3000) {
    if (wSensorData != 0) {
     lcd.clear();
     timer3 = millis();
    }    
  } 
  shutDownRelay();      
}

//--------------------------------------
//Отправка управляющего сигнала на реле
void processTXData() {                 
    radio.stopListening();
    isCompressorActive = pinCheck;         
    Serial.print(isCompressorActive);
    Serial.println(" -- !!processTXData!!");   
    radio.write(&isCompressorActive, sizeof(isCompressorActive));         
}
//--------------------------------------
//Прием сырых данных уровня воды
void processRXData(){  
  radio.startListening();
    if (radio.available()) {
       radio.read(&data, sizeof(data));           
    }    
  Serial.print(wSensorData);
  Serial.println(" -- !!processRXData -- SensorData!!");       
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
  radio.setRetries(1, 15);    //(время между попыткой достучаться, число попыток)
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
}

void initDisplay() {
  lcd.init();
  // включаем подсветку
  lcd.backlight();
  // устанавливаем курсор в колонку 0, строку 0
  lcd.setCursor(0, 0);
  // печатаем первую строку
  lcd.print("Starting...");  
}

void operateLcd(int waterLevel) {
  lcd.setCursor(0, 0);
  lcd.print(String("Water level: ") + String(waterLevel));
  
  lcd.setCursor(0, 1);
  lcd.print(String("Scale level: ") + String(scale)); 
}

void setLcdCopressorState(int waterLevel) { 
  String compr;
  if (isCompressorActive == 0) {
    flags.clearFlag = 0;    
    compr = "Off";
  } else if ((flags.clearFlag == 0) && (isCompressorActive == 1)) {
    flags.clearFlag = 1;       
    compr = "On"; 
    lcd.clear();
    operateLcd(waterLevel);        
  }  
  lcd.setCursor(0, 2);  
  lcd.print(String("Compr. state: ") + compr); 
 
}

void shutDownRelay() {
  if (waterLevel >= FULL && isCompressorActive == 1 && wSensorData != 0) {
    Blynk.virtualWrite(V1, LOW);    
    pinCheck = 0;    
  }  
}

void handleButton() {    
  if ((digitalRead(D4) != HIGH) && (pinCheck == 0)) {   
    if (millis() - timer4 >= 200) {      
      pinCheck = 1;
//      Serial.println(isCompressorActive);
//      Serial.println("ON");      
      Blynk.virtualWrite(V1, HIGH);
      led0.on();
      timer4 = millis();      
    }    
   } else if ((digitalRead(D4) != HIGH) && (pinCheck == 1)) {   
    if (millis() - timer4 >= 200) {     
      pinCheck = 0;
//      Serial.println(pinCheck);
//      Serial.println("Off");      
      Blynk.virtualWrite(V1, LOW);
      led0.off();       
      timer4 = millis();
      }       
   }   
}

void relayHealthcheck() {
  radio.stopListening();
  bool check;
  byte payload = 12;
  check = radio.write(&payload, sizeof(payload));
  if (check) {     
     Blynk.setProperty(V4, "color", GREEN);      
  } else {     
     Blynk.setProperty(V4, "color", RED);
  }
}

void wSensorHealthCheck() {
  if (data.id == 1) {
    Blynk.setProperty(V5, "color", GREEN);
  } else {
    Blynk.setProperty(V5, "color", RED);
  }
}

void handleData() {
   if (data.id == 1) {
      wSensorData = data.data1;
    } else if (data.id == 2) {
      chicken.temp1 = data.data1 / 100;
      chicken.temp2 = data.data2 / 100;      
    } 
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
