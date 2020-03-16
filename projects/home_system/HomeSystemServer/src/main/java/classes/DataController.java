package classes;

import com.fasterxml.jackson.databind.ObjectMapper;
import resources.logic.DataHandler;
import resources.models.DataRequest;
import resources.models.DataResponse;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@RestController
public class DataController {
    private DataResponse dataResponse = new DataResponse();
    private DataRequest dataRequest = new DataRequest();
    private DataHandler dataHandler = new DataHandler(this.dataRequest, this.dataResponse);


    @RequestMapping(value = "/data", method = RequestMethod.POST)
    public ResponseEntity<DataResponse> sendData(@RequestBody DataRequest dataRequest) {

        this.dataRequest.setOutTempUpperSensor(dataRequest.getOutTempUpperSensor());
        this.dataRequest.setOutTempLowerSensor(dataRequest.getOutTempLowerSensor());

        this.dataRequest.setOutHumUpperSensor(dataRequest.getOutHumUpperSensor());
        this.dataRequest.setOutHumLowerSensor(dataRequest.getOutHumLowerSensor());

        this.dataRequest.setOutRawWaterData(dataRequest.getOutRawWaterData());
        this.dataRequest.setPinFlag(dataRequest.getPinFlag());

        dataHandler.handleData();
        return ResponseEntity.ok(this.dataResponse);
    }

    @RequestMapping(value = "/data", method = RequestMethod.GET)
    public ResponseEntity<DataResponse> sendData() {
        return ResponseEntity.ok(this.dataResponse);
    }
}
