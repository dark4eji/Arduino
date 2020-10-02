#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ArduinoOTA.h>
#include <WaterHandler.h>
#include <Constants.h>

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

const long utcOffsetInSeconds = 25200;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

byte activityChicken    = 1;  // состояние датчика в курятнике
byte activityWater      = 1;  // состояние датчика воды
byte activityRelay      = 1;  // состояние реле

byte scheduledReboot    = 1;  // запланированная перезагрузка MCU (вкл/выкл)
byte conditionalReboot  = 1;  // условная перезагрузка в зависимости от состояния модулей (вкл/выкл)

int rebootTime          = 3;  // время перезагрузки (в часах)

RF24 radio              (D4, D2);
WidgetLED ledRelay      (V8);
WidgetLED ledWater      (V9);
WidgetLED ledTemp       (V4);

WaterHandler  wh;
Constants     ns;

// WiFi Parameters
char auth[] = "3e883JyisvmmRpfE9RrWlw_Qoa-B0vCX";
char ssid[] = "RT-WiFi_95B8_Repeater";
char pass[] = "9edt3JgT";

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

unsigned long timer_rxtx;
unsigned long timer_notif;
unsigned long timer_restart;
unsigned long timer_IdTwo;
unsigned long timer_IdOne;

int currentHour;
int currentMin;
int currentSec;

float t1;
float t2;
float h1;
float h2;

int waterLevel;
int pinState;
byte compressor;
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
WidgetTerminal terminal(V10);

void setup() {
  Serial.begin            (9600);
  Blynk.begin             (auth, ssid, pass);
  timeClient.begin        ();

  showDateTime            ();
  terminal.println        ("Initializing MCU...");
  terminal.flush          ();

  ArduinoOTA.setHostname  ("MCU");
  ArduinoOTA.begin        ();

  setupRadio();

  timer_rxtx = millis     ();
  timer_notif = millis    ();
  timer_restart = millis  ();
  timer_IdTwo = millis    ();
  timer_IdOne = millis    ();

  ledRelay.on             ();
  ledWater.on             ();
  ledTemp.on              ();

  showDateTime            ();
  terminal.println        ("MCU is ready");
  terminal.flush          ();
}

void loop() {
  Blynk.run();
  ArduinoOTA.handle();
  restartMCU();
  shutRelay();
  manageBlynkButton();

  if (millis() - timer_rxtx >= 50) {
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

  if (activityRelay == 1) {
    Blynk.setProperty(V8, "color", ns.GREEN);
  } else {
    Blynk.setProperty(V8, "color", ns.RED);
  }
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
      activityChicken = 0;
      timer_IdOne = millis();
    }
  } else {
    timer_IdOne = millis();
    activityChicken = 1;
    setIdOneParams();
  }

  if (data.id != 2) {
    if (millis() - timer_IdTwo >= 20000) {
      Blynk.virtualWrite(V5, 0);
      Blynk.virtualWrite(V6, 0);
      Blynk.setProperty(V9, "color", ns.RED);
      activityWater = 0;
      timer_IdTwo = millis();
    }
  } else {
    timer_IdTwo = millis();
    activityWater = 1;
    setIdTwoParams();
  }
}

void shutRelay() {
  /* отключает реле при включенном насосе по достижении критического уровня +
     отклчает реле при неактивном сенсоре воды
  */
  if ((waterLevel >= ns.TANK_FULL && compressor == 1) || activityWater == 0) {
      compressor = 0;
      pinState = 0;
      Blynk.virtualWrite(V7, LOW);
      terminal.println("Shut off relay");
  }
  
}

void manageBlynkButton() {
  if (pinState == 1 && compressor == 0 && waterLevel < ns.TANK_FULL) {
      compressor = 1;
      terminal.println("compressor = 1");
      Blynk.virtualWrite(V7, HIGH);
  } else if (pinState == 0 && compressor == 1) {
      compressor = 0;
      terminal.println("compressor = 0");
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
    activityRelay = radio.write(&compressor, sizeof(compressor));
}

void applyRestart() {
  showDateTime();
  terminal.println("Restarting MCU...");
  terminal.flush();
  radio.powerDown();
  delay(1000);
  ESP.restart();
}

void restartMCU() {
  if (scheduledReboot == 1) {
    timeClient.update();
    if (timeClient.getHours() == rebootTime && timeClient.getMinutes() == 0
                                            && timeClient.getSeconds() == 0) {
      applyRestart();
    }
  }

  if (conditionalReboot == 1) {
    if (activityChicken == 0 && activityWater == 0 && activityRelay == 0) {
      if (millis() - timer_restart >= 150000) {
        applyRestart();
      }
    } else {
      timer_restart = millis();
    }
  }
}

void showDateTime() {
  timeClient.update();
  String dateTime = timeClient.getFormattedDate();
  terminal.print(dateTime + " ");
}

BLYNK_WRITE(V7) {
  pinState = param.asInt();
}

void setupRadio() {
  radio.begin             (); //активировать модуль
  radio.setAutoAck        (1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries        (1, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload  ();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize    (32);     //размер пакета, в байтах
  radio.setChannel        (0x6b);
  radio.openWritingPipe   (address[2]);   //мы - труба 0, открываем канал для передачи данных
  radio.openReadingPipe   (1, address[1]);
  radio.setPALevel        (RF24_PA_MAX);
  radio.setDataRate       (RF24_250KBPS);
  radio.powerUp           (); //начать работу
  radio.startListening    ();  //не слушаем радиоэфир, мы передатчик
}
