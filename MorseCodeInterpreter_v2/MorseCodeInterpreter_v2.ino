#include <Wire.h>
#include <toneAC.h>

#define BUF_LEN 96
#define I2C_LEN 32
#define MSG_LEN 10
#define PIN_LED 13
#define PIN_MORSE 12
#define WIRE_ADDR 5
#define PIN_OUT 2
#define PRESSED !digitalRead(PIN_MORSE)
#define BOUNDARY_PADDING 40            // Percent 
#define true 1
#define false 0
#define DURATION_MIN 10
#define FLASH_PERIOD 333
#define BULB_1 A0
#define BULB_2 A1
#define BULB_3 A2
#define BULB_4 A3
#define BULB_5 3
#define BULB_6 4
#define BULB_7 5
#define BULB_8 6
#define FINISH_FLASHES 3
#define FINAL_ILLUMINATION_TIME_SECS 10
#define DEBUG 0

char defaultMessage[9] = {'C', 'O', 'D', 'E', 'B', 'L', 'U', 'E', 0};
const char MAX_TAPS = MSG_LEN * 5;
char expected_taps[MAX_TAPS] = {0};
int expected_msg_length = 5;
char expected_msg_decoded[MSG_LEN] = {0};
int tap_durations[MAX_TAPS] = {0};

bool key_pressed = false;
int completed = false;                      // 1 for puzzle done
int totalTaps = 0;                      // Tap counter
int attempts = 0;
char last_attempt[MAX_TAPS] = {0};      // Prevous attempt
bool checked = false;

unsigned long seconds_since_reset = 0;
int wire_count = 0;
char wire_buffer[BUF_LEN + 1] = {0};
char tx_buffer[I2C_LEN + 1] = {0};
char message_buffer[MSG_LEN] = {0};

void setup() {
  pinMode(PIN_MORSE, INPUT);
  pinMode(13, OUTPUT);
  pinMode(PIN_OUT, OUTPUT);
  pinMode(BULB_1, OUTPUT);
  pinMode(BULB_2, OUTPUT);
  pinMode(BULB_3, OUTPUT);
  pinMode(BULB_4, OUTPUT);
  pinMode(BULB_5, OUTPUT);
  pinMode(BULB_6, OUTPUT);
  pinMode(BULB_7, OUTPUT);
  pinMode(BULB_8, OUTPUT);
//  pinMode(BULB_9, OUTPUT);
//  pinMode(BULB_10, OUTPUT);
  digitalWrite(PIN_OUT, HIGH);
  digitalWrite(13, LOW);
  Serial.begin(9600);
  Wire.begin(WIRE_ADDR);
  Wire.onReceive(receiveComms);
  Wire.onRequest(transmitComms);
  set_message(defaultMessage);
  illuminate_bulbs(0);
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
  strcat(wire_buffer, ",DONE=");
  if (completed) strcat(wire_buffer, "1");
  else strcat(wire_buffer, "0");
  strcat(wire_buffer, ",ATTEMPTS=");
  sprintf(tmp, "%d", attempts);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",PRESSED=");
  if (key_pressed) strcat(wire_buffer, "1");
  else strcat(wire_buffer, "0");
  strcat(wire_buffer, ",MSG=");
  strcat(wire_buffer, expected_msg_decoded);
  // strcat(wire_buffer, ",TARG=");
  // strcat(wire_buffer, expected_taps);
  // strcat(wire_buffer, ",LAST=");
  // strcat(wire_buffer, last_attempt);
  strcat(wire_buffer, '\0');
}

void receiveComms(int howMany) {
  int i;
  digitalWrite(PIN_LED, 1);
//  if (DEBUG) Serial.println("Receiving message:");
  for (i = 0; i < BUF_LEN; i++) wire_buffer[i] = '\0';
  i = 0;
  while (Wire.available() > 0) {
    wire_buffer[i] = Wire.read();
    i++;
  }
//  if (DEBUG) Serial.println(wire_buffer);
  if (strcmp(wire_buffer, "*IDN?") == 0) strcpy(wire_buffer, "Morse code interpreter v1.0");
  else if (strcmp(wire_buffer, "*STAT?") == 0) load_status();
  else if (strcmp(wire_buffer, "*TRIG") == 0) {
//    if (DEBUG) Serial.println("Triggering");
    completed = 1;
    load_status();
  }
  else if (strcmp(wire_buffer, "*RST") == 0) {
    totalTaps = 0;
    completed = 0;
    attempts = 0;
    set_message(defaultMessage);
    key_pressed = false;
    seconds_since_reset = millis() / 1000;
    load_status();
    digitalWrite(PIN_OUT, HIGH);
    for (int i = 0; i < MAX_TAPS; i++) last_attempt[i] = 0;
    clear_durations();
    
  }
  else if (wire_buffer[0] == '*' &&
           wire_buffer[1] == 'M' &&
           wire_buffer[2] == 'S' &&
           wire_buffer[3] == 'G' &&
           wire_buffer[4] == '=') {
    for (i = 0; i < MSG_LEN; i++) message_buffer[i] = '\0';
    for (i = 0; i < MSG_LEN; i++) {
      if (wire_buffer[i + 5] == 32 ||
          (wire_buffer[i + 5] >= 48 && wire_buffer[i + 5] <= 57) ||
          (wire_buffer[i + 5] >= 65 && wire_buffer[i + 5] <= 90) ||
          (wire_buffer[i + 5] >= 97 && wire_buffer[i + 5] <= 122)) {
        //Message character is a letter or number
        message_buffer[i] = wire_buffer[i + 5];
      }
      else {
        message_buffer[i] = '\0';
      }
    }
    set_message(message_buffer);
    load_status();
  }
  else strcpy(wire_buffer, "Unknown command");
}

void add_duration(int duration) {
  // Shifts all durations in the tap_durations array left one
  for (int i = 1; i < MAX_TAPS; i++) tap_durations[i - 1] = tap_durations[i];
  tap_durations[MAX_TAPS - 1] = duration;
}

void clear_durations() {
  for (int i = 0; i < MAX_TAPS; i++) tap_durations[i] = 0;
  for (int i = 0; i < MAX_TAPS; i++) last_attempt[i] = 0;
//  print_durations();
  illuminate_bulbs(0);
}

void print_durations() {
  for (int i = 1; i < MAX_TAPS; i++) {
    Serial.print(tap_durations[i]);
    Serial.print(", ");
  }
  Serial.println("");
}

bool durations_empty() {
  return (tap_durations[MAX_TAPS - 1] == 0);
}

void get_boundaries(int* minimum, int* maximum) {
  int longest = 0;
  int shortest = 32000;
  for (int i = 0; i < MAX_TAPS; i++) {
    if (tap_durations[i] > DURATION_MIN) {
      if (tap_durations[i] < shortest) shortest = tap_durations[i];
      if (tap_durations[i] > longest) longest = tap_durations[i];
    }
  }
  int boundary = 0;
  int range = (longest - shortest);
  boundary = (range / 2) + shortest;
  *minimum = boundary - (int)(BOUNDARY_PADDING);
  *maximum = boundary + (int)(BOUNDARY_PADDING);
  if (DEBUG) {
    Serial.print("Longest press = ");
    Serial.print(longest);
    Serial.println(" ms");
    Serial.print("Shortest press = ");
    Serial.print(shortest);
    Serial.println(" ms");
    Serial.print("Range = ");
    Serial.print(range);
    Serial.println(" ms");
    Serial.print("Boundary = ");
    Serial.print(boundary);
    Serial.println(" ms");
  }
}

int convert_taps_to_bulbs(int num_taps) {
  int count = 0;
  for (int i = 0; i < 10; i++) {
    count += convert_char_taps(expected_msg_decoded[i]).length();
    if (num_taps < count) return i;
  }
  return 10;
}

void illuminate_bulbs(int bulbs) {
  digitalWrite(BULB_1, bulbs >= 1);
  digitalWrite(BULB_2, bulbs >= 2);
  digitalWrite(BULB_3, bulbs >= 3);
  digitalWrite(BULB_4, bulbs >= 4);
  digitalWrite(BULB_5, bulbs >= 5);
  digitalWrite(BULB_6, bulbs >= 6);
  digitalWrite(BULB_7, bulbs >= 7);
  digitalWrite(BULB_8, bulbs >= 8);
}



void check_tap_durations() {
  int boundary_min = 0;
  int boundary_max = 0;
  int count_correct = 0;
  char tap = ' ';
  char unknown_char = 'x';
  int start_count = 0;
  get_boundaries(&boundary_min, &boundary_max);
  if (DEBUG) Serial.print("Boundary min = ");
  if (DEBUG) Serial.println(boundary_min);
  if (DEBUG) Serial.print("Boundary max = ");
  if (DEBUG) Serial.println(boundary_max);
  if (DEBUG) Serial.print("Looking for ");
  if (DEBUG) Serial.println(expected_taps);
  if (DEBUG) Serial.print("Message length = ");
  if (DEBUG) Serial.println(expected_msg_length);
  if (DEBUG) Serial.print("Got:\n");
  count_correct = 0;
  for (int i = 0; i < MAX_TAPS; i++) {
    if (tap_durations[i] < DURATION_MIN) {
      start_count = i;
      continue;
    }
    else if (tap_durations[i] < boundary_min) {
      // short
      tap = '.';
      
      if (DEBUG) Serial.print("Tap ");
      if (DEBUG) Serial.print(tap);
      if (DEBUG) Serial.print(" == ");
      if (DEBUG) Serial.print(expected_taps[count_correct]);
      if (DEBUG) Serial.print(" ? ");
      
      if (expected_taps[count_correct] == tap) {
        if (DEBUG) Serial.print("Yes");
        count_correct++;
      }
      else {
        if (DEBUG) Serial.print("No");
        count_correct = 0;
      }
      if (DEBUG) Serial.print("; count = ");
      if (DEBUG) Serial.println(count_correct);

    }
    else if (tap_durations[i] > boundary_max) {
      tap = '-';
      // long
      
      if (DEBUG) Serial.print("Tap ");
      if (DEBUG) Serial.print(tap);
      if (DEBUG) Serial.print(" == ");
      if (DEBUG) Serial.print(expected_taps[count_correct]);
      if (DEBUG) Serial.print(" ? ");
      
      if (expected_taps[count_correct] == tap) {
        if (DEBUG) Serial.print("Yes");
        count_correct++;
      }
      else {
        if (DEBUG) Serial.print("No");
        count_correct = 0;
      }
      if (DEBUG) Serial.print("; count = ");
      if (DEBUG) Serial.println(count_correct);
    }
    else {
      tap = 'x';
      if (DEBUG) Serial.print("Tap ");
      if (DEBUG) Serial.print(tap);
      if (DEBUG) Serial.print(" == ");
      if (DEBUG) Serial.print(expected_taps[count_correct]);
      if (DEBUG) Serial.print(" ? ");
      if (DEBUG) Serial.println("Yes");
      
      if (unknown_char == 'x') {
        // Give the user the benefit of the doubt on that tap
        // but make a note of which length it would have been
        // intended to represent.
        unknown_char = expected_taps[count_correct];
        count_correct++;
        if (DEBUG) Serial.print("Unknown now set as ");
        if (DEBUG) Serial.println(unknown_char);
      }
      else {
        if (expected_taps[count_correct] == unknown_char) {
          count_correct++;
          if (DEBUG) Serial.println("Consistant use of unknown char");
        }
        else {
          unknown_char = 'x';
          count_correct = 0;
          if (DEBUG) Serial.println("Inconsistant use of unknown char");
        }
      }
    }
    last_attempt[i-start_count-1] = tap;
  }
  if (DEBUG) Serial.print("Correct = ");
  if (DEBUG) Serial.println(count_correct);
  if (count_correct == expected_msg_length) {
    completed = true;
    if (DEBUG) Serial.println("All Correct!!!!!!!");
  }
  illuminate_bulbs(convert_taps_to_bulbs(count_correct));
  checked = true;
}

void flash_bulbs() {
  illuminate_bulbs(0);
  delay(FLASH_PERIOD);
  illuminate_bulbs(8);
  delay(FLASH_PERIOD);
}

void loop() {
  unsigned long millis_end = millis();
  unsigned long millis_start = 0;
  // Wait for the first press
  while (!PRESSED && !completed) {
    if ((millis() - millis_end > 200) && !checked) check_tap_durations();
    if ((millis() - millis_end > 3000) && !durations_empty()) {
      attempts++;
      clear_durations();
    }
  }
  key_pressed = true;
  millis_start = millis();
  while (PRESSED && !completed) toneAC(1000);
  toneAC();
  while ((millis() - millis_start) < DURATION_MIN);
  add_duration(millis() - millis_start);
  checked = false;

  if (completed) {
    while (completed) {
    	flash_bulbs();
    }
  }
}

String convert_char_taps(char character) {
  String out;
  if (character == 'A' || character == 'a') out = ".-";
  else if (character == 'B' || character == 'b') out = "-...";
  else if (character == 'C' || character == 'c') out = "-.-.";
  else if (character == 'D' || character == 'd') out = "-..";
  else if (character == 'E' || character == 'e') out = ".";
  else if (character == 'F' || character == 'f') out = "..-.";
  else if (character == 'G' || character == 'g') out = "--.";
  else if (character == 'H' || character == 'h') out = "....";
  else if (character == 'I' || character == 'i') out = "..";
  else if (character == 'J' || character == 'j') out = ".---";
  else if (character == 'K' || character == 'k') out = "-.-";
  else if (character == 'L' || character == 'l') out = ".-..";
  else if (character == 'M' || character == 'm') out = "--";
  else if (character == 'N' || character == 'n') out = "-.";
  else if (character == 'O' || character == 'o') out = "---";
  else if (character == 'P' || character == 'p') out = ".--.";
  else if (character == 'Q' || character == 'q') out = "--.-";
  else if (character == 'R' || character == 'r') out = ".-.";
  else if (character == 'S' || character == 's') out = "...";
  else if (character == 'T' || character == 't') out = "-";
  else if (character == 'U' || character == 'u') out = "..-";
  else if (character == 'V' || character == 'v') out = "...-";
  else if (character == 'W' || character == 'w') out = ".--";
  else if (character == 'X' || character == 'x') out = "-..-";
  else if (character == 'Y' || character == 'y') out = "-.--";
  else if (character == 'Z' || character == 'z') out = "--..";
  else if (character == '0') out = "-----";
  else if (character == '1') out = ".----";
  else if (character == '2') out = "..---";
  else if (character == '3') out = "...--";
  else if (character == '4') out = "....-";
  else if (character == '5') out = ".....";
  else if (character == '6') out = "-....";
  else if (character == '7') out = "--...";
  else if (character == '8') out = "---..";
  else if (character == '9') out = "----.";
  else out = " ";
  return out;
}

void set_message(char *message) {
  int i;
  expected_msg_length = 0;
//  if (DEBUG) Serial.print("Setting message to: ");
//  if (DEBUG) Serial.println(message);
  // Clear the expected message string
  for (i = 0; i < MSG_LEN; i++) expected_taps[i] = 0;
  for (i = 0; i < MSG_LEN; i++) expected_msg_decoded[i] = 0;
  for (i = 0; i < MSG_LEN && message[i] != '\0'; i++) expected_msg_decoded[i] = message[i];
  String code;
  int index = 0;
  i = 0;
  while (message[i] != '\0') {
    code = convert_char_taps(message[i]);
    for (int j = 0; j < 5 && code[j] != '\0'; j++) {
      expected_taps[index] = code[j];
      index++;
    }
    i++;
    if (code != " ") expected_msg_length += code.length();
  }
  expected_taps[index] = '\0';
//  if (DEBUG) Serial.println("Expected message pattern: ");
//  if (DEBUG) Serial.println(expected_taps);
}
