// подключаем библиотеку LiquidCrystal_I2C
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
 
// создаем объект-экран, передаём используемый адрес 
// и разрешение экрана:
LiquidCrystal_I2C lcd(0x26, 20, 4);
 
void setup()
{
    // инициализируем экран
    lcd.init();
    // включаем подсветку
    lcd.backlight();
    // устанавливаем курсор в колонку 0, строку 0
    lcd.setCursor(0, 0);
    // печатаем первую строку
    lcd.print("MAMIK PRIVET");  
}
 
void loop()
{
}
