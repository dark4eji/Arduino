#include <NTPClient.h>
#include <WiFiUdp.h>

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

WidgetLED leftLamp      (V11);
WidgetLED rightLamp     (V12);
WidgetLED heater        (V13);
WidgetLED socket        (V14);
WidgetLED automation    (V15);

WidgetTerminal terminal (V10);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

byte full = 0;
byte half = 0;
byte empty = 0;
byte notifyFlag = 0;

byte activityChicken    = 1;  // состояние датчика в курятнике
byte activityWater      = 1;  // состояние датчика воды
  // состояние реле

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
unsigned long syncTimer;
unsigned long timer_relay;
unsigned long timer_relay_send;

float x = 0;

int currentHour;
int currentMin;
int currentSec;

int notifyPeriod = 5000; // период отправки уведомлений об уровне воды

int waterLevel;
float scale;
int pinState;
byte compressor;
String message;

short relayHealthcheck = 0;
short syncStatus = 0;

/*
  Механизм предотвращение избыточных расчетов делений и отправки уровня воды
*/
byte prevWaterLvl = 0; //Предыдущий уровень воды
byte currentWaterLvl = -1; //Текущий уровень воды

struct TXMessage {
  short id = 10; // id 1 = реле, id 2 = сарай, id 10 = синхронизатор
  short data1; // левая лампа
  short data2; // правая лампа
  short data3; // общий свет
  short data4; // нагрев
  short data5; // розетка  
  short data6; // бесполезная нагрузка от компрессора 
};

struct TempData {
  float t1 = 0;
  float h1 = 0;
  float t2 = 0;
  float h2 = 0;
};

struct Data {
  short id = 0; // id 1 = модуль температуры в сарае, id 2 = модуль воды,id 10 = синхронизатор, 3 = реле хелсчек
  short data1 = 0; //Water data
  float data2 = 0; //Temper 1
  float data3 = 0; //Temper 2
  float data4 = 0; //Hum 1
  float data5 = 0; //Hum 2
};

Data data;
TempData tempData;
TXMessage TXm;

float scalesArray[SCALES][3];

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  timeClient.begin();

  printTerminalMessage("Initializing MCU...");

  setupRadio();

  timer_rxtx = millis();
  timer_notif = millis();
  timer_restart = millis();
  timer_IdTwo = millis();
  timer_IdOne = millis();
  timer_notif_get = millis();
  syncTimer = millis();
  timer_relay = millis();
  timer_relay_send = millis();

  ledRelay.on();
  ledWater.on();
  ledTemp.on();

  leftLamp.on();
  rightLamp.on();
  heater.on();
  socket.on();
  automation.on();

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
  restartMCU();
  
  manageBarnButtons();
  manageMainRelayButton();

  notifyPeriod = compressor == 0 ? 60000 : 5000;

  if (millis() - timer_notif_get >= notifyPeriod){
      message = getWaterNotification();
      if (notifyFlag == 1) {
        Blynk.notify(message);
      }
      timer_notif_get = millis();
  }
  
  shutRelay();  
  relayHealthcheck = 0;
  
  if (millis() - timer_rxtx >= 50) {  
    processRXData(); 
    setData();  
    processTXData();        
    messenger();
    timer_rxtx = millis();
  }
}

void messenger() {
  Serial.print("SyncStatus: ");
  Serial.println(String(syncStatus));
  Serial.print("Txm.id: ");
  Serial.println(String(TXm.id));
  Serial.print("Txm.data1: ");
  Serial.println(TXm.data1);
  Serial.print("Txm.data2: ");
  Serial.println(TXm.data2);
  Serial.print("data.id: ");
  Serial.println(data.id);
  Serial.println(TXm.data6);
  Serial.println("****"); 
  Serial.println(millis()); 
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
      getScale();      
    }
    
    Blynk.virtualWrite(V5, scale);
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

  if (data.id == 10 && syncStatus == 0) {
      syncStatus = 1;    
      TXm.id = 0; // флаг окончания синхронизации и вход в штатный режим
      TXm.data1 = data.data1; // левая лампа
      TXm.data2 = data.data2; // правая лампа
      Serial.print("****************************************************************************");
      Serial.println(data.data1);
      Serial.println(data.data2);
      Serial.print("****************************************************************************");
      TXm.data3 = data.data3; // общий свет
      TXm.data4 = data.data4; // нагрев
      TXm.data5 = data.data5; // розетка
      data.id = 0;      
  } 

  if (syncStatus == 0) {
    if (millis() - syncTimer >= 15000) {
      TXm.id = 10;
      syncTimer = millis();
    }
  }

  if (data.id == 3) {
    timer_relay = millis();
    relayHealthcheck = 1;
    Blynk.setProperty(V8, "color", GREEN);
  } else {
    if (millis() - timer_relay >= 10000) {
      Blynk.setProperty(V8, "color", RED);
      timer_relay = millis();
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

void processRXData(){
  radio.startListening();
  if (radio.available()) {
     radio.read(&data, sizeof(data));     
  } 
}

void processTXData() {
    radio.stopListening();
    TXm.data6 = compressor;  
    
    if ((TXm.id != 10) && (syncStatus == 1) && (millis() - timer_relay_send >= 2000)) {
        TXm.id = 2;
        timer_relay_send = millis();
    }
    
    radio.write(&TXm, sizeof(TXm));
     
    if (TXm.id != 1) {
      TXm.id = 1;
    }   
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

BLYNK_WRITE(V7) {
  pinState = param.asInt();  
}

BLYNK_WRITE(V16) {
  if (TXm.id != 10) {
    TXm.id = 2;  
    TXm.data1 = param.asInt();
   manageBarnButtons();
  } 
}  
 

BLYNK_WRITE(V17) {
  if (TXm.id != 10) {
    TXm.id = 2;
    TXm.data2 = param.asInt();
    manageBarnButtons();
  }
}

void manageMainRelayButton() {
  if (pinState == 1 && compressor == 0 && waterLevel < TANK_FULL) {
      compressor = 1;
      Blynk.virtualWrite(V7, HIGH);
  } else if (pinState == 0 && compressor == 1) {
      compressor = 0;
      Blynk.virtualWrite(V7, LOW);
  }
}

void manageBarnButtons() {

  if (TXm.data1 == 1) {     
      Blynk.virtualWrite(V16, HIGH);
      Blynk.setProperty(V11, "color", GREEN);
  } else {
      Blynk.virtualWrite(V16, LOW);
      Blynk.setProperty(V11, "color", RED);
  }
  
   if (TXm.data2 == 1) {         
      Blynk.virtualWrite(V17, HIGH);
      Blynk.setProperty(V12, "color", GREEN);
  } else {
      Blynk.virtualWrite(V17, LOW);
      Blynk.setProperty(V12, "color", RED);
  }
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
    if (activityChicken == 0 && activityWater == 0 && relayHealthcheck == 0) {
      if (millis() - timer_restart >= 150000) {
        applyRestart();
      }
    } else {
      timer_restart = millis();
    }
  }
}

void getScale() {  
  for (int i = 0; i < SCALES; i++) {
      for (int j = 1; j < 3; j++) {
          if (scalesArray[i][j] == waterLevel) {
             scale = scalesArray[i][0];
          }
      }
    }  
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

    for (int b = mappingArray[i] + 1; b < wlvlsLength; b++) {
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
