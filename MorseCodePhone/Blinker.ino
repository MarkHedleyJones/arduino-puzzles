#include <A4990MotorShield.h>
#include <Wire.h>

#define PIN_RECEIVER 11
#define PIN_HOOK 2
#define PIN_TRIGGER 12
#define PIN_LED 13
#define RING_OSCILLATIE_PERIOD 30
#define MORSE_FREQUENCY 800
#define MORSE_UNIT_PERIOD_MS 200
#define MSG_LEN 96
/*
LOOP WITH DIALTONE IN BETWEEN
RESET ON HANGUP
TIMINGS AS SET
*/


A4990MotorShield motors;


char morse_message[21] = "sos";
bool ringer_fault = false;
bool execute_signal_recieved = false;
bool phone_ringing = false;
int play_count = 0;
unsigned long seconds_since_reset = 0;
char wire_buffer[MSG_LEN] = "";
int wire_count = 0;
char tx_buffer[32] = "";

void stopIfFault() {
  if (motors.getFault()) {
    motors.setSpeeds(0,0);
    ringer_fault = true;
    while(1);
  }
}

bool phone_on_hook() {
  return digitalRead(PIN_HOOK);
}

bool triggered() {
  return !digitalRead(PIN_TRIGGER);
}

void setup() {
  Serial.begin(9600);
  pinMode(PIN_HOOK, INPUT_PULLUP);
  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_RECEIVER, OUTPUT);
  Serial.println("I'm awake...");
  Wire.begin(8);
  Wire.onReceive(receiveComms);
  Wire.onRequest(transmitComms);
}

void transmitComms() {
  digitalWrite(PIN_LED, HIGH);
  int offset = wire_count * 32;
  for (int i=0; i < 32; i++) {
    tx_buffer[i] = wire_buffer[i+offset];
  }
  Wire.write(tx_buffer);
  Serial.println(offset);
  Serial.println(wire_count);
  Serial.println(tx_buffer);

  if (wire_count == 2) wire_count = 0;
  else wire_count++;

  Serial.println("Done");
  digitalWrite(PIN_LED,0);
}

void load_status() {
    strcpy(wire_buffer, "EX=");
    if (execute_signal_recieved) strcat(wire_buffer, "1");
    else strcat(wire_buffer, "0");
    strcat(wire_buffer, ",HOOK=");
    if (phone_on_hook()) strcat(wire_buffer, "1");
    else strcat(wire_buffer, "0");
    strcat(wire_buffer,",RING=");
    if (phone_ringing) strcat(wire_buffer, "1");
    else strcat(wire_buffer, "0");
    strcat(wire_buffer,",FAULT=");
    if (ringer_fault) strcat(wire_buffer, "1");
    else strcat(wire_buffer, "0");
    strcat(wire_buffer, ",TIME=");
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
    strcat(wire_buffer, ",PLAY_COUNT=");
    sprintf(tmp, "%d", play_count);
    strcat(wire_buffer, tmp);
    strcat(wire_buffer, ",MSG=");
    strcat(wire_buffer, morse_message);
}

void receiveComms(int howMany) {
  int i;
  digitalWrite(PIN_LED,1);
  Serial.println("Receiving message:");
  for (i=0; i<MSG_LEN; i++) wire_buffer[i] = '\0';
  i = 0;
  while (Wire.available() > 0) {
    wire_buffer[i] = Wire.read();
    i++;
  }
  if (strcmp(wire_buffer, "*IDN?") == 0) strcpy(wire_buffer,"Morse code telephone");
  else if (strcmp(wire_buffer, "*STAT?") == 0) load_status();
  else if (strcmp(wire_buffer, "*TRIG") == 0) {
    Serial.println("Triggering");
    execute_signal_recieved = true;
    load_status();
  }
  else if (strcmp(wire_buffer, "*RST") == 0) {
    seconds_since_reset = millis() / 1000;
    execute_signal_recieved = false;
    play_count = 0;
    ringer_fault = 0;
    strcpy(morse_message, "sos");
    load_status();
  }
}

void oscillate() {
    delay(RING_OSCILLATIE_PERIOD);
    motors.setM1Speed(400);
    stopIfFault();
    delay(RING_OSCILLATIE_PERIOD);
    motors.setM1Speed(-400);
    stopIfFault();
}

void ring() {
  unsigned long i;
  while (phone_on_hook()) {
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
  String code;
  Serial.print("Send message: ");
  Serial.println(input_message);
  input_message.toLowerCase();
  for(i=0; i < input_message_length; i++) {
    if (phone_on_hook() || !execute_signal_recieved) return;
    else {
      if (input_message[i] == 'a') code = ".-";
      else if (input_message[i] == 'b') code = "-...";
      else if (input_message[i] == 'c') code = "-.-.";
      else if (input_message[i] == 'd') code = "-..";
      else if (input_message[i] == 'e') code = ".";
      else if (input_message[i] == 'f') code = "..-.";
      else if (input_message[i] == 'g') code = "--.";
      else if (input_message[i] == 'h') code = "....";
      else if (input_message[i] == 'i') code = "..";
      else if (input_message[i] == 'j') code = ".---";
      else if (input_message[i] == 'k') code = "-.-";
      else if (input_message[i] == 'l') code = ".-..";
      else if (input_message[i] == 'm') code = "--";
      else if (input_message[i] == 'n') code = "-.";
      else if (input_message[i] == 'o') code = "---";
      else if (input_message[i] == 'p') code = ".--.";
      else if (input_message[i] == 'q') code = "--.-";
      else if (input_message[i] == 'r') code = ".-.";
      else if (input_message[i] == 's') code = "...";
      else if (input_message[i] == 't') code = "-";
      else if (input_message[i] == 'u') code = "..-";
      else if (input_message[i] == 'v') code = "...-";
      else if (input_message[i] == 'w') code = ".--";
      else if (input_message[i] == 'x') code = "-..-";
      else if (input_message[i] == 'y') code = "-.--";
      else if (input_message[i] == 'z') code = "--..";
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
        delay(MORSE_UNIT_PERIOD_MS * 10);
      }
      else {
        // Play a letter
        for(j=0; j<5 && code[j] != '\0'; j++) {
          if(code[j] == '.') {
            tone(PIN_RECEIVER, MORSE_FREQUENCY);
            Serial.print("*");
            delay(MORSE_UNIT_PERIOD_MS);
          }
          else {
            tone(PIN_RECEIVER, MORSE_FREQUENCY);
            Serial.print("***");
            delay(MORSE_UNIT_PERIOD_MS * 3);
          }
          noTone(PIN_RECEIVER);
          Serial.print("_");
          delay(MORSE_UNIT_PERIOD_MS);
        }
        noTone(PIN_RECEIVER);
        Serial.print("__");
        delay(MORSE_UNIT_PERIOD_MS * 5);
      }
    }
  }
  play_count++;
}

void dialTone() {
  tone(PIN_RECEIVER, 400);
}

void loop() {
  while (!execute_signal_recieved) {
    dialTone();
  }
  ring();
  noTone(PIN_RECEIVER);
  while (execute_signal_recieved) {
    noTone(PIN_RECEIVER);
    while(phone_on_hook()) delay(1); // Loop here if phone hung up

    for (int i=0; i < 2000 && !phone_on_hook() && execute_signal_recieved; i++) delay(1);
    play_morse(morse_message);
    for (int i=0; i < 500 && !phone_on_hook() && execute_signal_recieved; i++) delay(1);
    dialTone();
    for (int i=0; i < 2000 && !phone_on_hook() && execute_signal_recieved; i++) delay(1);
  }
}
