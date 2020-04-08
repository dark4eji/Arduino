package models;

import lombok.Data;

@Data
public class DataResponse {
    private float inTempUpperSensor;
    private float inTempLowerSensor;
    private short inHumUpperSensor;
    private short inHumLowerSensor;
    private short actualWaterLevel; //Water level after processing
    private float scale; //Water level in scales
    private byte waterNotifyFlag; //Denotes should water lvl notification be send or not
    private String waterNotificationMessage;
    private byte stateFlag; //Current button state
   // private short[] ledState;
}
