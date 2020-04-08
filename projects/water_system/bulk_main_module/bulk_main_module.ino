#include <ArduinoOTA.h>
#include <WaterHandler.h>
#include <Constants.h>

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(D4, D2);
WidgetLED ledRelay(V8);
WidgetLED ledWater(V9);
WidgetLED ledTemp(V4);

WaterHandler wh;
Constants ns;

// WiFi Parameters
char auth[] = "3e883JyisvmmRpfE9RrWlw_Qoa-B0vCX";
char ssid[] = "RT-WiFi_95B8";
char pass[] = "9edt3JgT";

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

unsigned long timer_rxtx;
unsigned long timer_json;
unsigned long timer_notif;
unsigned long timer_IdTwo;
unsigned long timer_IdOne;

float t1;
float t2;
float h1;
float h2;

byte wstate;
int waterLevel;
int pinState;
byte compressor;
int rslt;
String message;

struct Data {
  short id = 0; // id 1 = модуль температуры в сарае, id 2 = модуль воды
  short data1 = 0;
  float data2 = 0;
  float data3 = 0;
  float data4 = 0;
  float data5 = 0;
};

Data data;

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  ArduinoOTA.setHostname("MCU");
  ArduinoOTA.begin();

  setupRadio();

  timer_json = millis();
  timer_rxtx = millis();
  timer_notif = millis();
  timer_IdTwo = millis();
  timer_IdOne = millis();

  ledRelay.on();
  ledWater.on();
  ledTemp.on();
}

void loop() {
  Blynk.run();
  ArduinoOTA.handle();

  shutRelay();
  manageBlynkButton();

  if (millis() - timer_rxtx >= 10) {
    processTXData();
    processRXData();
    setData();
    timer_rxtx = millis();
  }

  if (millis() - timer_notif >= 1000) {
    if (wh.getNotifyFlag() == 1) {
        Blynk.notify(message);
    }
    timer_notif = millis();
  }

  if (rslt == 1) {
    Blynk.setProperty(V8, "color", ns.GREEN);
  } else {
    Blynk.setProperty(V8, "color", ns.RED);
  }

  Serial.println(compressor);
  Serial.println(rslt);
}

void setIdOneParams() {
  t1 = data.data2;
  t2 = data.data3;
  h1 = data.data4;
  h2 = data.data5;
  Blynk.virtualWrite(V0, t1);
  Blynk.virtualWrite(V1, h1);
  Blynk.virtualWrite(V2, t2);
  Blynk.virtualWrite(V3, t2);
  Blynk.setProperty(V4, "color", ns.GREEN);
}

void setIdTwoParams(){
  waterLevel = wh.getWaterLvl(data.data1);
  message = wh.getWaterNotification(waterLevel);
  Blynk.virtualWrite(V5, wh.getScale(waterLevel));
  Blynk.virtualWrite(V6, waterLevel);
  Blynk.setProperty(V9, "color", ns.GREEN);
}

void setData() {
  if (data.id != 1) {
    if (millis() - timer_IdOne >= 120000) {
      Blynk.setProperty(V4, "color", ns.RED);
      Blynk.virtualWrite(V0, 0);
      Blynk.virtualWrite(V1, 0);
      Blynk.virtualWrite(V2, 0);
      Blynk.virtualWrite(V3, 0);
      timer_IdOne = millis();
    }
  } else {
    timer_IdOne = millis();
    setIdOneParams();
  }

  if (data.id != 2) {
    if (millis() - timer_IdTwo >= 20000) {
      Blynk.virtualWrite(V5, 0);
      Blynk.virtualWrite(V6, 0);
      Blynk.setProperty(V9, "color", ns.RED);
      wstate = 0;
      timer_IdTwo = millis();
    }
  } else {
    timer_IdTwo = millis();
    wstate = 1;
    setIdTwoParams();
  }
}

void shutRelay() {
  if (waterLevel >= 130 && compressor == 1) {
      compressor = 0;
      pinState = 0;
      Blynk.virtualWrite(V7, LOW);
  }
}

void manageBlynkButton() {
  if (pinState == 1 && compressor == 0 && waterLevel < 130) {
      compressor = 1;
      Blynk.virtualWrite(V7, HIGH);
  } else if (pinState == 0 && compressor == 1) {
      compressor = 0;
      Blynk.virtualWrite(V7, LOW);
  }
}

void processRXData(){
  radio.startListening();
  if (radio.available()) {
     radio.read(&data, sizeof(data));
     Serial.println("YES");
  } else {
    data.id = 0;
    Serial.println("NA");
  }
}

void processTXData() {
    radio.stopListening();
    rslt = radio.write(&compressor, sizeof(compressor));
}

BLYNK_WRITE(V7) {
  pinState = param.asInt();
  if (waterLevel >= 130 || wstate == 0) {
    Blynk.virtualWrite(V7, LOW);
    compressor = 0;
    pinState = 0;
  }
}

void setupRadio() {
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(1, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.setChannel(0x6b);
  radio.openWritingPipe(address[2]);   //мы - труба 0, открываем канал для передачи данных
  radio.openReadingPipe(1, address[1]);
  radio.setPALevel (RF24_PA_MAX);
  radio.setDataRate (RF24_250KBPS);
  radio.powerUp(); //начать работу
  radio.startListening();  //не слушаем радиоэфир, мы передатчик
}
