#include <Wire.h>

#define BUF_LEN 96
#define I2C_LEN 32
#define WIRE_ADDR 12

#define PIN_BLACK   A1
#define PIN_GREEN   A2
#define PIN_YELLOW  A3
#define PIN_RED     10
#define PIN_LED     2
#define PIN_IN      8
#define PIN_OUT     9

#define TRIGGER_THRESHOLD 1000

uint8_t wires_state;
uint16_t trigger_out;

const char device_name[] = "Coloured Wires";
unsigned long seconds_since_reset = 0;
int wire_count = 0;
char wire_buffer[BUF_LEN+1] = {0};
char tx_buffer[I2C_LEN+1] = {0};

void transmitComms() {
  int offset = wire_count * I2C_LEN;
  for (int i=0; i < I2C_LEN; i++) tx_buffer[i] = '\0';
  task();
  for (int i=0; i < I2C_LEN; i++) {
    tx_buffer[i] = wire_buffer[i+offset];
  }
  task();
  Wire.write(tx_buffer);
  task();
  if (wire_count == 2) wire_count = 0;
  else wire_count++;
  task();
}


void load_status() {
  wire_count = 0;
  
  // BEGIN TIME TRANSMIT
  strcpy(wire_buffer, "TIME=");
  task();
  unsigned long seconds = millis();
  seconds = seconds / 1000;
  seconds = seconds - seconds_since_reset;
  int hours = seconds / 3600;
  int mins = ((seconds / 60) - (hours * 60)) % 3600;
  seconds = seconds - (hours * 3600) - (mins * 60);
  char tmp[4] = "";
  task();
  sprintf(tmp, "%d", hours);
  if (hours <= 10) strcat(wire_buffer, "0");
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ":");
  task();
  if (mins <= 10) strcat(wire_buffer, "0");
  sprintf(tmp, "%d", mins);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ":");
  task();
  if (seconds <= 10) strcat(wire_buffer, "0");
  sprintf(tmp, "%d", seconds);
  strcat(wire_buffer, tmp);
  // END OF TIME TRANSMIT
  task();
  
  strcat(wire_buffer, ",WIRES=");
  sprintf(tmp, "%d", wires_state);
  strcat(wire_buffer, tmp);
  task();
  
  strcat(wire_buffer, ",OUTPUT=");
  if (trigger_out > TRIGGER_THRESHOLD) strcat(wire_buffer, "1");
  else strcat(wire_buffer, "0");
  task();
  
  // TERMINATE THE WIREBUFFER
  strcat(wire_buffer, 0);
}

void receiveComms(int howMany) {
  String message;
  int i;
  for (i=0; i<BUF_LEN; i++) wire_buffer[i] = '\0';
  task();
  i = 0;
  while (Wire.available() > 0) {
    task();
    wire_buffer[i] = Wire.read();
    i++;
  }
  task();
  message = String(wire_buffer);
  if (strcmp(wire_buffer, "*IDN?") == 0) strcpy(wire_buffer,device_name);
  else if (strcmp(wire_buffer, "*STAT?") == 0) load_status();
  else if (strcmp(wire_buffer, "*RST") == 0) {
    seconds_since_reset = millis() / 1000;
    trigger_out = 0;
    load_status();
  }
  else strcpy(wire_buffer,"Unknown command");
  task();
}

void setup() {
  Serial.begin(9600);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BLACK, INPUT_PULLUP);
  pinMode(PIN_GREEN, INPUT_PULLUP);
  pinMode(PIN_YELLOW, INPUT_PULLUP);
  pinMode(PIN_RED, INPUT_PULLUP);
  pinMode(PIN_IN, INPUT_PULLUP);
  pinMode(PIN_OUT, OUTPUT);

  Wire.begin(WIRE_ADDR);
  Wire.onReceive(receiveComms);
  Wire.onRequest(transmitComms);
  trigger_out = 0;
}

void task() {
  if (trigger_out > TRIGGER_THRESHOLD) digitalWrite(PIN_OUT, digitalRead(PIN_IN));
}

void check_wires_state() {
  // Avoid resetting the wires_state variable to prevent
  // glitches when transmitting the state due to a SPI
  // read during the update event.
  task();
  if (digitalRead(PIN_BLACK) == 0) wires_state |= 0b0001;
  else wires_state &= 0b1110;
  task();
  if (digitalRead(PIN_GREEN) == 0) wires_state |= 0b0010;
  else wires_state &= 0b1101;
  task();
  if (digitalRead(PIN_YELLOW) == 0) wires_state |= 0b0100;
  else wires_state &= 0b1011;
  task();
  if (digitalRead(PIN_RED) == 0) wires_state |= 0b1000;
  else wires_state &= 0b0111;
}

void check_wires() {
  // Check the state and set trigger variable appropriately
  check_wires_state();
  if (wires_state == 15 && trigger_out < (TRIGGER_THRESHOLD + 1)) trigger_out++;
  task();
}

void loop() {
  check_wires();
  // Update outputs based on value of trigger variable
  if (trigger_out > TRIGGER_THRESHOLD) {
    digitalWrite(PIN_LED, 1);
  }
  else {
    digitalWrite(PIN_LED, 0);
    digitalWrite(PIN_OUT, 1);
  }
  task();
}



