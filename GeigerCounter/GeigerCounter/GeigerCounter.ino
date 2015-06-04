#define PIN_DIAL_ZERO 8
#define PIN_DIAL_X100 9
#define PIN_DIAL_X10  10
#define PIN_DIAL_X1   11
#define PIN_DIAL_X01  12
#define PIN_GROUND    4
#define PIN_METER     6


void setup() {
  // initialize digital pin 13 as an output.
  pinMode(PIN_DIAL_X01, INPUT_PULLUP);
  pinMode(PIN_DIAL_X1, INPUT_PULLUP);
  pinMode(PIN_DIAL_X10, INPUT_PULLUP);
  pinMode(PIN_DIAL_X100, INPUT_PULLUP);
  pinMode(PIN_DIAL_ZERO, INPUT_PULLUP);
  pinMode(A0, INPUT_PULLUP);
  pinMode(PIN_GROUND, OUTPUT);
  digitalWrite(PIN_GROUND, LOW);
  pinMode(13, OUTPUT);
  Serial.begin(9600);
}

int total = 0;
// the loop function runs over and over again forever
void loop() {
  int inc = 0;
  int dial_adjust = 0;
  int out_value = 0;
  dial_adjust += analogRead(A0);
  delay(1);
  dial_adjust += analogRead(A0);
  delay(1);
  dial_adjust += analogRead(A0);
  delay(1);
  dial_adjust += analogRead(A0);
  delay(1);
  dial_adjust += analogRead(A0);
  delay(1);
  dial_adjust += analogRead(A0);
  delay(1);
  dial_adjust += analogRead(A0);
  delay(1);
  dial_adjust += analogRead(A0);
  delay(1);
  dial_adjust += analogRead(A0);
  delay(1);
  dial_adjust += analogRead(A0);
  dial_adjust /= 10;
  dial_adjust = 310 - dial_adjust;
  if (dial_adjust < 0) dial_adjust = 0;
  else if (dial_adjust > 255) dial_adjust = 255;

  if (digitalRead(PIN_DIAL_ZERO) == 0) out_value = 0;
  else if (digitalRead(PIN_DIAL_X100) == 0) out_value = 200;
  else if (digitalRead(PIN_DIAL_X10) == 0) out_value = 100;
  else if (digitalRead(PIN_DIAL_X1) == 0) out_value = 50;
  else if (digitalRead(PIN_DIAL_X01) == 0) out_value = 25;
  else out_value = -1;

  if (out_value >= 0) {
    out_value += dial_adjust;
    Serial.println(out_value);
    if (out_value > 255) out_value = 255;
    else if (out_value < 0) out_value = 0;
    analogWrite(PIN_METER, out_value);
  }
  else analogWrite(PIN_METER, 0);


}
