#include <Wire.h>

#define BUF_LEN 96
#define I2C_LEN 32
#define MSG_LEN 20
#define PIN_LED 13

const int buttonPin = 12;

const int maxNumberTaps = 100;
const int waitTime = 50;               // Time in us for each tap count
const long timeoutLength = 20000;       // Time in us for timeout
const int maxMessageLength = 20;
const float disambiguationFactor = 0.2; // Zone between - and .
const int i2cAddress = 8;

char defaultMessage[6] = {'B', 'R', 'A', 'V', 'O', 0};
char expectedMessage[maxNumberTaps] = {0};
char expectedMessageText[maxMessageLength] = {0};

int completed = 0;                      // 1 for puzzle done
int totalTaps = 0;                      // Tap counter

unsigned long seconds_since_reset = 0;
char wire_buffer[BUF_LEN+1] = "";
int wire_count = 0;
char tx_buffer[I2C_LEN+1] = "";

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
  Serial.print("Sending part ");
  Serial.print(wire_count);
  Serial.print(": ");
  Serial.println(tx_buffer);
  if (wire_count == 2) wire_count = 0;
  else wire_count++;
  Serial.println("Done");
  digitalWrite(PIN_LED,0);
}

void load_status() {
    strcpy(wire_buffer, ",COMPLETE=");
    if (completed) strcat(wire_buffer, "1");
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
    strcat(wire_buffer, ",TAP_COUNT=");
    sprintf(tmp, "%d", totalTaps);
    strcat(wire_buffer, tmp);
    strcat(wire_buffer, ",MSG=");
    strcat(wire_buffer, expectedMessageText);
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
    setMessage(defaultMessage);
    load_status();
  }
  else if (wire_buffer[0] == '*' &&
           wire_buffer[1] == 'M' &&
           wire_buffer[2] == 'S' &&
           wire_buffer[3] == 'G' &&
           wire_buffer[4] == '=') {
    for (i=0; i<MSG_LEN; i++) {
      char newMessage[20] = {0};
      if (wire_buffer[i+5] == 32 ||
          (wire_buffer[i+5] >= 48 && wire_buffer[i+5] <= 57) ||
          (wire_buffer[i+5] >= 65 && wire_buffer[i+5] <= 90) ||
          (wire_buffer[i+5] >= 97 && wire_buffer[i+5] <= 122)) {
        newMessage[i] = wire_buffer[i+5];
      }
      else {
        newMessage[i] = '\0';
        break;
      }
      setMessage(newMessage);
    }
    load_status();
  }
}

void setup() 
{
    pinMode(buttonPin, INPUT);
    pinMode(13, OUTPUT);
    digitalWrite(13, LOW);
    Serial.begin(9600);
    Wire.begin(i2cAddress);
    Wire.onReceive(receiveComms);
    Wire.onRequest(transmitComms);
    setMessage(defaultMessage);
}

void loop() 
{
  int tapLengths[maxNumberTaps] = {0};
  
  // Wait for the first press
  while(digitalRead(buttonPin)) {}
  
  // Taps have started
  readTaps(tapLengths);

  char message[maxNumberTaps] = {0};
  decodeTaps(message, tapLengths);

  // Decide if taps correct or not
  completed |= compareTaps(message);

  // Write out tap lenghts via serial
  for (int i = 0; i < maxNumberTaps; i++)  {
    if (tapLengths[i] != 0) Serial.println(tapLengths[i]);
    else break;
  }

  // Print what was tapped
  Serial.println(message);

  if (completed) {
    digitalWrite(13, HIGH);
    Serial.println("Correct message! - Waiting for reset..."); 
    while (completed) {};
  }
  else Serial.println("Soz G");
}

void readTaps(int *tapLengths)
{
  int upTime = 0;
  int timedOut = 0;
  int index = 0;

  while(!timedOut) {
    upTime = 0;
    while(!digitalRead(buttonPin)) {
      tapLengths[index]++;
      delayMicroseconds(waitTime);
    }

    // Tap finished
    while (digitalRead(buttonPin)) {
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

void setMessage(char *message)
{
  int messageIndex = 0;
  int codeIndex = 0;  
  
  // Make sure entire expectedMessage is set to 0's
  for (int i = 0; i < maxNumberTaps; i++) expectedMessage[i] = 0;

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

void receiveCommand(int howMany)
{
    char command[32] = {0};
    int messageIndex = 0;
    while (Wire.available())
    {
        command[messageIndex] = Wire.read();
        if (messageIndex == 32)
        {
            break;
        }
        messageIndex++;
    }

    // *MSG
    if (command[0] == '*' && command[1] == 'M' && command[2] == 'S' && command[3] == 'G' && command[4] == '=')
    {
        // Command is new message
        // Convert the message to uppercase
        messageIndex = 0;
        char newMessage[20] = {0};

        while (command[messageIndex + 5] != 0)
        {
            if ((command[messageIndex + 5] > 96) && (command[messageIndex + 5] < 123))
            {
                newMessage[messageIndex] = command[messageIndex + 5] - 32;
            }
            else
            {
                newMessage[messageIndex] = command[messageIndex + 5];
            }
            messageIndex++;
        }
        newMessage[messageIndex] = 0;
        setMessage(newMessage);
        returnStatus();
    }
    // *RST
    else if (command[0] == '*' && command[1] == 'R' && command[2] == 'S' && command[3] == 'T')
    {
        reset();
        returnStatus();
    }
    // *TRIG
    else if (command[0] == '*' && command[1] == 'T' && command[2] == 'R' && command[3] == 'I' && command[4] == 'G')
    {
        completed = 1;
        returnStatus();
    }
    // *IDN?
    else if (command[0] == '*' && command[1] == 'I' && command[2] == 'D' && command[3] == 'N' && command[4] == '?')
    {
        Serial.println("Morse code interpreter v1.0");
        Wire.write("Morse code interpreter v1.0");
    }
    // *STAT?
    else if (command[0] == '*' && command[1] == 'S' && command[2] == 'T' && command[3] == 'A' && command[4] == 'T' && command[5] == '?')
    {
        returnStatus();
    }
}

void returnStatus()
{
    Serial.print("TAPS=");
    Serial.print(totalTaps);
    Serial.print(",MSG=");
    Serial.print(expectedMessageText);
    Serial.print(",CMPLT=");
    if (completed)
    {
        Serial.println("T");
    }
    else
    {
        Serial.println("F");
    }
    
    Wire.write("TAPS=");
    Wire.write(totalTaps);
    Wire.write(",MSG=");
    Wire.write(expectedMessageText);
    Wire.write(",CMPLT=");
    if (completed)
    {
        Wire.write("T");
    }
    else
    {
        Wire.write("F");
    }
    Wire.write(0);
}

void reset() {
    totalTaps = 0;
    completed = 0;
}
