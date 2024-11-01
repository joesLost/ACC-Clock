// Define pins
#define DIR_PIN 19      // Direction pin connected to DIR+ on both DM542

void setup() {
  // Initialize the DIR pin as output
  pinMode(DIR_PIN, OUTPUT);
}

void loop() {
  // Generate pulses on the DIR pin
  digitalWrite(DIR_PIN, HIGH);
  delayMicroseconds(20); // 20 µs pulse width
  digitalWrite(DIR_PIN, LOW);
  delayMicroseconds(80); // Adjust as needed for pulse frequency testing
}
