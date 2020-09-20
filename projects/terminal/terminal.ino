#define DSWB 2

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// создаем объект-экран, передаём используемый адрес
// и разрешение экрана:
LiquidCrystal_I2C lcd(0x26, 20, 4);
int initCheck = 0;

int dSwCheck = 0;
int dSwButRead;
int dSwPrevState = HIGH;
int dSwCurState = LOW;
unsigned long dSwTimer;

int debounce = 200;

struct DataToDisplay {
  int wLevelPartition = 0;
  int comprState = 0;
  float hS1 = 0;
  float hS2 = 0;
  float tS1 = 0;
  float tS2 = 0;
};

struct DataToSend {
  short id = 3; // id 1 = модуль температуры в сарае, id 2 = модуль воды, id 3 = терминал
  short data1 = 0;
  float data2 = 0;
  float data3 = 0;
  float data4 = 0;
  float data5 = 0;
  short data6 = 0; //состояние компрессора со стороны терминала
};

DataToDisplay data;

byte degree[8] = // кодируем символ градуса
{
B00111,
B00101,
B00111,
B00000,
B00000,
B00000,
B00000,
};

void setup()
{   Serial.begin(9600);
  // инициализируем экран
  lcd.init();
    // включаем подсветку
  lcd.backlight();
  // устанавливаем курсор в колонку 0, строку 0
  lcd.setCursor(0, 0);
    // печатаем первую строку
  lcd.print("Inititalization...");
  lcd.createChar(1, degree); // Создаем символ под номером 1
  pinMode(DSWB, INPUT_PULLUP);
  dSwTimer = millis();
}

void loop()
{
  if (initCheck == 0) {
    changeDispData();
  }
  handleDisplayButton();
  Serial.println(dSwCheck);
}

void handleDisplayButton()
{
  dSwCurState = digitalRead(DSWB);
  if ((dSwCurState == LOW && dSwPrevState == HIGH) && (millis() - dSwTimer >= 200)) {
    changeDispData();
  }
  dSwPrevState = dSwCurState;
}

void waterPage() {
  lcd.setCursor(0, 0);
  lcd.print(String("        Water"));
  lcd.setCursor(0, 1);
  lcd.print(String("Deleniya:  ") + String(data.wLevelPartition));
  lcd.setCursor(0, 2);
  lcd.print(String("Nasos:     ") + getCompressorState());
}

void chickPage() {
  lcd.setCursor(0, 0);
  lcd.print(String("    Temperature"));
  lcd.setCursor(0, 1);
  lcd.print(String("Verh: ") + String(data.tS1) + String("\1C") + String("  ") + String(data.hS1) + String("%"));
  lcd.setCursor(0, 2);
  lcd.print(String("Niz:  ") + String(data.tS2) + String("\1C") + String("  ") + String(data.hS2) + String("%"));
}

String getCompressorState() {
  String comprOnOff;
  if (data.comprState == 0) comprOnOff = "Vykl"; else comprOnOff = "Vkl";
  return comprOnOff;
}

void changeDispData(){
  lcd.clear();
  if (dSwCheck == 0) {
    dSwCheck = 1;
    waterPage();
    dSwTimer = millis();
  } else {
    dSwCheck = 0;
    chickPage();
    dSwTimer = millis();
  }
  if (initCheck == 0) {
    initCheck = 1;
  }
}
