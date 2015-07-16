#include <Wire.h>

#define PIN_TRIG 12
#define PIN_LED 13
#define BUF_LEN 96
#define I2C_LEN 32
#define WIRE_ADDR 9
#define DEFAULT_COMBO 2178

unsigned int combination;
unsigned int state;
bool triggered;
unsigned long seconds_since_reset = 0;
int wire_count = 0;
char wire_buffer[BUF_LEN+1] = {0};
char tx_buffer[I2C_LEN+1] = {0};

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
  
  strcat(wire_buffer, ",TRIG=");
  sprintf(tmp, "%d", triggered);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",COMBO=");
  sprintf(tmp, "%u", combination,3);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",STATE=");
  sprintf(tmp, "%u", state,3);
  strcat(wire_buffer, tmp);  
//  // TERMINATE THE WIREBUFFER
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
  if (strcmp(wire_buffer, "*IDN?") == 0) strcpy(wire_buffer,"Telephone Exchange");
  else if (strcmp(wire_buffer, "*STAT?") == 0) load_status();
  else if (strcmp(wire_buffer, "*RST") == 0) {
    seconds_since_reset = millis() / 1000;
    combination = DEFAULT_COMBO;
    state = read_state();
    triggered = 0;
    load_status();
  }
  else if (message.indexOf("*COMBO=") != -1) {
    // Shift the number to the start
    combination = message.substring(7).toInt();
    load_status();
  }
  else if (message.indexOf("*TRIG") != -1) {
    triggered = 1;
    load_status();
  }
  else strcpy(wire_buffer,"Unknown command");
}

void setup() {
  // put your setup code here, to run once:
  for (int i = 0; i < 12; i++) pinMode(i, INPUT_PULLUP);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_TRIG,1);
  digitalWrite(PIN_LED,0);
  pinMode(13, OUTPUT);
  Wire.begin(WIRE_ADDR);
  Wire.onReceive(receiveComms);
  Wire.onRequest(transmitComms);
  combination = DEFAULT_COMBO;
  read_state();
}

unsigned int read_state() {
  state = 0;
  for (int i = 0; i < 12; i++) if (digitalRead(i) == 0) state |= 1 << i;
  if (state == combination) triggered = 1;
  if (triggered) {
    digitalWrite(PIN_TRIG,0);
    digitalWrite(PIN_LED,1);
  }
  return state;
}

void loop() {
  read_state();
  delay(100);
}
