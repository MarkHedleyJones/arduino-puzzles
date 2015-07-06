#include <Wire.h>
#include <stdlib.h>

#define BUF_LEN 96
#define I2C_LEN 32
#define MSG_LEN 20
#define PIN_LED 13
#define PIN_MORSE 12
#define I2C_ADDR 7
#define PIN_SWITCH 7

unsigned int rx_count = 0;
unsigned long rx_time;
int located = 0;
float battvolt = -1.0;
String readBuf;
unsigned long seconds_since_reset = 0;
int wire_count = 0;
char wire_buffer[BUF_LEN+1] = {0};
char tx_buffer[I2C_LEN+1] = {0};
char rx_buffer[5] = {0};
char message_buffer[MSG_LEN] = {0};

void setup() {
  pinMode(PIN_MORSE, INPUT);
  pinMode(13, OUTPUT);
  pinMode(PIN_SWITCH,INPUT_PULLUP); 
  digitalWrite(13, LOW);
  Serial.begin(9600);
  Wire.begin(I2C_ADDR);
  Wire.onReceive(receiveComms);
  Wire.onRequest(transmitComms);
}

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
  if (digitalRead(PIN_SWITCH)) located = 1;
  wire_count = 0;
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
  strcat(wire_buffer, ",OPENED=");
  sprintf(tmp, "%d", located);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",OPEN=");
  sprintf(tmp, "%d", digitalRead(PIN_SWITCH));
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",RX_COUNT=");
  sprintf(tmp, "%d", rx_count);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",RX_SIGNAL=");
  if ((millis() - rx_time) < 300) strcat(wire_buffer, "1");
  else strcat(wire_buffer, "0");
  strcat(wire_buffer, ",BATTVOLT=");
//  sprintf(tmp, "%d", tempor);
  dtostrf(battvolt, 2, 2, tmp);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, 0);
}

void receiveComms(int howMany) {
  int i;
  for (i=0; i<BUF_LEN; i++) wire_buffer[i] = '\0';
  i = 0;
  while (Wire.available() > 0) {
    wire_buffer[i] = Wire.read();
    i++;
  }
  if (strcmp(wire_buffer, "*IDN?") == 0) strcpy(wire_buffer,"Geiger Counter");
  else if (strcmp(wire_buffer, "*STAT?") == 0) load_status();
  else if (strcmp(wire_buffer, "*RST") == 0) {
    seconds_since_reset = millis() / 1000;
    rx_count = 0;
    located = 0;
    battvolt = -1.0;
    load_status();
  }
  else strcpy(wire_buffer, "Unknown command");
}

int i = 0;

void loop() {
  if (Serial.available()) {
    delay(100);
    rx_time = millis();
    if (Serial.available() == 6) {
      battvolt = Serial.parseFloat();
      rx_count++;
    }
    else {
      while (Serial.available()) Serial.read();
    }
  }
  if (digitalRead(PIN_SWITCH)) located = 1;
}
