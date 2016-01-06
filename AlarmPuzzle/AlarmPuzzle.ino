#include <Wire.h>

#define BUF_LEN 96
#define I2C_LEN 32
#define WIRE_ADDR 11

#define PIN_DOOR   7
#define PIN_BUZZER 8
#define PIN_LED    9
#define PIN_1 2
#define PIN_2 3
#define PIN_3 4
#define PIN_4 5

#define READY 1
#define ARMED 2
#define DISABLED 0
#define TRIGGERED 3

// Defuse combination lookup-table
//
// 0  1234 - 
// 1  1243 - 
// 2  1324 - 
// 3  1342 - 
// 4  1423 - 
// 5  1432 - 
// 6  2134 - 
// 7  2143 - 
// 8  2314 - 
// 9  2341 - 
// 10  2413 - 
// 11  2431 - 
// 12  3124 - 
// 13  3142 - 
// 14  3214 - 
// 15  3241 - 
// 16  3412 - 
// 17  3421 - 
// 18  4123 - 
// 19  4132 - 
// 20  4213 - 
// 21  4231 - 
// 22  4312 - 
// 23  4321 - 

const uint8_t defuse_lookup[24][4] = {
  {0b1110, 0b1100, 0b1000, 0b0000},
  {0b1110, 0b1100, 0b0100, 0b0000},
  {0b1110, 0b1010, 0b1000, 0b0000},
  {0b1110, 0b1010, 0b0010, 0b0000},
  {0b1110, 0b0110, 0b0100, 0b0000},
  {0b1110, 0b0110, 0b0010, 0b0000},
  {0b1101, 0b1100, 0b1000, 0b0000},
  {0b1101, 0b1100, 0b0100, 0b0000},
  {0b1101, 0b1001, 0b1000, 0b0000},
  {0b1101, 0b1001, 0b0001, 0b0000},
  {0b1101, 0b0101, 0b0100, 0b0000},
  {0b1101, 0b0101, 0b0001, 0b0000},
  {0b1011, 0b1010, 0b1000, 0b0000},
  {0b1011, 0b1010, 0b0010, 0b0000},
  {0b1011, 0b1001, 0b1000, 0b0000},
  {0b1011, 0b1001, 0b0001, 0b0000},
  {0b1011, 0b0011, 0b0010, 0b0000},
  {0b1011, 0b0011, 0b0001, 0b0000},
  {0b0111, 0b0110, 0b0100, 0b0000},
  {0b0111, 0b0110, 0b0010, 0b0000},
  {0b0111, 0b0101, 0b0100, 0b0000},
  {0b0111, 0b0101, 0b0001, 0b0000},
  {0b0111, 0b0011, 0b0010, 0b0000},
  {0b0111, 0b0011, 0b0001, 0b0000},
};

const uint8_t default_defuse_combination = 17;
const uint8_t default_alarm_state = READY;
const uint8_t default_time_factor = 12;
const uint8_t default_siren_duration = 15;

uint8_t defuse_combination;
uint8_t alarm_state;
uint8_t time_factor;
uint8_t current_combination;
uint16_t siren_duration;

const char device_name[] = "Alarm";
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
  
  strcat(wire_buffer, ",COMBO=");
  sprintf(tmp, "%d", defuse_combination);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",STATE=");
  sprintf(tmp, "%d", alarm_state);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",TFACTOR=");
  sprintf(tmp, "%d", time_factor);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",WIRES=");
  sprintf(tmp, "%d", current_combination);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",SIREN_T=");
  sprintf(tmp, "%d", siren_duration / 1000);
  strcat(wire_buffer, tmp);
  
  // TERMINATE THE WIREBUFFER
  strcat(wire_buffer, 0);
}

void receiveComms(int howMany) {
  String message;
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
    defuse_combination = default_defuse_combination;
    alarm_state = default_alarm_state;
    time_factor = default_time_factor;
    load_status();
  }
  else if (message.indexOf("*COMBO=") != -1) {
    defuse_combination = message.substring(7).toInt();
    load_status();
  }
  else if (message.indexOf("*SIREN_T=") != -1) {
    siren_duration = message.substring(9).toInt() * 1000;
    load_status();
  }
  else if (message.indexOf("*TFACTOR=") != -1) {
    time_factor = message.substring(9).toInt();
    load_status();
  }
  else if (message.indexOf("*TRIG") != -1) {
    alarm_state = 3;
    load_status();
  }
  else if (message.indexOf("*ARM") != -1) {
    alarm_state = 2;
    load_status();
  }
  else strcpy(wire_buffer,"Unknown command");
}

void setup() {
  Serial.begin(9600);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_1, INPUT_PULLUP);
  pinMode(PIN_2, INPUT_PULLUP);
  pinMode(PIN_3, INPUT_PULLUP);
  pinMode(PIN_4, INPUT_PULLUP);
  pinMode(PIN_DOOR, INPUT_PULLUP);

  Wire.begin(WIRE_ADDR);
  Wire.onReceive(receiveComms);
  Wire.onRequest(transmitComms);

  // Reset state variables
  defuse_combination = default_defuse_combination;
  alarm_state = default_alarm_state;
  time_factor = default_time_factor;
  siren_duration = default_siren_duration * 1000;
}

void check_door(void) {
  if (digitalRead(PIN_DOOR) == 1) {
    delay(100);
    if (digitalRead(PIN_DOOR) == 1) {
      delay(50);
      if (digitalRead(PIN_DOOR) == 1) {
        delay(25);
      }
      if (digitalRead(PIN_DOOR) == 1) {
        if (alarm_state == 1) alarm_state = ARMED;
      }
    }
  }
}

void check_wire_state() {
  // Debounce dodgy connections
  uint8_t out = 0b0000;
  uint8_t tmp = 0;  
  tmp = (PIND & 0b111100) >> 2;
  if (tmp & 1) out |= 1;
  if (tmp & 2) out |= 2;
  if (tmp & 4) out |= 4;
  if (tmp & 8) out |= 8;
  delay(50);
  tmp = (PIND & 0b111100) >> 2;
  if (tmp & 1) out |= 1;
  if (tmp & 2) out |= 2;
  if (tmp & 4) out |= 4;
  if (tmp & 8) out |= 8;
  delay(25);
  tmp = (PIND & 0b111100) >> 2;
  if (tmp & 1) out |= 1;
  if (tmp & 2) out |= 2;
  if (tmp & 4) out |= 4;
  if (tmp & 8) out |= 8;
  delay(12);
  tmp = (PIND & 0b111100) >> 2;
  
  if (tmp & 1) out |= 1;
  if (tmp & 2) out |= 2;
  if (tmp & 4) out |= 4;
  if (tmp & 8) out |= 8;
  // Convert the output to be 1=wire attached, 0=wire detached
  current_combination = (~out) & 0b1111;
}

void check_wires() {
  check_wire_state();
  int progress = 0;
  if ((current_combination & 0b0001) == 0) progress++;
  if ((current_combination & 0b0010) == 0) progress++;
  if ((current_combination & 0b0100) == 0) progress++;
  if ((current_combination & 0b1000) == 0) progress++;
  if (progress > 0) {
    if (progress == 4) alarm_state = DISABLED;
    else {
      progress--;
      if (current_combination != defuse_lookup[defuse_combination][progress]) {
        alarm_state = TRIGGERED;
      }
    }
  }
}

void loop() {
  int i, time_delay;
  digitalWrite(PIN_LED, HIGH);
  while (alarm_state == READY) check_door();

  time_delay = 1000;
  
  while (alarm_state == ARMED) {
    check_wires();
    digitalWrite(PIN_LED, HIGH);
    digitalWrite(PIN_BUZZER, HIGH);
    delay(15);
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LED, LOW);
    delay(time_delay/2);
    check_wires();
    delay(time_delay/2);
//    for (i = 0; i < time_delay; i++) {
//      delay(1);
//      if (i % 2 == 0) check_wires();
//    }
    time_delay -= time_factor;
    if (time_delay <= 1) alarm_state = TRIGGERED; 
  }

  while (alarm_state == TRIGGERED) {
    Serial.println("Triggered");
    digitalWrite(PIN_LED, HIGH);
    digitalWrite(PIN_BUZZER, HIGH);
    // Only disable the alarm if not 0
    if (siren_duration > 0) {
      delay(siren_duration);
      digitalWrite(PIN_LED, LOW);
      digitalWrite(PIN_BUZZER, LOW);
    }
    // Wait here until reset
    while(alarm_state == TRIGGERED);
  }

  while (alarm_state == DISABLED) {
    Serial.println("Solved");
    digitalWrite(PIN_LED, LOW);
    digitalWrite(PIN_BUZZER, LOW);
    delay(200);
    digitalWrite(PIN_LED, HIGH);
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_LED, LOW);
    digitalWrite(PIN_BUZZER, LOW);
    delay(200);
    digitalWrite(PIN_LED, HIGH);
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_LED, LOW);
    digitalWrite(PIN_BUZZER, LOW);
    // Wait here until reset
    while(alarm_state == DISABLED);
  }
}
