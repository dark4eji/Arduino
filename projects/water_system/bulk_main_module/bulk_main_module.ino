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

RF24 radio              (D4, D2);
WidgetLED ledRelay      (V8);
WidgetLED ledWater      (V9);
WidgetLED ledTemp       (V4);
WidgetTerminal terminal (V10);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

Data data;

const int TANK_HEIGHT = 165;
const int TANK_FULL   = 147;
const int TANK_HALF   = 70;
const int TANK_EMPTY  = 56;

const String GREEN    = "#23C48E";
const String RED      = "#D3435C";

byte full = 0;
byte half = 0;
byte empty = 0;
byte notifyFlag = 0;

const long utcOffsetInSeconds = 25200;

byte activityChicken    = 1;  // состояние датчика в курятнике
byte activityWater      = 1;  // состояние датчика воды
byte activityRelay      = 1;  // состояние реле

byte scheduledReboot    = 1;  // запланированная перезагрузка MCU (вкл/выкл)
byte conditionalReboot  = 1;  // условная перезагрузка в зависимости от состояния модулей (вкл/выкл)

int rebootTime          = 3;  // время перезагрузки (в часах)

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

int mapping = 8;
int scales = 81; //81 дробное значение для шкалы делений воды
int wlvls = 100;

const int mappingArray[mapping] = {6, 7, 8, 14, 22, 23, 24, 29};
const int pairsArray[mapping][2] = {{141, 140}, {139, 138}, {137, 136},
                   {130, 129}, {121, 120}, {119, 118},
                   {117, 116}, {111, 110}
                  };
int wlevelsArray[wlvls][2];
float scalesArray[scales];

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  timeClient.begin();

  showDateTime();
  terminal.println("Initializing MCU...");
  terminal.flush();

  ArduinoOTA.setHostname("MCU");
  ArduinoOTA.begin();

  setupRadio();

  timer_rxtx = millis();
  timer_notif = millis();
  timer_restart = millis();
  timer_IdTwo = millis();
  timer_IdOne = millis();

  ledRelay.on();
  ledWater.on();
  ledTemp.on();

  showDateTime();
  terminal.println("Building data arrays...");
  terminal.flush();
  buildWaterDataArrays();
  showDateTime();
  terminal.println("Arrays built");
  terminal.flush();

  showDateTime();
  terminal.println("MCU is ready");
  terminal.flush();
}

//================================================

void loop() {
  Blynk.run();
  ArduinoOTA.handle();
  restartMCU();

  waterLevel = data.data1 == 0 ? 0 : TANK_HEIGHT - data.data1;
  message = getWaterNotification();

  shutRelay(calcedWaterLevel);
  manageBlynkButton(calcedWaterLevel);

  if (millis() - timer_rxtx >= 50) {
    processTXData();
    processRXData();
    setData(waterLevel);
    timer_rxtx = millis();
  }

  if (millis() - timer_notif >= 1000) {
    if (notifyFlag == 1) {
        Blynk.notify(message);
    }
    timer_notif = millis();
  }

  if (activityRelay == 1) {
    Blynk.setProperty(V8, "color", GREEN);
  } else {
    Blynk.setProperty(V8, "color", RED);
  }
}

//============================

void setData(int waterLevel) {
  if (data.id == 1) {
    timer_IdOne = millis();
    activityChicken = 1;
    t1 = data.data2;
    t2 = data.data3;
    h1 = data.data4;
    h2 = data.data5;
    Blynk.virtualWrite(V0, t1);
    Blynk.virtualWrite(V1, h1);
    Blynk.virtualWrite(V2, t2);
    Blynk.virtualWrite(V3, t2);
    Blynk.setProperty(V4, "color", GREEN);
  } else {
    if (millis() - timer_IdOne >= 120000) {
      Blynk.setProperty(V4, "color", RED);
      Blynk.virtualWrite(V0, 0);
      Blynk.virtualWrite(V1, 0);
      Blynk.virtualWrite(V2, 0);
      Blynk.virtualWrite(V3, 0);
      activityChicken = 0;
      timer_IdOne = millis();
    }
  }

  if (data.id == 2) {
    timer_IdTwo = millis();
    activityWater = 1;
    Blynk.virtualWrite(V5, getScale());
    Blynk.virtualWrite(V6, waterLevel);
    Blynk.setProperty(V9, "color", GREEN);
  } else {
    if (millis() - timer_IdTwo >= 20000) {
      Blynk.virtualWrite(V5, 0);
      Blynk.virtualWrite(V6, 0);
      Blynk.setProperty(V9, "color", RED);
      activityWater = 0;
      timer_IdTwo = millis();
    }
  }
}

void shutRelay() {
  /* отключает реле при включенном насосе по достижении критического уровня +
     отклчает реле при неактивном сенсоре воды
  */
  if ((waterLevel >= TANK_FULL && compressor == 1) || activityWater == 0) {
      compressor = 0;
      pinState = 0;
      Blynk.virtualWrite(V7, LOW);
  }
}

void manageBlynkButton() {
  if (pinState == 1 && compressor == 0 && waterLevel < TANK_FULL) {
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
     //Serial.println("YES");
  } else {
    data.id = 0;
    //Serial.println("NA");
  }
}

void processTXData() {
    radio.stopListening();
    activityRelay = radio.write(&compressor, sizeof(compressor));
}

String getWaterNotification() {
  String notificationMessage = "";
  notifyFlag = 0;

  if (compressor == 0) {
    if ((waterLevel >= TANK_FULL) && (full != 1)) {
        full = 1;
        half = 0;
        empty = 0;
        notifyFlag = 1;
        return "!ВОДА! -- Бак полон";
    } else if (waterLevel == (TANK_FULL - 4)) {
        full = 0;
    }

    if ((waterLevel == TANK_HALF) && (half != 1)) {
        half = 1;
        notifyFlag = 1;
        return "!ВОДА! -- Пол бака";
    }

    if ((waterLevel == TANK_EMPTY) && (empty != 1)) {
        empty = 1;
        notifyFlag = 1;
        return "!ВОДА! -- !НИЗКИЙ УРОВЕНЬ ВОДЫ!";
    }
  }
  return notificationMessage;
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

void buildWaterDataArrays() {
  buildScalesArray();
  buildWLevelsArray();
}

void buildScalesArray() {
  for (int i = 0; i < scales; i++) {
      scale[i] = 10 - (i * 0.1);
  }
}

void buildWLevelsArray() {
  for (int i = 0; i < lvlAmount; i++) {
      for (int j = 0; j < 2; j++) {
          if (j % 2 == 0) {
              nums[i][j] = 147 - i;
          } else {
              nums[i][j] = 0;
          }
      }
  }

  for (int i = 0; i < mapping; i++) {
      for (int j = 0; j < 2; j++) {
          wlevelsArray[mappingArray[i]][j] = pairsArray[i][j];
      }

      for (int b = mappingArray[i] + 1; b < wlvls; b++) {
              wlevelsArray[b][0] -= 1;
      }
  }
}

float getScale() {
  float scale = 1;
  for (int i = 0; i < wlvls; i++) {
      for (int j = 0; j < 2; j++) {
          if (wlevelsArray[i][j] == waterLevel) {
            scale = scalesArray[i];
          }
      }
  }
  return scale;
}
