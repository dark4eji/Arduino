package resources.logic;

import resources.models.DataRequest;
import resources.models.DataResponse;

public class DataHandler {
    private DataRequest dataRequest;
    private DataResponse dataResponse;
    private WaterHandler waterHandler;
    private ConditionsHandler conditionsHandler;

    public DataHandler(DataRequest dataRequest, DataResponse dataResponse) {
        this.dataRequest = dataRequest;
        this.dataResponse = dataResponse;
        this.waterHandler = new WaterHandler(this.dataRequest, this.dataResponse);
        this.conditionsHandler = new ConditionsHandler(this.dataRequest, this.dataResponse);

    }

    public void handleData() {
        waterHandler.handleWaterData();
        conditionsHandler.handleTemperatureData();
    }
}
