#define SENSPIN PB3
#define RELAY PB4
#define LMOIST 590
#define HMOIST 320 

void setup() {
  pinMode(RELAY, OUTPUT);
  pinMode(SENSPIN, INPUT);
  digitalWrite(RELAY, HIGH);  
}

void loop() {
  int moisture = analogRead(SENSPIN);  
  if (moisture >= LMOIST) {
    digitalWrite(RELAY, LOW);
  } else if (moisture > HMOIST && moisture < LMOIST) {
    digitalWrite(RELAY, HIGH);
  } else if (moisture == 0) {
    digitalWrite(RELAY, LOW);  
  }
  delay(2000);
}
