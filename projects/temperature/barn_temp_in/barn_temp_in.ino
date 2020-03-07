#define GREEN     "#23C48E"
#define RED       "#D3435C"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

unsigned long timer;
unsigned long timer2;

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};  //1Node - главный модуль; 2Node - модуль реле; 3Node - модуль сенсора воды

RF24 radio(D4, D2);
WidgetLED led0(V4);

char auth[] = "3e883JyisvmmRpfE9RrWlw_Qoa-B0vCX";
char ssid[] = "RT-WiFi_95B8";
char pass[] = "9edt3JgT";

struct Data {
  float humidity1;
  float temperature1;
  float humidity2;
  float temperature2;
};

Data data;

void setup(){         
  Serial.begin(9600); //открываем порт для связи с ПК    
  Blynk.begin(auth, ssid, pass);
  led0.on();    
  timer = millis();
  setupRadio();   
}

void loop() {    
  Blynk.run();
  if (millis() - timer >= 2000) {   
    if (radio.available()) {
       radio.read(&data, sizeof(data));
       data.temperature1 = data.temperature1 != 0 ? data.temperature1 - 1.6 : 0;
       data.temperature2 = data.temperature2 != 0 ? data.temperature2 - 0.9 : 0;
       data.humidity1 = data.humidity1 != 0 ? data.humidity1 + 4.0 : 0; 
       data.humidity2 = data.humidity2 != 0 ? data.humidity2 - 3.0 : 0; 
       Serial.println(data.temperature1);    
       Blynk.setProperty(V4, "color", GREEN);           
    } else {
      data.temperature1 = 0;
      data.humidity1 = 0;
      data.temperature2 = 0;
      data.humidity2 = 0;
      Blynk.setProperty(V4, "color", RED);
      
//      if (millis() - timer2 >= 120000) {
//         Blynk.notify("Модуль датчиков не активен 3 минуты");  
//         timer2 = millis();
//      }
      
      Serial.println("Test2");
    }    
    Blynk.virtualWrite(V0, data.temperature1);
    Blynk.virtualWrite(V1, data.humidity1); 
    Blynk.virtualWrite(V2, data.temperature2);
    Blynk.virtualWrite(V3, data.humidity2);       
    timer = millis();
  }
} 

void setupRadio() {
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(1, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.setChannel(0x6b);
  radio.openReadingPipe(1, address[0]);
  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!
  radio.powerUp(); //начать работу 
  radio.startListening();
}
  
