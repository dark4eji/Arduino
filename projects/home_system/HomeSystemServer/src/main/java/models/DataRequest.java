package models;

import lombok.Data;

@Data
public class DataRequest {
    private float outTempUpperSensor;
    private float outTempLowerSensor;
    private short outHumUpperSensor;
    private short outHumLowerSensor;
    private short outRawWaterData;
    private byte blynkButtonState;
    //private byte[] moduleState;
}
