#include <SPI.h>
#include <Wire.h>
#include <Ethernet.h>

//#define SCL_PIN 5
//#define SCL_PORT PORTC
//#define SDA_PIN 4
//#define SDA_PORT PORTC
//#define I2C_TIMEOUT 1000
//#define I2C_SLOWMODE 1
//#include <SoftI2CMaster.h>


#define PIN_LED 4
#define PIN_BUTTON 3
#define PIN_MORSE 5

#define BUFLEN 96
#define STATE_PRE_RUN         0
#define STATE_RUNNING         1
#define STATE_ENDCONDITION    2
#define STATE_ENDED           3
#define STATE_RESET           4

#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

Adafruit_7segment matrix = Adafruit_7segment();

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 177);
EthernetServer server(80);

void reset_room(void);

char state = 0;


char success = false;
char clock_paused = 0;
unsigned long seconds_since_reset = 0;
char update_flag = 1;
char message_given = 0;
char i2c_msg[BUFLEN] = {0};

int counter = 0;
bool first_entry = 1;
char second = 0;
char second_prev = 0;
char btn_state = 0;
char wire_buffer[BUFLEN] = "";

void setup() {
  delay(1000);
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  matrix.begin(0x70);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_MORSE, INPUT_PULLUP);
}

/**
/ Takes a string (input) and extracts a value (*value) given a key (key)
**/
void get_value_with_key(char* input, char* key, char* value) {
  int key_index_start = 0;
  int key_index_end = 0;
  int len_match = strlen(key);
  boolean match = true;
  for (int i=0; i < BUFLEN - len_match; i++) {

    key_index_start = i;
    match = true;

    for (int j=0; j < len_match; j++) {
      if (input[i+j] != key[j]) {
        match = false;
        break;
      }
    }

    if (match) {
      key_index_end = key_index_start + len_match;
      break;
    }
  }
  // If we found 'command=' in input, copy the command value
  if (key_index_end != 0) {
    for (int i=0; i < BUFLEN; i++) {
      if (input[key_index_end + i] == '\0' || input[key_index_end + i] == '&') break;
      value[i] = input[key_index_end + i];
    }
  }
}

void url_decode(char* string) {
  int string_length = strlen(string);
  char replacement = 0;
  for (int i=0; i<string_length; i++) {
    if (string[i] == '+') string[i] = ' ';
    else if (string[i] == '%') {
      if (i <= string_length - 2) {
        if (string[i+1] == '3' && string[i+2] == 'F') replacement = '?';
        else if (string[i+1] == '2' && string[i+2] == 'B') replacement = '+';
        else if (string[i+1] == '2' && string[i+2] == '3') replacement = '#';
        else if (string[i+1] == '4' && string[i+2] == '0') replacement = '@';
        else if (string[i+1] == '2' && string[i+2] == '1') replacement = '!';
        else if (string[i+1] == '2' && string[i+2] == '4') replacement = '$';
        else if (string[i+1] == '2' && string[i+2] == '5') replacement = '%';
        else if (string[i+1] == '5' && string[i+2] == 'E') replacement = '^';
        else if (string[i+1] == '2' && string[i+2] == '6') replacement = '&';
        else if (string[i+1] == '2' && string[i+2] == '8') replacement = '(';
        else if (string[i+1] == '2' && string[i+2] == '9') replacement = ')';
        else if (string[i+1] == '2' && string[i+2] == 'C') replacement = ',';
        else if (string[i+1] == '2' && string[i+2] == 'F') replacement = '/';
        else if (string[i+1] == '3' && string[i+2] == 'A') replacement = ':';
        else if (string[i+1] == '3' && string[i+2] == 'D') replacement = '=';

        if (replacement != 0) {
          string[i] = replacement;
          for (int j=i+1; j<string_length; j++) string[j] = string[j+2];
          replacement = 0;
        }
      }
    }
  }
}

void parse_command(char* params, char* command, int* device_addr) {
  char key_command[] = "command=";
  char key_device[] = "device=";
  char device_string[BUFLEN];
  for (int i=0; i<BUFLEN; i++) device_string[i] = '\0';
  get_value_with_key(params, key_command, command);
  get_value_with_key(params, key_device, device_string);
  url_decode(command);
  *device_addr = atoi(device_string);
}

char communicate_with_device(int device, char* command) {
  boolean comms_broke = false;
  boolean ack = false;
  Wire.beginTransmission(device);
  for (int i=0; i < BUFLEN; i++) {
    Wire.write(command[i]);
  }
  Wire.endTransmission();
  delay(50);
  Wire.requestFrom(device, 32);
  for (int i = 0; i < BUFLEN; i++) command[i] = '\0';
  for (int i = 0; i < 32; i++) {
    command[i] = Wire.read();
    if (command[i] == (char) 0xff || command[i] == '\0') {
      command[i] = '\0';
      comms_broke = true;
      break;
    }
  }
  delay(10);
  Wire.requestFrom(device, 32);
  for (int i = 32; i < 64; i++) {
    command[i] = Wire.read();
    if (command[i] == (char) 0xff || command[i] == '\0') {
      command[i] = '\0';
      comms_broke = true;
      break;
    }
  }
  delay(10);
  Wire.requestFrom(device, 32);
  for (int i = 64; i < 95; i++) {
    command[i] = Wire.read();
    if (command[i] == (char) 0xff || command[i] == '\0') {
      command[i] = '\0';
      comms_broke = true;
      break;
    }
  }
  while(Wire.available()) Wire.read();

  return 1;
}

void clock_routine() {
  second = millis() / 1000;
  if (second != second_prev) {
   if(clock_paused == 0) {
    update_clock();
   }
   else seconds_since_reset++;
   update_flag = 2;
  }
  second_prev = second;
}

void update_clock() {
    unsigned long seconds = millis();
    seconds = seconds / 1000;
    seconds = seconds - seconds_since_reset;
    int mins = ((seconds / 60)) % 6000;
    seconds = seconds - (mins * 60) % 360000;
    matrix.drawColon(true);
    matrix.writeDigitNum(0, (mins / 10), true);
    matrix.writeDigitNum(1, (mins % 10), true);
    matrix.writeDigitNum(3, (seconds / 10) % 60, true);
    matrix.writeDigitNum(4, seconds % 10, true);
    matrix.writeDisplay();
}

void set_status_clock() {
    char tmp[4] = "";
    unsigned long seconds = millis();
    seconds = seconds / 1000;
    seconds = seconds - seconds_since_reset;
    int mins = ((seconds / 60)) % 6000;
    seconds = seconds - (mins * 60) % 360000;
    strcpy(wire_buffer, "TIME=");
    sprintf(tmp, "%d", (mins / 10));
    strcat(wire_buffer, tmp);
    sprintf(tmp, "%d", (mins % 10));
    strcat(wire_buffer, tmp);
    strcat(wire_buffer, ":");
    sprintf(tmp, "%d", (seconds / 10));
    strcat(wire_buffer, tmp);
    sprintf(tmp, "%d", (seconds % 10));
    strcat(wire_buffer, tmp);
    strcat(wire_buffer, ",PAUSED=");
    if (clock_paused) strcat(wire_buffer, "1");
    else strcat(wire_buffer, "0");
}

void set_status_secret_message() {
    char tmp[4] = "";
    unsigned long seconds = millis();
    seconds = seconds / 1000;
    seconds = seconds - seconds_since_reset;
    int mins = ((seconds / 60)) % 6000;
    seconds = seconds - (mins * 60) % 360000;
    strcpy(wire_buffer, "TIME=");
    sprintf(tmp, "%d", (mins / 10));
    strcat(wire_buffer, tmp);
    sprintf(tmp, "%d", (mins % 10));
    strcat(wire_buffer, tmp);
    strcat(wire_buffer, ":");
    sprintf(tmp, "%d", (seconds / 10));
    strcat(wire_buffer, tmp);
    sprintf(tmp, "%d", (seconds % 10));
    strcat(wire_buffer, tmp);
    strcat(wire_buffer, ",MSG_RECEIVED=");
    if (message_given) strcat(wire_buffer, "1");
    else strcat(wire_buffer, "0");
}

            
void handle_http_client(EthernetClient client) {
  char wire_prevCommand[BUFLEN] = "";
  char http_buffer[BUFLEN] = "";
  char http_params[BUFLEN] = "";
  int index = 0;
  String line;
  // an http request ends with a blank line
  boolean server_relay = false;
  boolean currentLineIsBlank = true;
  boolean client_receive = true;
  while (client.connected()) {
    if (client.available()) {
      while (client_receive) {
        char c = client.read();
        if (c == '\n' && currentLineIsBlank) client_receive = false;
        else if (c == '\n') {
          currentLineIsBlank = true;
          http_buffer[index] = '\n';
          index++;
          for (int i = 0; (index + i) < BUFLEN; i++) http_buffer[index+i] = '\0';
          index = 0;
          line = String(http_buffer);
          if (line.startsWith("GET") && line.indexOf("device=") != -1 && line.indexOf("command=") != -1) {
            for (int i = 0; i < BUFLEN; i++) http_params[i] = '\0';
            for (int i = 6; i < (BUFLEN-6); i++) {
              if (http_buffer[i] == ' ') break;
              else http_params[i-6] = http_buffer[i];
              server_relay = true;
            }
          }
        }
        else if (c != '\r') {
          currentLineIsBlank = false;
          http_buffer[index] = c;
          index++;
        }
      }
      
      // Finished receiving data from the client, time to respond
      if (server_relay) {
        
        int device_addr;
        for (int i=0; i<BUFLEN; i++) wire_buffer[i] = '\0';
        parse_command(http_params, wire_buffer, &device_addr);

        client.println("HTTP/1.1 200 OK");
        client.println("Access-Control-Allow-Origin: *");
        client.println("Content-Type: application/json");
        client.println("Connection: close");  // the connection will be closed after completion of the response
        client.println();
        
        for (int i=0; i<BUFLEN; i++) wire_prevCommand[i] = wire_buffer[i];
        if (device_addr == 0) {
          char tmp[BUFLEN] = {0};
          strcpy(tmp, wire_buffer);
          String message = String(wire_buffer);
          client.println("{");
          for (int i=3; i<11; i++) {
            if (i == 4) {
              set_status_clock();
              strcpy(i2c_msg, wire_buffer);
            }
            else if (i == 10) {
              set_status_secret_message();
              strcpy(i2c_msg, wire_buffer);
            }
            else {
              clock_routine();
              Serial.println(tmp);
              if (strcmp(tmp,"*RST") == 0) strcpy(i2c_msg, "*RST");
              else strcpy(i2c_msg, "*STAT?");
              communicate_with_device(i, i2c_msg);
            }
            client.print("\"");
            client.print(i);
            client.print("\":\"");
            client.print(i2c_msg);
            client.print("\"");
            if (i < 10) client.print(",\n");
            else client.print("\n");
            delay(1);
          }
          client.println("}");
          if (strcmp(tmp, "*RST") == 0) {
            state = STATE_RESET;
            counter = 0;
            first_entry = 1;
          }
          break;
        }
        else if (device_addr == 4) {
          String message = String(wire_buffer);
          if (strcmp(wire_buffer,"*RST") == 0) {
            seconds_since_reset = millis() / 1000;
            update_clock();
          }
          else if (message.indexOf("*PAUSE=") != -1)  {
            clock_paused = message.substring(7).toInt();
          }
          else if (message.indexOf("*TIME=") != -1)  {
            char mins = message.substring(6,8).toInt();
            char secs = message.substring(9,11).toInt();
            seconds_since_reset = (millis() / 1000) - (mins * 60) - secs;
            update_clock();
          }
            set_status_clock();
        }
        else if (device_addr == 10) {
          String message = String(wire_buffer);
          if (strcmp(wire_buffer,"*RST") == 0) {
            message_given = 0;
          }
          else if (strcmp(wire_buffer,"*SET") == 0) {
            message_given = 1;
          }
          set_status_secret_message();
        }
        else if (communicate_with_device(device_addr, wire_buffer) == false) {
          strcpy(wire_buffer,"No response");
          strcat(wire_buffer,'\0');
        }
        client.println("{");
        client.print(" \"device\":");
        client.print(device_addr);
        client.println(",");
        client.print(" \"command\":\"");
        client.print(wire_prevCommand);
        client.println("\",");
        client.print(" \"response\":\"");
        client.print(wire_buffer);
        client.println("\"");
        client.println("}");
        break;
      }
      else {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");  // the connection will be closed after completion of the response
        client.println();
        client.println("<html>");
        client.println("Server working, but nothing to see here");
        client.println("</html>");
        break;
      }
    }
    Serial.println("Disconnect");
  }
  // give the web browser time to receive the data
  delay(1);
  // close the connection:
  client.stop();

  // Clean up
  for (int i = 0; i < BUFLEN; i++) http_buffer[i] = '\0';
  index = 0;
}

void loop() {
  
  EthernetClient client = server.available();
  if (client) handle_http_client(client);

  switch (state) {

    case STATE_PRE_RUN:
      if (first_entry) {
        Serial.println("STATE_PRE_RUN");
        clock_paused = 1;
        seconds_since_reset = millis() / 1000;
        update_clock();
        btn_state = 0;
        for (int i=0; i<10; i++) {
          digitalWrite(PIN_LED, i % 2);
          delay(75);
        }
      }
      digitalWrite(PIN_LED, 0);

      // Debounce the button press
      if (digitalRead(PIN_BUTTON) == btn_state) counter++;
      else counter = 0;

      // On press: transition to state_running
      if (counter > 300 && btn_state == 0) {
        btn_state = 1;
        counter = 0;
      }
      if (counter > 300 && btn_state == 1) {
        state = STATE_RUNNING;
        first_entry = 1;
      }
      else first_entry = 0;
      break;
      
    case STATE_RUNNING:
      if (first_entry) {
        Serial.println("STATE_RUNNING");
        counter = 0;
        clock_paused = 0;
        seconds_since_reset = millis() / 1000;
        strcpy(i2c_msg, "*SET_PROG=0");
        communicate_with_device(3, i2c_msg);
      }
      
      // Debounce the button press and check for hard reset
      if (digitalRead(PIN_BUTTON) == 0) {
        counter++;
        delay(10);

        // This is a reset (button held for 3s)
        if (counter > 300) {
          first_entry = 1;
          state = STATE_RESET;
          break;
        }
      }
      else if (counter > 0) message_given = 1;
      
      if (message_given || digitalRead(PIN_MORSE) == 0) {
        state = STATE_ENDCONDITION;
        first_entry = 1;
      }
      else first_entry = 0;
      break;
    
    case STATE_ENDCONDITION:
      if (first_entry) {
        counter = 0;
        Serial.println("STATE_ENDCONDITION");
      }

      // Debounce the button press and check for hard reset
      if (digitalRead(PIN_BUTTON) == 0) {
        counter++;
        delay(10);

        // This is a reset (button held for 3s)
        if (counter > 300) {
          first_entry = 1;
          state = STATE_RESET;
          break;
        }
      }
      
      digitalWrite(PIN_LED, millis()/500 % 2);
      if (digitalRead(PIN_BUTTON) == 0) message_given = 1;
      if (message_given && digitalRead(PIN_MORSE) == 0) {
        state = STATE_ENDED;
        first_entry = 1;
      }
      else first_entry = 0;
      break;
    
    case STATE_ENDED:
      if (first_entry) {
        Serial.println("STATE_ENDED");
        strcpy(i2c_msg, "*SET_PROG=150");
        communicate_with_device(3, i2c_msg);
        clock_paused = 1;
      }
      digitalWrite(PIN_LED, 1);
      if (digitalRead(PIN_BUTTON) == 0) {
        counter++;
        delay(10);
      }
      else counter = 0;
      if (counter > 300) {
        state = STATE_RESET;
        counter = 0;
        first_entry = 1;
      }
      else first_entry = 0;
      break;

    case STATE_RESET:
      if (first_entry) {
        Serial.println("STATE_RESET");
        counter = 0;
        strcpy(i2c_msg, "*RST");
        communicate_with_device(3, i2c_msg);
        strcpy(i2c_msg, "*RST");
        communicate_with_device(7, i2c_msg);
        strcpy(i2c_msg, "*RST");
        communicate_with_device(5, i2c_msg);
        strcpy(i2c_msg, "*RST");
        communicate_with_device(6, i2c_msg);
        strcpy(i2c_msg, "*RST");
        communicate_with_device(8, i2c_msg);
        strcpy(i2c_msg, "*RST");
        communicate_with_device(9, i2c_msg);
      }
      message_given = 0;
      digitalWrite(PIN_LED, 0);
      if (digitalRead(PIN_BUTTON) == 1) counter++;
      else counter = 0;
      if (counter > 100) {
        state = STATE_PRE_RUN;
        first_entry = 1;
      }
      else first_entry = 0;
      break;
  }
  
  clock_routine();
}
