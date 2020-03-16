#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// WiFi Parameters
char* ssid = "RT-WiFi_95B8";
char* pass = "9edt3JgT";
String url = "http://192.168.0.59:8080/sendData";
String contentType = "application/json";

unsigned long timer1;

struct OutData {
  float bOutTempUpperSensor;
  float bOutTempLowerSensor;
  float bOutHumUpperSensor;
  float bOutHumLowerSensor;
  short outRawWaterData;
  
};

struct InData {
  float bInTempUpperSensor;
  float bInTempLowerSensor;
  float bInHumUpperSensor;
  float bIntHumLowerSensor;
  short inWaterLevel;
  byte isCompressorActive;  
};

OutData outData;
InData  inData;

void setup() {  
  Serial.begin(9600);
  WiFi.begin(ssid, pass); 
  while (WiFi.status() != WL_CONNECTED) {    
    delay(1000);
    Serial.println("Connecting...");    
  }
  timer1 = millis();
}

void loop() {
  if (millis() - timer1 >= 2000) {
    sendDataToServer();
    Serial.println(dataToGet.b_tempUpperSensor);
    Serial.println(dataToGet.b_tempLowerSensor);
    timer1 = millis();
  }
}

void sendDataToServer() {    
  if (WiFi.status() == WL_CONNECTED) {   
    HTTPClient http;    
    http.begin(url);
    http.addHeader("Content-Type", contentType);
    int httpCode = http.POST(buildJsonToPost());
                                                                        
    if (httpCode > 0) {      
      const size_t capacity = JSON_OBJECT_SIZE(2) + 310;
      DynamicJsonDocument docToGet(capacity);      
      DeserializationError error = deserializeJson(docToGet, http.getString());         
      if (error) {        
          Serial.print(("deserializeJson() failed: "));
          Serial.println(error.c_str());
          return;
        }        
      dataToGet.b_tempUpperSensor = docToGet["tempResult1"];
      dataToGet.b_tempLowerSensor = docToGet["tempResult2"];   
    }
    http.end();   //Close connection
  }
}

String buildJsonToPost() {
  String json;
  StaticJsonDocument<200> docToPost;
    
  docToPost["b_tempUpperSensor"] = dataToPost.b_tempUpperSensor;
  docToPost["b_tempLowerSensor"] = dataToPost.b_tempLowerSensor;
  
  serializeJson(docToPost, json);
  return json;
}
