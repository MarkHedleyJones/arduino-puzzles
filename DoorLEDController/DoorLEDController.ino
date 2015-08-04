#include <Adafruit_NeoPixel.h>
#include <avr/power.h>
#include <Wire.h>

#define PIN 6
#define BUF_LEN 96
#define I2C_LEN 32
#define WIRE_ADDR 3

Adafruit_NeoPixel strip = Adafruit_NeoPixel(300, PIN, NEO_GRB + NEO_KHZ800);

unsigned long seconds_since_reset = 0;
int wire_count = 0;
char wire_buffer[BUF_LEN+1] = {0};
char tx_buffer[I2C_LEN+1] = {0};
int leds_grn = 0;
int leds_red = 0;
int leds_set = -1;
int leds_brightness = 64;
int leds_int = 0;
char make_pattern = 0;
char transition = 0;
int leds_set_prev = -1;
int brightness;

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
  
  leds_grn = 150-leds_set;
  leds_red = leds_set;
  strcat(wire_buffer, ",LEDS_RED=");
  sprintf(tmp, "%d", leds_grn);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",LEDS_GRN=");
  sprintf(tmp, "%d", leds_red);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",TRANSITION=");
  sprintf(tmp, "%d", transition);
  strcat(wire_buffer, tmp);
  
  
  // TERMINATE THE WIREBUFFER
  strcat(wire_buffer, 0);
}

void receiveComms(int howMany) {
  String message;
  digitalWrite(13,1);
  int i;
  for (i=0; i<BUF_LEN; i++) wire_buffer[i] = '\0';
  i = 0;
  while (Wire.available() > 0) {
    wire_buffer[i] = Wire.read();
    i++;
  }
  message = String(wire_buffer);
  if (strcmp(wire_buffer, "*IDN?") == 0) strcpy(wire_buffer,"Door LED Controller");
  else if (strcmp(wire_buffer, "*STAT?") == 0) load_status();
  else if (strcmp(wire_buffer, "*RST") == 0) {
    seconds_since_reset = millis() / 1000;
    leds_set = -1;
    load_status();
  }
  else if (message.indexOf("*SET_PROG=") != -1) {
    leds_set = message.substring(10).toInt();
    if (leds_set < -1) leds_set = -1;
    else if (leds_set > 150) leds_set = 150;

    load_status();
  }
  else if (message.indexOf("*SET_BRIG=") != -1) {
    leds_brightness = message.substring(10).toInt();
    if (leds_brightness < 0) leds_brightness = 0;
    if (leds_brightness > 255) leds_brightness = 255;
    load_status();
  }
  else if (message.indexOf("*SET_TRANS=") != -1) {
    transition = message.substring(11).toInt();
    load_status();
  }
  else if (strcmp(wire_buffer, "*BLINK=") != -1) {
    make_pattern = message.substring(7).toInt();
    load_status();
  }
  else strcpy(wire_buffer,"Unknown command");
  Serial.println(leds_grn);
  Serial.println(leds_red);
  Serial.println(leds_set);
  Serial.println(leds_brightness);
  Serial.println();
}

void update_leds(char setleds, int bright) {
  leds_grn = 150-setleds;
  leds_red = setleds;
  if (setleds == -1) {
    for(int i=0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(0,0,0));
  }
  else {
    for(int i=0; i < 150; i++) {
      if(i<leds_red) {
        strip.setPixelColor(i,strip.Color(0,bright,0));
        strip.setPixelColor(299-i,strip.Color(0,bright,0));
      }
      else {
        strip.setPixelColor(i, strip.Color(bright,0,0));
        strip.setPixelColor(299-i, strip.Color(bright,0,0));
      }
    }
  }
  strip.show();
}

void setup() {
  Serial.begin(9600);
  strip.begin();
  for (int i=0; i<strip.numPixels(); i++) strip.setPixelColor(i,strip.Color(255,0,0));
  strip.show(); // Initialize all pixels to 'off'
  Wire.begin(WIRE_ADDR);
  Wire.onReceive(receiveComms);
  Wire.onRequest(transmitComms);
}

void loop() {
  if (leds_set != leds_set_prev) {
    if (transition) {
      brightness = leds_brightness;
      while (brightness > 0) {
        update_leds(leds_set_prev, brightness);
        brightness -= 3;
      }
      brightness = 0;
      while (brightness < leds_brightness) {
        update_leds(leds_set, brightness);
        brightness += 3;
      }
    }
    else {
      update_leds(leds_set, leds_brightness);
    }
    leds_set_prev = leds_set;
  }
  else update_leds(leds_set, leds_brightness);
  if (make_pattern) {
    while (make_pattern > 0) {
      update_leds(-1, leds_brightness);
      delay(100);
      update_leds(leds_set, leds_brightness);
      delay(100);
      make_pattern--;
    }
    make_pattern = 0;
  }
  delay(200);
}

