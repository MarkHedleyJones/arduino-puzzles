#include <stdlib.h>

#define PIN_DIAL_ZERO 8
#define PIN_DIAL_X100 9
#define PIN_DIAL_X10  10
#define PIN_DIAL_X1   11
#define PIN_DIAL_X01  12
#define PIN_GROUND    4
#define PIN_METER     6
#define PIN_LED       13
#define PIN_BATT      A5
#define PIN_CAL_POT   A0
#define PIN_RX_SIG    A3
#define AVGS          1

// Resistor to the display
// 68k + 10k + 6.8k = 84.8k (driven at 5v)


void setup() {
  pinMode(PIN_DIAL_X01, INPUT_PULLUP);
  pinMode(PIN_DIAL_X1, INPUT_PULLUP);
  pinMode(PIN_DIAL_X10, INPUT_PULLUP);
  pinMode(PIN_DIAL_X100, INPUT_PULLUP);
  pinMode(PIN_DIAL_ZERO, INPUT_PULLUP);
  pinMode(PIN_CAL_POT, INPUT_PULLUP);
  pinMode(PIN_RX_SIG, INPUT);
  pinMode(PIN_GROUND, OUTPUT);
  pinMode(PIN_BATT, INPUT);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_GROUND, LOW);
  Serial.begin(9600);
}

unsigned int get_dial_setting() {
  int out = 0;
  out += analogRead(PIN_CAL_POT);
  delay(1);
  out += analogRead(PIN_CAL_POT);
  delay(1);
  out += analogRead(PIN_CAL_POT);
  delay(1);
  out += analogRead(PIN_CAL_POT);
  delay(1);
  out += analogRead(PIN_CAL_POT);
  delay(1);
  out += analogRead(PIN_CAL_POT);
  delay(1);
  out += analogRead(PIN_CAL_POT);
  delay(1);
  out += analogRead(PIN_CAL_POT);
  delay(1);
  out += analogRead(PIN_CAL_POT);
  delay(1);
  out += analogRead(PIN_CAL_POT);
  out /= 10;
  out = 310 - out;
  if (out < 0) out = 0;
  else if (out > 255) out = 255;
  return out;
}

unsigned int get_multiplier_setting() {
  unsigned int out = 0;
  if (digitalRead(PIN_DIAL_ZERO) == 0) out = 0;
  else if (digitalRead(PIN_DIAL_X100) == 0) out = 8;
  else if (digitalRead(PIN_DIAL_X10) == 0) out = 4;
  else if (digitalRead(PIN_DIAL_X1) == 0) out = 2;
  else if (digitalRead(PIN_DIAL_X01) == 0) out = 1;
  return out;
}

unsigned int get_signal_strength() {
  int out = analogRead(PIN_RX_SIG);
  out -= 4;
  if (out < 0) out = 0;
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
  sig_strength /= 4;
  output = sig_strength * multiplier;
  Serial.println(sig_strength);
  output += offset;
  if (output > 255) output = 255;
  if (output < 0) output = 0;
  analogWrite(PIN_METER, output);
  delay(100);
}