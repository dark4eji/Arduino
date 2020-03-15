package classes;

import models.DataRequest;
import models.DataResponse;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@RestController
public class DataController {

    @RequestMapping(value = "/sendData", method = RequestMethod.POST)
    public ResponseEntity<DataResponse> sendData(@RequestBody DataRequest dataRequest) {
        DataResponse dataResponse = new DataResponse();
        compTemp(dataRequest, dataResponse);
        return ResponseEntity.ok(dataResponse);
    }

    private void compTemp(DataRequest dataRequest, DataResponse dataResponse) {
        int temper1 = dataRequest.getTemper1();
        if (temper1 <= 20) {
            dataResponse.setTempResult1("FeelBadman");
        }
    }
}
