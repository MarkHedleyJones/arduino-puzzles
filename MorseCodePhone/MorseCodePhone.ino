
#include <A4990MotorShield.h>
#include <Wire.h>

#define PIN_RECEIVER 11
#define PIN_HOOK 2
#define PIN_TRIGGER 12
#define PIN_LED 13
#define RING_OSCILLATIE_PERIOD 40
//#define RING_OSCILLATIE_PERIOD 0
#define MORSE_FREQUENCY 800
#define MORSE_UNIT_PERIOD_MS 200
#define BUF_LEN 96
#define I2C_LEN 32
#define MSG_LEN 20
/*
LOOP WITH DIALTONE IN BETWEEN
RESET ON HANGUP
TIMINGS AS SET
*/


A4990MotorShield motors;


char morse_message[MSG_LEN + 1] = "SOS";
bool ringer_fault = false;
bool execute_signal_received = false;
bool phone_ringing = false;
int play_count = 0;
unsigned long seconds_since_reset = 0;
char wire_buffer[BUF_LEN + 1] = "";
int wire_count = 0;
char tx_buffer[I2C_LEN + 1] = "";
bool pin_trigger = 0;

void stopIfFault() {
  if (motors.getFault()) {
    motors.setSpeeds(0, 0);
    ringer_fault = true;
    while (1);
  }
}

bool phone_on_hook() {
  return digitalRead(PIN_HOOK);
}

bool check_trigger() {
  pin_trigger = digitalRead(PIN_TRIGGER);
  if (execute_signal_received == 0 && digitalRead(PIN_TRIGGER) == 0) {
    delay(100);
    if (pin_trigger == 0) {
      delay(100);
      if (pin_trigger == 0) execute_signal_received = 1;
    }
  }
  return execute_signal_received;
}

void setup() {
//  Serial.begin(9600);
  pinMode(PIN_HOOK, INPUT_PULLUP);
  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_RECEIVER, OUTPUT);
//  Serial.println("I'm awake...");
  Wire.begin(8);
  Wire.onReceive(receiveComms);
  Wire.onRequest(transmitComms);
}

void transmitComms() {
  int offset = wire_count * I2C_LEN;
  for (int i = 0; i < I2C_LEN; i++) tx_buffer[i] = '\0';
  for (int i = 0; i < I2C_LEN; i++) {
    tx_buffer[i] = wire_buffer[i + offset];
  }
  Wire.write(tx_buffer);
  if (wire_count == 2) wire_count = 0;
  else wire_count++;
}

void load_status() {
  char tmp[4] = "";
  strcpy(wire_buffer, "TIME=");
  unsigned long seconds = millis();
  seconds = seconds / 1000;
  seconds = seconds - seconds_since_reset;
  int hours = seconds / 3600;
  int mins = ((seconds / 60) - (hours * 60)) % 3600;
  seconds = seconds - (hours * 3600) - (mins * 60);
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
  strcat(wire_buffer, ",RUN=");
  if (execute_signal_received) strcat(wire_buffer, "1");
  else strcat(wire_buffer, "0");
  strcat(wire_buffer, ",TRIG=");
  if (pin_trigger == 0) strcat(wire_buffer, "1");
  else strcat(wire_buffer, "0");
  strcat(wire_buffer, ",HOOK=");
  if (phone_on_hook()) strcat(wire_buffer, "1");
  else strcat(wire_buffer, "0");
  strcat(wire_buffer, ",RING=");
  if (phone_ringing) strcat(wire_buffer, "1");
  else strcat(wire_buffer, "0");
  strcat(wire_buffer, ",PLAYED=");
  sprintf(tmp, "%d", play_count);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",MSG=");
  strcat(wire_buffer, morse_message);

  // TERMINATE THE WIREBUFFER
  strcat(wire_buffer, 0);
}

void receiveComms(int howMany) {
  int i;
//  digitalWrite(PIN_LED, 1);
  for (i = 0; i < BUF_LEN; i++) wire_buffer[i] = '\0';
  i = 0;
  while (Wire.available() > 0) {
    wire_buffer[i] = Wire.read();
    i++;
  }
  if (strcmp(wire_buffer, "*IDN?") == 0) strcpy(wire_buffer, "Morse code telephone");
  else if (strcmp(wire_buffer, "*STAT?") == 0) load_status();
  else if (strcmp(wire_buffer, "*TRIG") == 0) {
    execute_signal_received = true;
    load_status();
  }
  else if (strcmp(wire_buffer, "*RST") == 0) {
    seconds_since_reset = millis() / 1000;
    execute_signal_received = false;
    play_count = 0;
    ringer_fault = 0;
    strcpy(morse_message, "SOS");
    load_status();
  }
  else if (wire_buffer[0] == '*' &&
           wire_buffer[1] == 'M' &&
           wire_buffer[2] == 'S' &&
           wire_buffer[3] == 'G' &&
           wire_buffer[4] == '=') {
    for (i = 0; i < MSG_LEN; i++) morse_message[i] = '\0';
    for (i = 0; i < MSG_LEN; i++) {
      if (wire_buffer[i + 5] == 32 ||
          (wire_buffer[i + 5] >= 48 && wire_buffer[i + 5] <= 57) ||
          (wire_buffer[i + 5] >= 65 && wire_buffer[i + 5] <= 90) ||
          (wire_buffer[i + 5] >= 97 && wire_buffer[i + 5] <= 122)) {
        morse_message[i] = wire_buffer[i + 5];
      }
      else {
        morse_message[i] = '\0';
        break;
      }

    }
    load_status();
  }
}

void oscillate() {
  delay(RING_OSCILLATIE_PERIOD);
  motors.setM1Speed(400);
  //   stopIfFault();
  delay(RING_OSCILLATIE_PERIOD);
  motors.setM1Speed(-400);
  //   stopIfFault();
}

void ring() {
  unsigned long i;
  //  if (pin_trigger == 0) execute_signal_received = true;
  check_trigger();
  while (phone_on_hook() && execute_signal_received) {
    phone_ringing = true;
    while (phone_on_hook() && i < 10) {
      oscillate();
      i++;
    }
    motors.setM1Speed(0);
    i = 0;
    while (phone_on_hook() && i < 40000) {
      motors.setM1Speed(0);
      i++;
    }
    i = 0;
    while (phone_on_hook() && i < 10) {
      oscillate();
      i++;
    }
    i = 0;
    while (phone_on_hook() && i < 150000) {
      motors.setM1Speed(0);
      i++;
    }
    i = 0;
  }
  phone_ringing = false;
  motors.setM1Speed(0);
  stopIfFault();
}

void play_morse(String input_message) {
  int i;
  int j;
  int input_message_length = input_message.length();
  int delay_millis = 0;
  String code;
  input_message.toUpperCase();
  for (i = 0; i < input_message_length; i++) {
    if (phone_on_hook() || !execute_signal_received) return;
    else {
      if (input_message[i] == 'A') code = ".-";
      else if (input_message[i] == 'B') code = "-...";
      else if (input_message[i] == 'C') code = "-.-.";
      else if (input_message[i] == 'D') code = "-..";
      else if (input_message[i] == 'E') code = ".";
      else if (input_message[i] == 'F') code = "..-.";
      else if (input_message[i] == 'G') code = "--.";
      else if (input_message[i] == 'H') code = "....";
      else if (input_message[i] == 'I') code = "..";
      else if (input_message[i] == 'J') code = ".---";
      else if (input_message[i] == 'K') code = "-.-";
      else if (input_message[i] == 'L') code = ".-..";
      else if (input_message[i] == 'M') code = "--";
      else if (input_message[i] == 'N') code = "-.";
      else if (input_message[i] == 'O') code = "---";
      else if (input_message[i] == 'P') code = ".--.";
      else if (input_message[i] == 'Q') code = "--.-";
      else if (input_message[i] == 'R') code = ".-.";
      else if (input_message[i] == 'S') code = "...";
      else if (input_message[i] == 'T') code = "-";
      else if (input_message[i] == 'U') code = "..-";
      else if (input_message[i] == 'V') code = "...-";
      else if (input_message[i] == 'W') code = ".--";
      else if (input_message[i] == 'X') code = "-..-";
      else if (input_message[i] == 'Y') code = "-.--";
      else if (input_message[i] == 'Z') code = "--..";
      else if (input_message[i] == '0') code = "-----";
      else if (input_message[i] == '1') code = ".----";
      else if (input_message[i] == '2') code = "..---";
      else if (input_message[i] == '3') code = "...--";
      else if (input_message[i] == '4') code = "....-";
      else if (input_message[i] == '5') code = ".....";
      else if (input_message[i] == '6') code = "-....";
      else if (input_message[i] == '7') code = "--...";
      else if (input_message[i] == '8') code = "---..";
      else if (input_message[i] == '9') code = "----.";
      else code = " ";

      if (code == " ") {
        // Play an inter-word space
        noTone(PIN_RECEIVER);
        hook_delay(MORSE_UNIT_PERIOD_MS * 10);
      }
      else {
        // Play a letter
        for (j = 0; j < 5 && code[j] != '\0' && execute_signal_received; j++) {
          if (code[j] == '.') {
            tone(PIN_RECEIVER, MORSE_FREQUENCY);
//            Serial.print("*");
            hook_delay(MORSE_UNIT_PERIOD_MS);
          }
          else {
            tone(PIN_RECEIVER, MORSE_FREQUENCY);
//            Serial.print("***");
            hook_delay(MORSE_UNIT_PERIOD_MS * 3);
          }
          noTone(PIN_RECEIVER);
//          Serial.print("_");
          hook_delay(MORSE_UNIT_PERIOD_MS);
        }
        noTone(PIN_RECEIVER);
//        Serial.print("__");
        hook_delay(MORSE_UNIT_PERIOD_MS * 5);
      }
    }
  }
  play_count++;
}

void hook_delay(int milliseconds) {
  for (int i = 0; i < milliseconds && !phone_on_hook(); i++) {
    delay(1);
  }
}

void dialTone() {
  tone(PIN_RECEIVER, 400);
}

void loop() {
  while (!execute_signal_received) {
    if (!phone_on_hook()) dialTone();
    else noTone(PIN_RECEIVER);
    check_trigger();
//    if (pin_trigger == 0) execute_signal_received = true;
  }
  ring();
  noTone(PIN_RECEIVER);
  while (check_trigger()) {
    noTone(PIN_RECEIVER);
    while (phone_on_hook() && check_trigger()) delay(1); // Loop here if phone hung up
    for (int i = 0; i < 2000 && !phone_on_hook() && check_trigger(); i++) delay(1);
    play_morse(morse_message);
    for (int i = 0; i < 500 && !phone_on_hook() && check_trigger(); i++) delay(1);
    dialTone();
    for (int i = 0; i < 2000 && !phone_on_hook() && check_trigger(); i++) delay(1);
  }
}
