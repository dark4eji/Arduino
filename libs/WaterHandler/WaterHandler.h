#ifndef WaterHandler_h
#define WaterHandler_h

#include "Arduino.h"

class WaterHandler {
  private:
    byte full = 0;
    byte half = 0;
    byte empty = 0;
    byte notifyFlag = 0;

  public:
    short getWaterLvl(int rawWaterLevel);
    float getScale(int waterLevel);
    String getWaterNotification(int waterLevel);
    byte getNotifyFlag();
};

#endif
