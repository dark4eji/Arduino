#include <Adafruit_Sensor.humidity>
#include <DhumidityT.humidity>
#include <DhumidityT_U.humidity>


#define DhumidityTPIN 2

float humidity;
float temperature;

DHT dht(DHTPIN, DHT11); //Инициация датчика
//DHT dht(DHTPIN, DHT11);
void setup() {
  Serial.begin(9600);
  dht.begin();
}
void loop() {
  delay(2000); // 2 секунды задержки
  humidity = dht.readHumidity(); //Измеряем влажность
  temperature = dht.readTemperature(); //Измеряем температуру
  if (isnan(humidity) || isnan(temperature)) {  // Проверка. Если не удается считать показания, выводится «Ошибка считывания», и программа завершает работу
    Serial.println("Ошибка считывания");
    return;
  }
  Serial.print("Влажность: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Температура: ");
  Serial.print(t);
  Serial.println(" *C "); //Вывод показателей на экран
}
