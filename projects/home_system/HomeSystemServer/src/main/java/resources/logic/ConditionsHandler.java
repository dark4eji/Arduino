package resources.logic;

import resources.models.DataRequest;
import resources.models.DataResponse;

public class ConditionsHandler {
    private DataRequest dataRequest;
    private DataResponse dataResponse;

    public ConditionsHandler(DataRequest dataRequest,
                             DataResponse dataResponse) {
        this.dataRequest = dataRequest;
        this.dataResponse = dataResponse;
    }

    public void handleTemperatureData() {
        float temperature1 = dataRequest.getOutTempUpperSensor() != 0 ?
                dataRequest.getOutTempUpperSensor() - 1.6f : 0;
        float temperature2 = dataRequest.getOutTempLowerSensor() != 0 ?
                dataRequest.getOutTempLowerSensor() - 0.9f : 0;

        short humidity1 = (short) (dataRequest.getOutHumUpperSensor() != 0 ?
                        dataRequest.getOutHumUpperSensor() + 4.0f : 0);
        short humidity2 = (short) (dataRequest.getOutHumLowerSensor() != 0 ?
                        dataRequest.getOutHumLowerSensor() - 3.0f : 0);

        dataResponse.setInTempUpperSensor(temperature1);
        dataResponse.setInTempLowerSensor(temperature2);

        dataResponse.setInHumUpperSensor(humidity1);
        dataResponse.setInHumLowerSensor(humidity2);
    }
}
