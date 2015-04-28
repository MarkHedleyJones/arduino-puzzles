#include <A4990MotorShield.h>
#include <Wire.h>

#define PIN_RECEIVER 11
#define PIN_HOOK 2
#define PIN_TRIGGER 12
#define PIN_LED 13
#define RING_OSCILLATIE_PERIOD 30
#define MORSE_FREQUENCY 800
#define MORSE_UNIT_PERIOD_MS 200

/*
LOOP WITH DIALTONE IN BETWEEN
RESET ON HANGUP
TIMINGS AS SET
*/


A4990MotorShield motors;

unsigned char fault = false;
char message[21] = "sos";
char wireMessage[21] = "BLAH";
bool execute = false;
bool ringing = false;
bool on_hook = false;


void stopIfFault() {
  if (motors.getFault()) {
    motors.setSpeeds(0,0);
    fault = true;
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
  Wire.begin(1);
  Wire.onReceive(receiveComms);
  Wire.onRequest(transmitComms);
}

void transmitComms() {
  digitalWrite(13,LOW);
  Serial.println("request for comms..");
  Serial.println(message);
  if (wireMessage[0] == '*') {
    if (wireMessage[1] == 'M') Wire.print(message);
    else if (execute && on_hook && ringing) Wire.print("EX=1;HOOK=1;RING=1  ");
    else if (execute && on_hook && !ringing) Wire.print("EX=1;HOOK=1;RING=0  ");
    else if (execute && !on_hook && ringing) Wire.print("EX=1;HOOK=0;RING=1  ");
    else if (execute && !on_hook && !ringing) Wire.print("EX=1;HOOK=0;RING=0  ");
    else if (!execute && on_hook && ringing) Wire.print("EX=0;HOOK=1;RING=1  ");
    else if (!execute && on_hook && !ringing) Wire.print("EX=0;HOOK=1;RING=0  ");
    else if (!execute && !on_hook && ringing) Wire.print("EX=0;HOOK=0;RING=1  ");
    else if (!execute && !on_hook && !ringing) Wire.print("EX=0;HOOK=0;RING=0  ");
  }
  else Wire.print("MESSAGE UPDATED     ");
}

void receiveComms(int howMany) {
  digitalWrite(13,HIGH);
  Serial.println("Wire message received...");
  char wireIndex = 0;
  char wireInChar = -1;
  while(Wire.available() > 0) {
    wireInChar = Wire.read();
    wireMessage[wireIndex] = wireInChar;
    wireIndex++;
    wireMessage[wireIndex] = '\0';
  }
  if (wireMessage[0] == '*') {
    if (wireMessage[1] == 'E') execute = true;
  }
  else {
    for(wireIndex=0; wireIndex < 20; wireIndex++) {
      message[wireIndex] = wireMessage[wireIndex];
    }
  }
  Serial.println(wireMessage);
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
  motors.setM1Speed(0);
  stopIfFault();
}

void play_morse(String message) {
  int i;
  int j;
  int message_length = message.length();
  String code;
  Serial.print("Send message: ");
  Serial.println(message);
  message.toLowerCase();
  for(i=0; i < message_length; i++) {
    if (phone_on_hook()) return;
    else {
      if (message[i] == 'a') code = ".-";
      else if (message[i] == 'b') code = "-...";
      else if (message[i] == 'c') code = "-.-.";
      else if (message[i] == 'd') code = "-..";
      else if (message[i] == 'e') code = ".";
      else if (message[i] == 'f') code = "..-.";
      else if (message[i] == 'g') code = "--.";
      else if (message[i] == 'h') code = "....";
      else if (message[i] == 'i') code = "..";
      else if (message[i] == 'j') code = ".---";
      else if (message[i] == 'k') code = "-.-";
      else if (message[i] == 'l') code = ".-..";
      else if (message[i] == 'm') code = "--";
      else if (message[i] == 'n') code = "-.";
      else if (message[i] == 'o') code = "---";
      else if (message[i] == 'p') code = ".--.";
      else if (message[i] == 'q') code = "--.-";
      else if (message[i] == 'r') code = ".-.";
      else if (message[i] == 's') code = "...";
      else if (message[i] == 't') code = "-";
      else if (message[i] == 'u') code = "..-";
      else if (message[i] == 'v') code = "...-";
      else if (message[i] == 'w') code = ".--";
      else if (message[i] == 'x') code = "-..-";
      else if (message[i] == 'y') code = "-.--";
      else if (message[i] == 'z') code = "--..";
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
}

void dialTone() {
  tone(PIN_RECEIVER, 400);
}

void loop() {
  dialTone();
  ring();
  noTone(PIN_RECEIVER);
  unsigned int i;
  while (true) {
    noTone(PIN_RECEIVER);
    while(phone_on_hook()) delay(1); // Loop here if phone hung up

    for (i=0; i < 2000 && !phone_on_hook(); i++) delay(1);
    play_morse(message);
    for (i=0; i < 500 && !phone_on_hook(); i++) delay(1);
    dialTone();
    for (i=0; i < 2000 && !phone_on_hook(); i++) delay(1);
  }
}
