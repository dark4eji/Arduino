package logic;

import models.DataRequest;
import models.DataResponse;

public class ModulesHandler {
    public static void handleModules(DataRequest dataRequest,
                                     DataResponse dataResponse) {
        dataResponse.setRelayPermission((byte) 0);
        short waterLevel = WaterHandler.getActualWaterLvl(dataRequest);
        shutRelay(dataRequest, dataResponse, waterLevel);
        manageBlynkButton(dataRequest, dataResponse, waterLevel);
    }

    private static void shutRelay(DataRequest dataRequest,
                                  DataResponse dataResponse, short waterLevel) {

        if (waterLevel >= ConstFlags.TANK_FULL && ConstFlags.compressor == 1
                && dataRequest.getOutRawWaterData() != 0) {
            ConstFlags.compressor = 0;
            dataResponse.setRelayPermission((byte) 1);
            dataResponse.setStateFlag((byte) 0);
        }
    }

    private static void manageBlynkButton(DataRequest dataRequest,
                                          DataResponse dataResponse, short waterLevel) {
        if (dataRequest.getBlynkButtonState() == 1 && ConstFlags.compressor == 0
                && waterLevel < ConstFlags.TANK_FULL) {
            ConstFlags.compressor = 1;
            ConstFlags.ledState[0] = 255;
            dataResponse.setStateFlag((byte) 1);
            dataResponse.setRelayPermission((byte) 1);
        } else if (dataRequest.getBlynkButtonState() == 0 && ConstFlags.compressor == 1) {
            ConstFlags.compressor = 0;
            dataResponse.setStateFlag((byte) 0);
            ConstFlags.ledState[0] = 0;
            dataResponse.setRelayPermission((byte) 1);
        }
    }
}
