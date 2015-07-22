#include <avr/power.h>
#include <Wire.h>

#define PIN 6
#define BUF_LEN 96
#define I2C_LEN 32
#define WIRE_ADDR 6
#define PIN_LATCH_1 2
#define PIN_DOOR_1  3
#define PIN_LATCH_2 4
#define PIN_DOOR_2  5

unsigned long seconds_since_reset = 0;
int wire_count = 0;
char wire_buffer[BUF_LEN+1] = {0};
char tx_buffer[I2C_LEN+1] = {0};
char latch_trigger[2] = {0,0};
char latch_triggered[2] = {0,0};
char doors = 0;

void transmitComms() {
  int offset = wire_count * I2C_LEN;
  for (int i=0; i < I2C_LEN; i++) tx_buffer[i] = '\0';
  for (int i=0; i < I2C_LEN; i++) {
    tx_buffer[i] = wire_buffer[i+offset];
  }
  Wire.write(tx_buffer);
  if (wire_count == 2) wire_count = 0;
  else wire_count++;
}

void load_status() {
  wire_count = 0;
  
  // BEGIN TIME TRANSMIT
  strcpy(wire_buffer, "TIME=");
  unsigned long seconds = millis();
  seconds = seconds / 1000;
  seconds = seconds - seconds_since_reset;
  int hours = seconds / 3600;
  int mins = ((seconds / 60) - (hours * 60)) % 3600;
  seconds = seconds - (hours * 3600) - (mins * 60);
  char tmp[4] = "";
  sprintf(tmp, "%d", hours);
  if (hours <= 10) strcat(wire_buffer, "0");
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ":");
  if (mins <= 10) strcat(wire_buffer, "0");
  sprintf(tmp, "%d", mins);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ":");
  if (seconds <= 10) strcat(wire_buffer, "0");
  sprintf(tmp, "%d", seconds);
  strcat(wire_buffer, tmp);
  // END OF TIME TRANSMIT
  strcat(wire_buffer, ",LATCH_1=");
  sprintf(tmp, "%d", latch_triggered[0]);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",DOOR_1=");
  if (doors & 1) strcat(wire_buffer, "1");
  else strcat(wire_buffer, "0");
  strcat(wire_buffer, ",LATCH_2=");
  sprintf(tmp, "%d", latch_triggered[1]);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",DOOR_2=");
  if (doors & 2) strcat(wire_buffer, "1");
  else strcat(wire_buffer, "0");
  
  // TERMINATE THE WIREBUFFER
  strcat(wire_buffer, 0);
}

void receiveComms(int howMany) {
  String message;
  digitalWrite(13,1);
  int i;
  for (i=0; i<BUF_LEN; i++) wire_buffer[i] = '\0';
  i = 0;
  while (Wire.available() > 0) {
    wire_buffer[i] = Wire.read();
    i++;
  }
  message = String(wire_buffer);
  if (strcmp(wire_buffer, "*IDN?") == 0) strcpy(wire_buffer,"Wall Switch Controller");
  else if (strcmp(wire_buffer, "*STAT?") == 0) load_status();
  else if (strcmp(wire_buffer, "*RST") == 0) {
    seconds_since_reset = millis() / 1000;
    latch_trigger[0] = 0;
    latch_trigger[1] = 0;
    latch_triggered[0] = 0;
    latch_triggered[1] = 0;
    load_status();
  }
  else if (message.indexOf("*TRIG=") != -1) {
    char state;
    state = message.substring(6).toInt();
    if (state & 1) latch_trigger[0]++;
    if (state & 2) latch_trigger[1]++;
    load_status();
  }
  else strcpy(wire_buffer,"Unknown command");
}

void setup() {
  Serial.begin(9600);
  Wire.begin(WIRE_ADDR);
  Wire.onReceive(receiveComms);
  Wire.onRequest(transmitComms);
  pinMode(PIN_LATCH_1, OUTPUT);
  pinMode(PIN_DOOR_1, INPUT_PULLUP);
  pinMode(PIN_LATCH_2, OUTPUT);
  pinMode(PIN_DOOR_2, INPUT_PULLUP);
}

void trigger_latch() {
  if (latch_trigger[0] > latch_triggered[0]) {
    digitalWrite(PIN_LATCH_1, 1);
    latch_triggered[0]++;
  }
  if (latch_trigger[1] > latch_triggered[1]) {
    digitalWrite(PIN_LATCH_2, 1);
    latch_triggered[1]++;
  }
  delay(1800);
  digitalWrite(PIN_LATCH_1, 0);
  digitalWrite(PIN_LATCH_2, 0);
}

void loop() {
  if (digitalRead(PIN_DOOR_1)) doors &= 1;
  else doors |= ~1;
  if (digitalRead(PIN_DOOR_2)) doors &= 2;
  else doors |= ~2;
  if (latch_trigger[0] > latch_triggered[0] || latch_trigger[1] > latch_triggered[1]) trigger_latch();
}

