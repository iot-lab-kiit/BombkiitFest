
const int clapanalog = A1; 
//const int clapdigital = 2;
int clapState = 0; 
//int threshold = 350;

void setup() {
  pinMode(clapanalog, INPUT);
  //pinMode(clapdigital, INPUT);
  Serial.begin(9600);
}

void loop() {
  clapState = analogRead(clapanalog); 
  //clapState = analogRead(clapdigital);
  Serial.println(clapState);
  delay(10);

  // if (clapState >= threshold) {
  //   digitalWrite(ledPin, HIGH);  // Turn on the LED
  //   delay(10);  // Debounce delay
  // } else {
  //   digitalWrite(ledPin, LOW);  // Turn off the LED
  // }
}
