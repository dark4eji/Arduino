#ifndef ConstFlags_h
#define ConstFlags_h

#include "Arduino.h"

class Constants {
  public:
    const short TANK_HEIGHT = 165;
    const short TANK_FULL = 147;
    const short TANK_HALF = 70;
    const short TANK_EMPTY = 56;

    const String GREEN = "#23C48E";
    const String RED = "#D3435C";
};

#endif
