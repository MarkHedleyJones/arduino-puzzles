
// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  DDRB = B00110000;
}

// the loop routine runs over and over again forever:
void loop() {
  PORTB = 0b00100000;
  delayMicroseconds(12);
  PORTB = 0b00010000;
  delayMicroseconds(12);
}
