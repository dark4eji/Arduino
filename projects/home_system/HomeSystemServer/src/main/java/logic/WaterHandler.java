package logic;

import models.DataRequest;
import models.DataResponse;

public class WaterHandler {
    public static void handleWaterData(DataRequest dataRequest,
                                DataResponse dataResponse) {

        short waterLevel = getActualWaterLvl(dataRequest);
        dataResponse.setActualWaterLevel(waterLevel);
        dataResponse.setWaterNotificationMessage(getWaterNotification(waterLevel, dataResponse));
        dataResponse.setScale(setScale(waterLevel));
    }

    private static String getWaterNotification(int waterLevel, DataResponse dataResponse) {
        dataResponse.setWaterNotifyFlag((byte)(0));
        String message = "";
        if (waterLevel >= ConstFlags.TANK_FULL && ConstFlags.full != 1) {
            ConstFlags.full = 1;
            message = "!ВОДА! -- Бак полон";
            dataResponse.setWaterNotifyFlag((byte)(1));
        } else if (waterLevel == (ConstFlags.TANK_FULL - 4)) {
            ConstFlags.full = 0;
        }

        if (waterLevel == ConstFlags.TANK_HALF && ConstFlags.half != 1) {
            ConstFlags.half = 1;
            message = "!ВОДА! -- Пол бака";
            dataResponse.setWaterNotifyFlag((byte)(1));
        } else if (waterLevel == (ConstFlags.TANK_HALF - 4)) {
            ConstFlags.half = 0;
        } else if ((waterLevel == (ConstFlags.TANK_HALF + 4)) && (ConstFlags.half == 1)) {
            ConstFlags.half = 0;
        }

        if ((waterLevel == ConstFlags.TANK_EMPTY) && (ConstFlags.empty != 1)) {
            ConstFlags.empty = 1;
            message = "!ВОДА! -- !НИЗКИЙ УРОВЕНЬ ВОДЫ!";
            dataResponse.setWaterNotifyFlag((byte)(1));
        } else if ((waterLevel == (ConstFlags.TANK_EMPTY + 4)) && (ConstFlags.empty == 1)) {
            ConstFlags.empty = 0;
        }
        return message;
    }

     static short getActualWaterLvl(DataRequest dataRequest) {
        return (short) (dataRequest.getOutRawWaterData() != 0 ?
               ConstFlags.TANK_HEIGHT - dataRequest.getOutRawWaterData() : 0);
    }

    private static float setScale(int waterLevel) {
        float scale = 0;
        short[] levelsArray = {138, 129, 124, 116, 110, 102, 97,
                                89, 82, 75, 70, 68, 61, 56, 47, 42};

        float[] scalesArray = {9, 8.5f, 8, 7.5f, 7, 6.5f, 6, 5.5f, 5,
                               4.5f, 4, 3.5f, 3, 2.5f, 2};

        for (int i = 0; i <= levelsArray.length - 3; i++) {
            if (waterLevel < levelsArray[i] && waterLevel > levelsArray[i + 1]) {
                return scalesArray[i + 1];
            } else if (waterLevel == levelsArray[i]) {
                return scalesArray[i];
            }
        }

        return scale;
    }
}
