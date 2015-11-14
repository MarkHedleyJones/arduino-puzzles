#include <avr/power.h>
#include <Wire.h>

#define PIN 6
#define BUF_LEN 96
#define I2C_LEN 32
#define WIRE_ADDR 2

#define PIN_L0 A0
#define PIN_L1 A1
#define PIN_L2 A2
#define PIN_L3 A3

#define THRESHOLD 300

const char device_name[] = "Locker Door Sensors";
unsigned long seconds_since_reset = 0;
int wire_count = 0;
char wire_buffer[BUF_LEN+1] = {0};
char tx_buffer[I2C_LEN+1] = {0};
int light[4] = {0};
int light_max[4] = {0};

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
  uint8_t calc = 0;
  strcat(wire_buffer, ",L1=");
  calc |= (light[0] > THRESHOLD);
  calc |= (light_max[0] > THRESHOLD) << 1;
  sprintf(tmp, "%d", calc);
  strcat(wire_buffer, tmp);

  calc = 0;
  strcat(wire_buffer, ",L2=");
  calc |= (light[1] > THRESHOLD);
  calc |= (light_max[1] > THRESHOLD) << 1;
  sprintf(tmp, "%d", calc);
  strcat(wire_buffer, tmp);
  
  calc = 0;
  strcat(wire_buffer, ",L3=");
  calc |= (light[2] > THRESHOLD);
  calc |= (light_max[2] > THRESHOLD) << 1;
  sprintf(tmp, "%d", calc);
  strcat(wire_buffer, tmp);
  
  calc = 0;
  strcat(wire_buffer, ",L4=");
  calc |= (light[3] > THRESHOLD);
  calc |= (light_max[3] > THRESHOLD) << 1;
  sprintf(tmp, "%d", calc);
  strcat(wire_buffer, tmp);
  
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
  if (strcmp(wire_buffer, "*IDN?") == 0) strcpy(wire_buffer,device_name);
  else if (strcmp(wire_buffer, "*STAT?") == 0) load_status();
  else if (strcmp(wire_buffer, "*RST") == 0) {
    seconds_since_reset = millis() / 1000;
    // Reset other variables here
    light_max[0] = 0;
    light_max[1] = 0;
    light_max[2] = 0;
    light_max[3] = 0;
    load_status();
  }
  else strcpy(wire_buffer,"Unknown command");
}

void setup() {
  Serial.begin(9600);
  Wire.begin(WIRE_ADDR);
  Wire.onReceive(receiveComms);
  Wire.onRequest(transmitComms);
  pinMode(PIN_L0, INPUT);
  pinMode(PIN_L1, INPUT);
  pinMode(PIN_L2, INPUT);
  pinMode(PIN_L3, INPUT);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(5, HIGH);
}

void loop() {
  delay(500);
  light[0] = analogRead(PIN_L0);
  light[1] = analogRead(PIN_L1);
  light[2] = analogRead(PIN_L2);
  light[3] = analogRead(PIN_L3);
//  Serial.println(light[0]);
//  Serial.println(light[1]);
//  Serial.println(light[2]);
//  Serial.println(light[3]);
//  Serial.println("");
  if (light[0] > light_max[0]) light_max[0] = light[0];
  if (light[1] > light_max[1]) light_max[1] = light[1];
  if (light[2] > light_max[2]) light_max[2] = light[2];
  if (light[3] > light_max[3]) light_max[3] = light[3];
//  if (light[0] > THRESHOLD) Serial.println("Locker 1 Open");
//  else Serial.println("Locker 1 Closed");
//  if (light[1] > THRESHOLD) Serial.println("Locker 2 Open");
//  else Serial.println("Locker 2 Closed");
//  if (light[2] > THRESHOLD) Serial.println("Locker 3 Open");
//  else Serial.println("Locker 3 Closed");
//  if (light[3] > THRESHOLD) Serial.println("Locker 4 Open");
//  else Serial.println("Locker 4 Closed");
//  Serial.println("");
}

