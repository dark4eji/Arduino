package classes;

import logic.ConditionsHandler;
import logic.ConstFlags;
import logic.ModulesHandler;
import logic.WaterHandler;
import models.DataRequest;
import models.DataResponse;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@RestController
public class DataController {
    private DataResponse dataResponse = new DataResponse();

    @RequestMapping(value = "/data", method = RequestMethod.POST)
    public ResponseEntity<DataResponse> sendData(@RequestBody DataRequest dataRequest) {

        WaterHandler.handleWaterData(dataRequest, this.dataResponse);
        ModulesHandler.handleModules(dataRequest, this.dataResponse);
        ConditionsHandler.getTemperatureData(dataRequest, this.dataResponse);
        dataResponse.setLedState(ConstFlags.ledState);

        return ResponseEntity.ok(this.dataResponse);
    }

    @RequestMapping(value = "/data", method = RequestMethod.GET)
    public ResponseEntity<DataResponse> sendData() {
        return ResponseEntity.ok(this.dataResponse);
    }
}
