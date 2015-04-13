#include <A4990MotorShield.h>
#include <Wire.h>
#define PIN_RECEIVER 12

/*
LOOP WITH DIALTONE IN BETWEEN
RESET ON HANGUP
TIMINGS AS SET
*/


/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.

  This example code is in the public domain.
 */
//test
A4990MotorShield motors;

/*
 * For safety, it is good practice to monitor motor driver faults and handle
 * them in an appropriate way. If a fault is detected, both motor speeds are set
 * to zero and it is reported on the serial port.
 4:30:15
 4:43:10
 */
void stopIfFault()
{
  if (motors.getFault())
  {
    motors.setSpeeds(0,0);
    Serial.println("Fault");
    while(1);
  }
}

// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
int led = 13;
char message[21] = "hamburgers are us";
char wireMessage[21]="abc";
bool execute = false;
bool ringing = false;
bool on_hook = false;

// the setup routine runs once when you press reset:
void setup() {
  Serial.begin(9600);
  // initialize the digital pin as an output.
  pinMode(led, INPUT);
  pinMode(13, OUTPUT);
  pinMode(11, INPUT);
  Serial.write("test\n");
  digitalWrite(12, HIGH);
  delay(1000);
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


void oscillate(float seconds, float period) {
   float i;
   float duration = seconds * 1000.0;
   for(i=0; i < duration; i += period) {
     delay(period/2.0);
     if (analogRead(A0) < 512) return;
     motors.setM1Speed(400);
     stopIfFault();
     delay(period/2.0);
     motors.setM1Speed(-400);
     stopIfFault();
   }
}

void ring(float ring, float pause) {
  ringing = true;
  // Ring for 2.0 seconds, pause for 4.0 seconds
  digitalWrite(PIN_RECEIVER, HIGH);
  float i;
  float duration = pause * 1000.0;
    while(1) {
      oscillate(ring, 60);
      for(i=0; i < duration; i++) {
        if (analogRead(A0) < 512) {
          digitalWrite(PIN_RECEIVER, LOW);
          ringing = false;
          return;
        }
        delay(1);
      }
    }
}

void play_morse(String message, unsigned int frequency, unsigned int unit_period_ms) {
  int i;
  int j;
  int message_length = message.length();
  String code;
  Serial.print("Send message: ");
  Serial.println(message);
  message.toLowerCase();
  for(i=0; i < message_length; i++) {
    switch(message[i]) {
      case 'a':
      code = ".-";
      break;

      case 'b':
      code = "-...";
      break;

      case 'c':
      code = "-.-.";
      break;

      case 'd':
      code = "-..";
      break;

      case 'e':
      code = ".";
      break;

      case 'f':
      code = "..-.";
      break;

      case 'g':
      code = "--.";
      break;

      case 'h':
      code = "....";
      break;

      case 'i':
      code = "..";
      break;

      case 'j':
      code = ".---";
      break;

      case 'k':
      code = "-.-";
      break;

      case 'l':
      code = ".-..";
      break;

      case 'm':
      code = "--";
      break;

      case 'n':
      code = "-.";
      break;

      case 'o':
      code = "---";
      break;

      case 'p':
      code = ".--.";
      break;

      case 'q':
      code = "--.-";
      break;

      case 'r':
      code = ".-.";
      break;

      case 's':
      code = "...";
      break;

      case 't':
      code = "-";
      break;

      case 'u':
      code = "..-";
      break;

      case 'v':
      code = "...-";
      break;

      case 'w':
      code = ".--";
      break;

      case 'x':
      code = "-..-";
      break;

      case 'y':
      code = "-.--";
      break;

      case 'z':
      code = "--..";
      break;

      case '0':
      code = "-----";
      break;

      case '1':
      code = ".----";
      break;

      case '2':
      code = "..---";
      break;

      case '3':
      code = "...--";
      break;

      case '4':
      code = "....-";
      break;

      case '5':
      code = ".....";
      break;

      case '6':
      code = "-....";
      break;

      case '7':
      code = "--...";
      break;

      case '8':
      code = "---..";
      break;

      case '9':
      code = "----.";
      break;

      default:
      code = " ";
    }
//    Serial.println(message[i]);

    // Play a space between words
    if (code == " ") {
        // Wait an extra 4 units on top of the previous 3 to make 7
        noTone(PIN_RECEIVER);
        delay(unit_period_ms * 10);
    }
    else {
      // Play a letter
      for(j=0; j<5 && code[j] != '\0'; j++) {
//        Serial.println(code[j]);
        if(code[j] == '.') {
          tone(PIN_RECEIVER, frequency);
          Serial.print("*");
          delay(unit_period_ms);
        }
        else {
          tone(PIN_RECEIVER, frequency);
          Serial.print("***");
          delay(unit_period_ms * 3);
        }
        noTone(PIN_RECEIVER);
        Serial.print("_");
        delay(unit_period_ms);
      }
      noTone(PIN_RECEIVER);
      Serial.print("__");
      delay(unit_period_ms * 5);
    }
  }
}
int i;
int repeat = 2;
// the loop routine runs over and over again forever:
void loop() {
//  char inChar=-1;
//  char index=0;
//  execute=true;
//
//  Serial.println("Playing dial tone");
//  tone(PIN_RECEIVER, 200);
//  Serial.println("Waiting for message");
//  while (execute == false) {
//    delay(1);
//  }
////  while (Serial.available() <= 0) {
////    delay(1);
////    if (execute) break;
////  }
////  Serial.println("Data received, storing new message");
////  while (Serial.available() > 0 || execute) {
////    inChar = Serial.read();
////    message[index] = inChar;
////    index++;
////    message[index] = '\0';
////    delay(10);
////  }
//  Serial.print("Message = ");
//  Serial.print(message);
//  Serial.println("Waiting for receiver hang-up");
//  while(1) {
//        on_hook = false;
//  	if (analogRead(A0) > 512) break;
//        on_hook = true;
//  }
//  noTone(PIN_RECEIVER);
//  delay(10);
//  Serial.println("Receiver on hook");
//  Serial.println("Ringing phone");
//  ring(2.0, 3.5);
//  Serial.println("Receiver picked up");
//  for(i=0; i<repeat; i++) {
//    delay(3000);
//    Serial.println("Playing message");
//    play_morse(message, 1000, 100);
//    Serial.println("Finished playing message");
//    execute = false;
//  }
  tone(PIN_RECEIVER, 200);
  delay(4000);
  noTone(PIN_RECEIVER);
  delay(1000);
  play_morse(message, 1000, 200);
}
