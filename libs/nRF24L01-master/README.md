﻿# nRF24L01
Модули беспроводной связи nRF24L01, настройка и примеры

## Папки

**RF24-master** - библиотека для модуля связи, установить в C:\Program Files\Arduino\libraries
  
**nRF24 tests** - тесты модулей
  
- **GettingStarted_CallResponse** - тест качества связи между двумя модулями
- **nrf_listen_air** - тест одного модуля, прослушивание всех каналов
- **время передачи** - скетч, измеряющий время передачи с модуля на модуль (сильно упрощённый GettingStarted_CallResponse)
- **тест расстояния** - скетч из видео, к Ардуино подключен дисплей, отображающий число принятых пакетов за единицу времени
  
**Projects** - папка со схемами и скетчами
  
- **3 канальное управление** - трёхканальное управлением реле, LED лентой и сервомашинкой
- **простой приём передача** - базовая пара скетчей со всеми настрйоками модулей, а также примером передачи данных
 
 
## Схема подключения
![подключение](https://github.com/AlexGyver/nRF24L01/blob/master/connect.jpg)
![подключение](https://github.com/AlexGyver/nRF24L01/blob/master/connect_adapter.png)
