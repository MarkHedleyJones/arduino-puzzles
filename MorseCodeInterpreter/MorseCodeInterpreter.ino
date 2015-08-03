#include <Wire.h>
#include <toneAC.h>

#define BUF_LEN 96
#define I2C_LEN 32
#define MSG_LEN 20
#define PIN_LED 13
#define PIN_MORSE 12
#define WIRE_ADDR 5
#define PIN_OUT 2

const int maxNumberTaps = 100;
const int waitTime = 50;               // Time in us for each tap count
const long timeoutLength = 20000;       // Time in us for timeout
const int maxMessageLength = 20;
const float disambiguationFactor = 0.2; // Zone between - and .

char defaultMessage[6] = {'B', 'R', 'A', 'V', 'O', 0};
char expectedMessage[maxNumberTaps] = {0};
char expectedMessageText[maxMessageLength] = {0};

int completed = 0;                      // 1 for puzzle done
int totalTaps = 0;                      // Tap counter
int attempts = 0;
char last_attempt[maxMessageLength] = {0};      // Prevous attempt

unsigned long seconds_since_reset = 0;
int wire_count = 0;
char wire_buffer[BUF_LEN+1] = {0};
char tx_buffer[I2C_LEN+1] = {0};
char message_buffer[MSG_LEN] = {0};

const char morseCode[130] = {
    '.', '-', 0, 0, 0,      // A
    '-', '.', '.', '.', 0,  // B
    '-', '.', '-', '.', 0,  // C
    '-', '.', '.', 0, 0,    // D
    '.', 0, 0, 0, 0,        // E
    '.', '.', '-', '.', 0,  // F
    '-', '-', '.', 0, 0,    // G
    '.', '.', '.', '.', 0,  // H
    '.', '.', 0, 0, 0,      // I
    '.', '-', '-', '-', 0,  // J
    '-', '.', '-', 0, 0,    // K
    '.', '-', '.', '.', 0,  // L
    '-', '-', 0, 0, 0,      // M
    '-', '.', 0, 0, 0,      // N
    '-', '-', '-', 0, 0,    // O
    '.', '-', '-', '.', 0,  // P
    '-', '-', '.', '-', 0,  // Q
    '.', '-', '.', 0, 0,    // R
    '.', '.', '.', 0, 0,    // S
    '-', 0, 0, 0, 0,        // T
    '.', '.', '-', 0, 0,    // U
    '.', '.', '.', '-', 0,  // V
    '.', '-', '-', 0, 0,    // W
    '-', '.', '.', '-', 0,  // X
    '-', '.', '-', '-', 0,  // Y
    '-', '-', '.', '.', 0   // Z
};

void transmitComms() {
  digitalWrite(PIN_LED, HIGH);
  int offset = wire_count * I2C_LEN;
  for (int i=0; i < I2C_LEN; i++) tx_buffer[i] = '\0';
  for (int i=0; i < I2C_LEN; i++) {
    tx_buffer[i] = wire_buffer[i+offset];
  }
  Wire.write(tx_buffer);
//  Serial.print("Sending part ");
//  Serial.print(wire_count);
//  Serial.print(": ");
//  Serial.println(tx_buffer);
  if (wire_count == 2) wire_count = 0;
  else wire_count++;
//  Serial.println("Done");
  digitalWrite(PIN_LED,0);
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
  strcat(wire_buffer, ",MSG=");
  strcat(wire_buffer, expectedMessageText);
//  strcat(wire_buffer, ",TAPS=");
//  sprintf(tmp, "%d", totalTaps);
//  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",TARG=");
  strcat(wire_buffer, expectedMessage);
  strcat(wire_buffer, ",LAST=");
  strcat(wire_buffer, last_attempt);
  strcat(wire_buffer, '\0');
}

void receiveComms(int howMany) {
  int i;
  digitalWrite(PIN_LED,1);
  Serial.println("Receiving message:");
  for (i=0; i<BUF_LEN; i++) wire_buffer[i] = '\0';
  i = 0;
  while (Wire.available() > 0) {
    wire_buffer[i] = Wire.read();
    i++;
  }
  Serial.println(wire_buffer);
  if (strcmp(wire_buffer, "*IDN?") == 0) strcpy(wire_buffer,"Morse code interpreter v1.0");
  else if (strcmp(wire_buffer, "*STAT?") == 0) load_status();
  else if (strcmp(wire_buffer, "*TRIG") == 0) {
    Serial.println("Triggering");
    completed = 1;
    load_status();
  }
  else if (strcmp(wire_buffer, "*RST") == 0) {
    seconds_since_reset = millis() / 1000;
    totalTaps = 0;
    completed = 0;
    digitalWrite(PIN_OUT, HIGH);
    for (int i=0; i < maxMessageLength; i++) last_attempt[i] = 0;
    setMessage_v2(defaultMessage);
    attempts = 0;
    load_status();
  }
  else if (wire_buffer[0] == '*' &&
           wire_buffer[1] == 'M' &&
           wire_buffer[2] == 'S' &&
           wire_buffer[3] == 'G' &&
           wire_buffer[4] == '=') {
    for (i=0; i<MSG_LEN; i++) message_buffer[i] = '\0';
    for (i=0; i<MSG_LEN; i++) {
      if (wire_buffer[i+5] == 32 ||
         (wire_buffer[i+5] >= 48 && wire_buffer[i+5] <= 57) ||
         (wire_buffer[i+5] >= 65 && wire_buffer[i+5] <= 90) ||
         (wire_buffer[i+5] >= 97 && wire_buffer[i+5] <= 122)) {
         //Message character is a letter or number
        message_buffer[i] = wire_buffer[i+5];
      }
      else {
        message_buffer[i] = '\0';
      }
    }
    setMessage_v2(message_buffer);
    load_status();
  }
  else strcpy(wire_buffer, "Unknown command");
}

void setup() {
  pinMode(PIN_MORSE, INPUT);
  pinMode(13, OUTPUT);
  pinMode(PIN_OUT, OUTPUT);
  digitalWrite(PIN_OUT, HIGH);
  digitalWrite(13, LOW);
  Serial.begin(9600);
  Wire.begin(WIRE_ADDR);
  Wire.onReceive(receiveComms);
  Wire.onRequest(transmitComms);
  setMessage_v2(defaultMessage);
}

void loop() {
  int tapLengths[maxNumberTaps] = {0};
  
  // Wait for the first press
  while(digitalRead(PIN_MORSE)) {}
  
  // Taps have started
  readTaps(tapLengths);

  char message[maxNumberTaps] = {0};
  decodeTaps(message, tapLengths);
  for (int i = 0; i < maxMessageLength; i++) last_attempt[i] = message[i];

  // Decide if taps correct or not
  completed |= compareTaps(message);
  attempts += 1;

  while (completed) {
    digitalWrite(13, HIGH);
    digitalWrite(PIN_OUT, LOW);
    Serial.println("Correct message! - Waiting for reset..."); 
  }
}

void readTaps(int *tapLengths)
{
  int upTime = 0;
  int timedOut = 0;
  int index = 0;

  while(!timedOut) {
    upTime = 0;
    while(!digitalRead(PIN_MORSE)) {
      tapLengths[index]++;
      toneAC(1000);
      delayMicroseconds(waitTime);
    }
    toneAC();

    // Tap finished
    while (digitalRead(PIN_MORSE)) {
      upTime++;
      delayMicroseconds(waitTime);
      if (upTime > timeoutLength) {
        // No taps for a while
        timedOut = 1;
        break;
      }
    }

    index++;
    totalTaps++;
  }
}

void decodeTaps(char *message, int *tapLengths)
{
  double total = 0;
  int average = 0;
  int dotMax = 0;
  int dashMin = 0;
  int numberOfTaps = 0;
  
  for (numberOfTaps; numberOfTaps < maxNumberTaps; numberOfTaps++) {
    if (tapLengths[numberOfTaps] != 0) total += tapLengths[numberOfTaps];
    else break;
  }

  average = total / numberOfTaps;

  dotMax = average - (average * disambiguationFactor);
  dashMin = average + (average * disambiguationFactor);

  for (int i = 0; i < maxNumberTaps; i++) {
    if (tapLengths[i] != 0) {
      if (tapLengths[i] > dashMin) message[i] = '-';
      else if (tapLengths[i] < dotMax) message[i] = '.';
      else message[i] = 'x';
    }
    else break;
  }
}

int compareTaps(char *message) {
  for (int i = 0; i < maxNumberTaps; i++) {
    if (expectedMessage[i] == 0 && message[i] == 0) return 1;
    else if (expectedMessage[i] == 0 && message[i] != 0) return 0;
    else if (expectedMessage[i] != 0 && message[i] == 0) return 0;
    else if (expectedMessage[i] != message[i]) return 0;
  }
  return 0;
}

void setMessage_v2(char *message) {
  int i;
  Serial.print("Setting message to: ");
  Serial.println(message);
  // Clear the expected message string
  for (i=0; i<maxNumberTaps; i++) expectedMessage[i] = 0;
  for (i=0; i<maxMessageLength; i++) expectedMessageText[i] = 0;
  for (i=0; i<maxMessageLength && message[i] != '\0'; i++) expectedMessageText[i] = message[i];
  String code;
  int index = 0;
  i = 0;
  while(message[i] != '\0') {
    if (message[i] == 'A' || message[i] == 'a') code = ".-";
    else if (message[i] == 'B' || message[i] == 'b') code = "-...";
    else if (message[i] == 'C' || message[i] == 'c') code = "-.-.";
    else if (message[i] == 'D' || message[i] == 'd') code = "-..";
    else if (message[i] == 'E' || message[i] == 'e') code = ".";
    else if (message[i] == 'F' || message[i] == 'f') code = "..-.";
    else if (message[i] == 'G' || message[i] == 'g') code = "--.";
    else if (message[i] == 'H' || message[i] == 'h') code = "....";
    else if (message[i] == 'I' || message[i] == 'i') code = "..";
    else if (message[i] == 'J' || message[i] == 'j') code = ".---";
    else if (message[i] == 'K' || message[i] == 'k') code = "-.-";
    else if (message[i] == 'L' || message[i] == 'l') code = ".-..";
    else if (message[i] == 'M' || message[i] == 'm') code = "--";
    else if (message[i] == 'N' || message[i] == 'n') code = "-.";
    else if (message[i] == 'O' || message[i] == 'o') code = "---";
    else if (message[i] == 'P' || message[i] == 'p') code = ".--.";
    else if (message[i] == 'Q' || message[i] == 'q') code = "--.-";
    else if (message[i] == 'R' || message[i] == 'r') code = ".-.";
    else if (message[i] == 'S' || message[i] == 's') code = "...";
    else if (message[i] == 'T' || message[i] == 't') code = "-";
    else if (message[i] == 'U' || message[i] == 'u') code = "..-";
    else if (message[i] == 'V' || message[i] == 'v') code = "...-";
    else if (message[i] == 'W' || message[i] == 'w') code = ".--";
    else if (message[i] == 'X' || message[i] == 'x') code = "-..-";
    else if (message[i] == 'Y' || message[i] == 'y') code = "-.--";
    else if (message[i] == 'Z' || message[i] == 'z') code = "--..";
    else if (message[i] == '0') code = "-----";
    else if (message[i] == '1') code = ".----";
    else if (message[i] == '2') code = "..---";
    else if (message[i] == '3') code = "...--";
    else if (message[i] == '4') code = "....-";
    else if (message[i] == '5') code = ".....";
    else if (message[i] == '6') code = "-....";
    else if (message[i] == '7') code = "--...";
    else if (message[i] == '8') code = "---..";
    else if (message[i] == '9') code = "----.";
    else code = " ";
    for (int j=0; j<5 && code[j] != '\0'; j++) {
      expectedMessage[index] = code[j];
      index++;
    }
    i++;
  }
  expectedMessage[index] = '\0';
  Serial.println("Expected message pattern: ");
  Serial.println(expectedMessage);
}

void setMessage(char *message) {
  int messageIndex = 0;
  int codeIndex = 0;  
  
  // Make sure entire expectedMessage is set to 0's
  for (int i = 0; i < maxNumberTaps; i++) expectedMessage[i] = '\0';

  while (message[messageIndex] != 0) {
    for (int letterIndex = 0; letterIndex < 5; letterIndex++) {
      if (morseCode[((message[messageIndex] - 65) * 5) + letterIndex]) {
        expectedMessage[codeIndex] = morseCode[((message[messageIndex] - 65) * 5) + letterIndex];
        codeIndex++;
      }
      else break;       
    }       
    messageIndex++;
  }
  expectedMessage[codeIndex] = 0;

  int i = 0;
  while (message[i]) {
    expectedMessageText[i] = message[i];
    i++;
  }
  expectedMessageText[i] = 0;

  Serial.println("New message has been set:");
  Serial.println(expectedMessageText);
  Serial.println(expectedMessage);
}

