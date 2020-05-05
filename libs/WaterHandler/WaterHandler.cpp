#include "Arduino.h"
#include "WaterHandler.h"
#include "Constants.h"

Constants cs;

short WaterHandler::getWaterLvl(int rawWaterLevel) {
  return rawWaterLevel != 0 ? cs.TANK_HEIGHT - rawWaterLevel : 0;
}

byte WaterHandler::getNotifyFlag() {
  return this->notifyFlag;
}

float WaterHandler::getScale(int waterLevel) {
  float scale = 1;
  short levelsArray[17] = {146, 143, 135, 129, 124, 116, 110, 102, 97,
                           89, 82, 75, 70, 68, 61, 56, 47};

  float scalesArray[17] = {10, 9.5, 9, 8.5, 8, 7.5, 7, 6.5, 6,
                           5.5, 5, 4.5, 4, 3.5, 3, 2.5, 2};

  for (int i = 0; i <= 17 - 1; i++) {
    if ((waterLevel < levelsArray[i]) && (waterLevel > levelsArray[i + 1])) {
      return scalesArray[i + 1];
    } else if (waterLevel == levelsArray[i]) {
      return scalesArray[i];
    } else if ((waterLevel > levelsArray[0) || (waterLevel > levelsArray[1])) {
      return scalesArray[0];
    }
  }
  return scale;
}

String WaterHandler::getWaterNotification(int waterLevel) {
  String notificationMessage = "";
  this->notifyFlag = 0;
  if ((waterLevel >= cs.TANK_FULL) && (this->full != 1)) {
      this->full = 1;
      this->notifyFlag = 1;
      return "!ВОДА! -- Бак полон";
  } else if (waterLevel == (cs.TANK_FULL - 4)) {
      this->full = 0;
  }

  if ((waterLevel == cs.TANK_HALF) && (this->half != 1)) {
      this->half = 1;
      this->notifyFlag = 1;
      return "!ВОДА! -- Пол бака";
  } else if (waterLevel == (cs.TANK_HALF - 4)) {
      this->half = 0;
  } else if ((waterLevel == (cs.TANK_HALF + 4)) && (this->half == 1)) {
      this->half = 0;
  }

  if ((waterLevel == cs.TANK_EMPTY) && (this->empty != 1)) {
      this->empty = 1;
      this->notifyFlag = 1;
      return "!ВОДА! -- !НИЗКИЙ УРОВЕНЬ ВОДЫ!";
  } else if ((waterLevel == (cs.TANK_EMPTY + 4)) && (this->empty == 1)) {
      this->empty = 0;
  }
  return notificationMessage;
}
