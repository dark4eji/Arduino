#define SENSPIN PB3
#define RELAY PB2 
#define GERK PB1
#define LMOIST 380
#define HMOIST 330 

void setup() {  
  //Serial.begin(9600);
  pinMode(RELAY, OUTPUT);
  pinMode(SENSPIN, INPUT);
  digitalWrite(RELAY, HIGH);  
}

void loop() {
  int moisture = analogRead(SENSPIN); 
  //Serial.println(moisture); 
  if (digitalRead(GERK) == 0) {    
    if (moisture >= LMOIST) {
      digitalWrite(RELAY, LOW);
    } else if (moisture <= HMOIST) {
      digitalWrite(RELAY, HIGH);
    } 
  } 
  delay(1000);
}
