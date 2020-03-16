package resources.models;

import lombok.Data;

@Data
public class DataResponse {
    private float inTempUpperSensor;
    private float inTempLowerSensor;
    private short inHumUpperSensor;
    private short inHumLowerSensor;
    private short ActualWaterLevel;
    private byte compressorFlag; //Is compressor active or nor (0/1)
    private float scale; //Water level in scales
    private byte WaterNotifyFlag; //Denotes should water lvl notification be send or not
    private String WaterNotificationMessage;
}
