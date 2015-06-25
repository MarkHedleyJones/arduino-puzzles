#define PIN_DIAL_ZERO 8
#define PIN_DIAL_X100 9
#define PIN_DIAL_X10  10
#define PIN_DIAL_X1   11
#define PIN_DIAL_X01  12
#define PIN_GROUND    4
#define PIN_METER     6
#define PIN_LED       13
#define PIN_BATT      A5
#define AVGS          24

unsigned int strength[AVGS] = {0};
unsigned char index = 0;

void setup() {
  // initialize digital pin 13 as an output.
  pinMode(PIN_DIAL_X01, INPUT_PULLUP);
  pinMode(PIN_DIAL_X1, INPUT_PULLUP);
  pinMode(PIN_DIAL_X10, INPUT_PULLUP);
  pinMode(PIN_DIAL_X100, INPUT_PULLUP);
  pinMode(PIN_DIAL_ZERO, INPUT_PULLUP);
  pinMode(A0, INPUT_PULLUP);
  pinMode(PIN_GROUND, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, INPUT);
  pinMode(PIN_BATT, INPUT);
  digitalWrite(2, LOW);
  digitalWrite(PIN_GROUND, LOW);
  pinMode(13, OUTPUT);
  Serial.begin(9600);
  index = 0;
}

unsigned int get_dial_setting() {
  int out = 0;
  out += analogRead(A0);
  delay(1);
  out += analogRead(A0);
  delay(1);
  out += analogRead(A0);
  delay(1);
  out += analogRead(A0);
  delay(1);
  out += analogRead(A0);
  delay(1);
  out += analogRead(A0);
  delay(1);
  out += analogRead(A0);
  delay(1);
  out += analogRead(A0);
  delay(1);
  out += analogRead(A0);
  delay(1);
  out += analogRead(A0);
  out /= 10;
  out = 310 - out;
  if (out < 0) out = 0;
  else if (out > 255) out = 255;
  return out;
}

unsigned int get_multiplier_setting() {
  unsigned int out = 0;
  if (digitalRead(PIN_DIAL_ZERO) == 0) out = 0;
  else if (digitalRead(PIN_DIAL_X100) == 0) out = 1000;
  else if (digitalRead(PIN_DIAL_X10) == 0) out = 100;
  else if (digitalRead(PIN_DIAL_X1) == 0) out = 10;
  else if (digitalRead(PIN_DIAL_X01) == 0) out = 1;
  return out;
}

unsigned int get_signal_strength() {
  unsigned int out = 0;
  out = 98 - pulseIn(3, LOW, 200);

  if (out > 10000) out = 0; // Overflowed

  out = out * 2.602;
  out += 1;
  if (out > 255) out = 255; // Trim to max

  strength[index] = out;

  out = 0;
  for (int i=0; i<AVGS; i++) out += strength[i];
  out /= AVGS;
  index++;
  if (index >= AVGS) index = 0;

  return out;
}

void set_needle_position(unsigned int input) {
  analogWrite(PIN_METER, input);
}

float get_battery_voltage() {
  return analogRead(PIN_BATT) * 0.017994723;
}


void loop() {

  unsigned int sig_strength, offset, multiplier, output;

  sig_strength = get_signal_strength();
  offset = get_dial_setting();
  multiplier = get_multiplier_setting();

  output = sig_strength * multiplier;
  output /= 20;
  output += offset;
  if (output > 190) output = 190;
  if (output < 0) output = 0;

  analogWrite(PIN_METER, output);
  Serial.println(get_battery_voltage());
  delay(200);
//  Serial.write("test");
}
