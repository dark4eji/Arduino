package resources.models;

import lombok.Data;
import javax.validation.constraints.NotNull;

@Data
public class DataRequest {
    @NotNull(message = "temper1 should not be empty")
    private float outTempUpperSensor;
    @NotNull(message = "temper2 should not be empty")
    private float outTempLowerSensor;
    private short outHumUpperSensor;
    private short outHumLowerSensor;
    private byte pinFlag;
    private short outRawWaterData;
}
