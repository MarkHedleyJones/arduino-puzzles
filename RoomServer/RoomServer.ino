#include <SPI.h>
#include <Ethernet.h>

#define SCL_PIN 5
#define SCL_PORT PORTC
#define SDA_PIN 4
#define SDA_PORT PORTC
#define I2C_TIMEOUT 1000
#define I2C_SLOWMODE 1
#include <SoftI2CMaster.h>

#define BUFLEN 96

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 2, 177);
EthernetServer server(80);

char http_buffer[BUFLEN] = "";
char wire_buffer[BUFLEN] = "";
char http_params[BUFLEN] = "";
char wire_prevCommand[BUFLEN] = "";
char temp[BUFLEN] = "";
//char comms_response[BUFLEN] = "";
int index = 0;
int device = 0;
char success = false;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  i2c_init();
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

void parse_command(char* http_params, char* command, int* device_addr) {
  char key_command[] = "command=";
  char key_device[] = "device=";
  char device_string[BUFLEN];
  for (int i=0; i<BUFLEN; i++) device_string[i] = '\0';
  get_value_with_key(http_params, key_command, command);
  get_value_with_key(http_params, key_device, device_string);
  url_decode(command);
  Serial.print("Command: ");
  Serial.println(command);
  Serial.print("device_string: ");
  Serial.println(device_string);
  Serial.print("device_integer: ");
  Serial.println(atoi(device_string));
  *device_addr = atoi(device_string);
}

char communicate_with_device(int device, char* command) {
  boolean comms_broke = false;
  boolean ack = false;
  Serial.println(command);
  Serial.println("Starting to wait");
  i2c_start((device << 1) | I2C_WRITE);
  Serial.println("Waiting over");
  for (int i=0; i < BUFLEN; i++) {
    if (!i2c_write(command[i])) {
      Serial.println("Slave not acknowledging - Aborting");
      return 0;
    }
    if (command[i] == '\0') break;
  }
  i2c_stop();
  Serial.print("Sent '");
  Serial.print(command);
  Serial.print("' to device: ");
  Serial.println(device);
  delay(200);
  i2c_start((device << 1) | I2C_READ);
  for (int i = 0; i < BUFLEN; i++) command[i] = '\0';
  for (int i = 0; i < 32; i++) {
    command[i] = i2c_read(false);
    Serial.print(command[i]);
    if (command[i] == (char) 0xff || command[i] == '\0') {
      command[i] = '\0';
      comms_broke = true;
      Serial.println("Device terminated transmission");
      break;
    }
  }
  i2c_stop();
  delay(50);
  i2c_start((device << 1) | I2C_READ);
  Serial.println();
  for (int i = 32; i < 64; i++) {
    command[i] = i2c_read(false);
    Serial.print(command[i]);
    if (command[i] == (char) 0xff || command[i] == '\0') {
      command[i] = '\0';
      comms_broke = true;
      Serial.println("Device terminated transmission");
      break;
    }
  }
  i2c_stop();
  delay(50);
  i2c_start((device << 1) | I2C_READ);
  Serial.println();
  for (int i = 64; i < 95; i++) {
    command[i] = i2c_read(false);
    Serial.print(command[i]);
    if (command[i] == (char) 0xff || command[i] == '\0') {
      command[i] = '\0';
      comms_broke = true;
      Serial.println("Device terminated transmission");
      break;
    }
  }
  Serial.println();
  if (comms_broke == false) command[96] = i2c_read(true);
  i2c_stop();
  Serial.print("Device responded: '");
  Serial.print(command);
  Serial.println("'");
  return 1;
}

void loop() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("---------------------------------");
    Serial.println("new client");
    // an http request ends with a blank line
    boolean redirect = false;
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
            if (http_buffer[0] == 'G' && http_buffer[1] == 'E' && http_buffer[2] == 'T' && http_buffer[5] == '?') {
              for (int i = 0; i < BUFLEN; i++) http_params[i] = '\0';
              for (int i = 6; i < (BUFLEN-6); i++) {
                if (http_buffer[i] == ' ') break;
                else http_params[i-6] = http_buffer[i];
                redirect = true;
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
        if (redirect) {
          Serial.println("Redirecting client (got GET parameters)");
          // Redirect the client, but remember - we have data to act on.
          client.println("HTTP/1.1 301 Moved Permanently");
          client.println("Location: /");
          client.println();

          int device_addr;
          for (int i=0; i<BUFLEN; i++) wire_buffer[i] = '\0';
          parse_command(http_params, wire_buffer, &device_addr);
          for (int i=0; i<BUFLEN; i++) wire_prevCommand[i] = wire_buffer[i];
//          for (int i=0; i<BUFLEN; i++) wire_buffer[i] = '\0';
          Serial.print("The device is: ");
          Serial.println(device_addr);
          if (communicate_with_device(device_addr, wire_buffer) == false) {
            strcpy(wire_buffer,"No response");
            strcat(wire_buffer,'\0');

          }
          break;
        }
        else {
          Serial.println("Sending page");
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<form action=\"/\" method=\"GET\">");
          client.println("  <input type=\"text\" name=\"device\" value=\"8\">");
          client.println("  <input type=\"text\" name=\"command\" value=\"\">");
          client.println("  <input type=\"submit\" value=\"Send\">");
          client.println("</form>");
          client.println("<html>");
          client.println("<h2>Command</h2>");
          client.println("<pre>");
          client.println(wire_prevCommand);
          client.println("</pre>");
          client.println("<h2>Response</h2>");
          client.println("<pre>");
          client.println(wire_buffer);
          client.println("</pre>");
          client.println("</html>");
          break;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();

    Serial.println("client disconnected");

    // Clean up
    for (int i = 0; i < BUFLEN; i++) http_buffer[i] = '\0';
    index = 0;
  }
}
