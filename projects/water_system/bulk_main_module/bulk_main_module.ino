#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ArduinoOTA.h>

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

const int TANK_HEIGHT = 165;
const int TANK_FULL   = 148;
const int TANK_HALF   = 70;
const int TANK_EMPTY  = 56;
const int SCALES      = 110;

const String GREEN    = "#23C48E";
const String RED      = "#D3435C";

const long utcOffsetInSeconds = 25200;

RF24 radio              (D4, D2);
WidgetLED ledRelay      (V8);
WidgetLED ledWater      (V9);
WidgetLED ledTemp       (V4);
WidgetTerminal terminal (V10);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

byte full = 0;
byte half = 0;
byte empty = 0;
byte notifyFlag = 0;

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
unsigned long timer_notif_get;

int currentHour;
int currentMin;
int currentSec;

int waterLevel;
int pinState;
byte compressor;
String message;

/*
  Механизм предотвращение избыточных расчетов делений и отправки уровня воды
*/
byte prevWaterLvl = 0; //Предыдущий уровень воды
byte currentWaterLvl = -1; //Текущий уровень воды

struct TempData {
  float t1 = 0;
  float h1 = 0;
  float t2 = 0;
  float h2 = 0;
};

TempData tempData;

struct Data {
  short id = 0; // id 1 = модуль температуры в сарае, id 2 = модуль воды
  short data1 = 0; //Water data
  float data2 = 0; //Temper 1
  float data3 = 0; //Temper 2
  float data4 = 0; //Hum 1
  float data5 = 0; //Hum 2
};

Data data;

float scalesArray[SCALES][3];

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  timeClient.begin();

  printTerminalMessage("Initializing MCU...");

  ArduinoOTA.setHostname("MCU");
  ArduinoOTA.begin();

  setupRadio();

  timer_rxtx = millis();
  timer_notif = millis();
  timer_restart = millis();
  timer_IdTwo = millis();
  timer_IdOne = millis();
  timer_notif_get = millis();

  ledRelay.on();
  ledWater.on();
  ledTemp.on();

  printTerminalMessage("Building scale array...");
  buildScalesArray();
  if (scalesArray[0][0] == 11) {
    printTerminalMessage("Success");
    printTerminalMessage("MCU is ready");
  } else {
    printTerminalMessage("Error: [0][0] != 11");
  }
}

//================================================

void loop() {
  Blynk.run();
  ArduinoOTA.handle();
  restartMCU();

  if (millis() - timer_notif_get >= 1000){
      message = getWaterNotification();
      timer_notif_get = millis();
  }

  shutRelay();
  manageBlynkButton();

  if (millis() - timer_rxtx >= 50) {
    processTXData();
    processRXData();
    setData();
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

void setData() {
  if (data.id == 1) {
    timer_IdOne = millis();
    activityChicken = 1;

    tempData.t1 = data.data2;
    tempData.h1 = data.data4;
    tempData.t2 = data.data3;
    tempData.h2 = data.data5;

    Blynk.virtualWrite(V0, tempData.t1); //Temper 1
    Blynk.virtualWrite(V1, tempData.h1); //Hum 1
    Blynk.virtualWrite(V2, tempData.t2); //Temper 2
    Blynk.virtualWrite(V3, tempData.h2); //Hum 2
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
    waterLevel = data.data1 == 0 ? 0 : TANK_HEIGHT - data.data1;

    timer_IdTwo = millis();

    activityWater = 1;
    currentWaterLvl = waterLevel;

    if (prevWaterLvl != currentWaterLvl) {
      Serial.println("CHECK");
      prevWaterLvl = currentWaterLvl;
      Blynk.virtualWrite(V5, getScale(waterLevel));
    }
    
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
  /*
    отключает реле при включенном насосе по достижении критического уровня +
    отключает реле при неактивном сенсоре воды
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

    if ((waterLevel >= TANK_FULL) && (full != 1)) {
        full = 1;
        notifyFlag = 1;
        return "!ВОДА! -- Бак полон";
    } else if (waterLevel == (TANK_FULL - 4)) {
        full = 0;
    }

    if ((waterLevel == TANK_HALF) && (half != 1)) {
        half = 1;
        notifyFlag = 1;
        return "!ВОДА! -- Пол бака";
    } else if (waterLevel == (TANK_HALF - 4)) {
        half = 0;
    } else if ((waterLevel == (TANK_HALF + 4)) && (half == 1)) {
        half = 0;
    }

    if ((waterLevel == TANK_EMPTY) && (empty != 1)) {
        empty = 1;
        notifyFlag = 1;
        return "!ВОДА! -- !НИЗКИЙ УРОВЕНЬ ВОДЫ!";
    } else if ((waterLevel == (TANK_EMPTY + 4)) && (empty == 1)) {
        empty = 0;
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
  printTerminalMessage("Restarting MCU...");
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

float getScale(int waterLevel) {
  float scale = 0;
  for (int i = 0; i < SCALES; i++) {
      for (int j = 1; j < 3; j++) {
          if (scalesArray[i][j] == waterLevel) {
             scale = scalesArray[i][0];
          }
	    }
    }
  return scale;
}

void buildScalesArray() {
  int wlvlsLength = 138;

  byte wlevelsArray[wlvlsLength][2];

  const byte mappingArray[] = { 16, 17, 18, 19, 27, 28, 29, 36, 37, 38, 39, 47, 48, 49,
                                56, 57, 58, 59, 66, 67, 68, 69, 77, 78, 79, 87, 88, 89,
                                96, 97, 98, 99, 105, 106, 107, 108, 109, 110 };

  int mapping = (sizeof(mappingArray) / sizeof(mappingArray[0]));

  for (int i = 0; i < wlvlsLength; i++) {
    for (int j = 0; j < 2; j++) {
	  if (j % 2 == 0) {
	      wlevelsArray[i][j] = TANK_FULL - i;
	  } else {
	      wlevelsArray[i][j] = 0;
	  }
    }
  }

  for (int i = 0; i < mapping; i++) {
    for (int j = 1, num = wlevelsArray[mappingArray[i]][0] - 1; j < 2; j++) {
	    wlevelsArray[mappingArray[i]][j] = num;
    }

    for (int b = mappingArray[i] + 1; b < wlvlsLength; b++)	{
	    wlevelsArray[b][0] -= 1;
    }
  }

  for (int i = 0; i < SCALES; i++) {
     scalesArray[i][0] = (SCALES / 10) - (i * 0.1);
     for (int j = 1; j < 3; j++) {
      scalesArray[i][j] = wlevelsArray[i][j - 1];
     }
   }
   scalesArray[0][2] = 149;
}

/*
  Служебные функции
*/

void printTerminalMessage(String message) {
  showDateTime();
  terminal.println(message);
  terminal.flush();
}

void showDateTime() {
  timeClient.update();
  String dateTime = timeClient.getFormattedDate();
  terminal.print(dateTime + " ");
}
