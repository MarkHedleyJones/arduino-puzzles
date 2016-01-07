#include <Adafruit_NeoPixel.h>
#include <avr/power.h>
#include <Wire.h>

#define PIN 6
#define BUF_LEN 96
#define I2C_LEN 32
#define WIRE_ADDR 3

#define PIN_SPK_1 8
#define PIN_SPK_2 9
#define PIN_LATCH 11
#define PIN_LIGHTS 12

Adafruit_NeoPixel strip = Adafruit_NeoPixel(290, PIN, NEO_GRB + NEO_KHZ800);

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
char transition = 1;
int leds_set_prev = -1;
int brightness;
int transition_blinks = 3;
int trigger_latch = 0;
int state_lights = 1;
int latch_ms = 1000;
int sounds_enabled = 1;

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
  
  leds_grn = 145-leds_set;
  leds_red = leds_set;
  strcat(wire_buffer, ",RED=");
  sprintf(tmp, "%d", leds_grn);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",GRN=");
  sprintf(tmp, "%d", leds_red);
  strcat(wire_buffer, tmp);
  strcat(wire_buffer, ",TRANS=");
  sprintf(tmp, "%d", transition);
  strcat(wire_buffer, tmp);
  
  strcat(wire_buffer, ",TRANS_BLINKS=");
  sprintf(tmp, "%d", transition_blinks);
  strcat(wire_buffer, tmp);
  
  strcat(wire_buffer, ",LIGHTS=");
  sprintf(tmp, "%d", state_lights);
  strcat(wire_buffer, tmp);
  
  strcat(wire_buffer, ",LATCH_MS=");
  sprintf(tmp, "%d", latch_ms);
  strcat(wire_buffer, tmp);

  strcat(wire_buffer, ",SOUNDS=");
  sprintf(tmp, "%d", sounds_enabled);
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
    if (leds_set < 0) leds_set = -1;
    else if (leds_set > 145) leds_set = 145;
    load_status();
  }
  else if (message.indexOf("*BRIGHT=") != -1) {
    leds_brightness = message.substring(8).toInt();
    if (leds_brightness < 0) leds_brightness = 0;
    if (leds_brightness > 255) leds_brightness = 255;
    load_status();
  }
  else if (message.indexOf("*TRANS=") != -1) {
    transition = message.substring(7).toInt();
    load_status();
  }
  else if (message.indexOf("*TRANS_BLINKS=") != -1) {
    transition_blinks = message.substring(14).toInt();
    load_status();
  }
  else if (strcmp(wire_buffer, "*TRIG=") != -1) {
    trigger_latch = 1;
    load_status();
  }
  else if (strcmp(wire_buffer, "*LATCH_MS=") != -1) {
    latch_ms = message.substring(10).toInt();
    load_status();
  }
  else if (strcmp(wire_buffer, "*LIGHTS=") != -1) {
    state_lights = message.substring(8).toInt();
    load_status();
  }
  else if (strcmp(wire_buffer, "*SOUNDS=") != -1) {
    sounds_enabled = message.substring(8).toInt();
    load_status();
  }
  else if (strcmp(wire_buffer, "*BLINK=") != -1) {
    make_pattern = message.substring(7).toInt();
    load_status();
  }
  else strcpy(wire_buffer,"Unknown command");
}

void update_leds(int setleds, int bright) {
  leds_grn = 155-setleds;
  leds_red = setleds;
  if (setleds == -1) {
    for(int i=0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(0,0,0));
  }
  else {
    for(int i=0; i < 145; i++) {
      if(i<leds_red) {
        strip.setPixelColor(i,strip.Color(0,bright,0));
        strip.setPixelColor(289-i,strip.Color(0,bright,0));
      }
      else {
        strip.setPixelColor(i, strip.Color(bright,0,0));
        strip.setPixelColor(289-i, strip.Color(bright,0,0));
      }
    }
  }
  strip.show();
}

void blink_leds(int leds_set, int leds_brightness) {
  update_leds(-1, leds_brightness);
  if (sounds_enabled) play_sound(100,400);
  else delay(100);
  update_leds(leds_set, leds_brightness);
  if (sounds_enabled) play_sound(100,400);
  else delay(100);
}


void setup() {
  Serial.begin(9600);
  strip.begin();
  for (int i=0; i<strip.numPixels(); i++) strip.setPixelColor(i,strip.Color(255,0,0));
  strip.show(); // Initialize all pixels to 'off'
  Wire.begin(WIRE_ADDR);
  Wire.onReceive(receiveComms);
  Wire.onRequest(transmitComms);
  pinMode(PIN_SPK_1, OUTPUT);
  pinMode(PIN_SPK_2, OUTPUT);
  pinMode(PIN_LIGHTS, OUTPUT);
  digitalWrite(PIN_LIGHTS, 0);
}

void play_sound(unsigned long duration, int period) {
  for (unsigned long i=0; i < duration; i++) {
    PORTB = 0b00000001;
    delayMicroseconds(period);
    PORTB = 0b00000010;
    delayMicroseconds(period);
  }  
}

void play_sound_completed() {
  play_sound(100,600);
  play_sound(100,500);
  play_sound(150,400);
}

void lights(bool state) {
  if (state == 0) digitalWrite(PIN_LIGHTS, 1);
  else digitalWrite(PIN_LIGHTS, 0);
}

void latch(bool state) {
  if (state == 0) digitalWrite(PIN_LATCH, 1);
  else digitalWrite(PIN_LATCH, 0);
}

void loop() {
  if (leds_set != leds_set_prev) {
    if (sounds_enabled) play_sound_completed();
    if (transition) {
      brightness = leds_brightness;
      while (brightness > 0) {
        update_leds(leds_set_prev, brightness);
        brightness -= 3;
      }
      brightness = 0;
      for (int i=0; i<4; i++) blink_leds(leds_set_prev, leds_brightness);
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
      blink_leds(leds_set, leds_brightness);
      make_pattern--;
    }
    make_pattern = 0;
  }

  if (trigger_latch > 0) {
    latch(1);
    delay(latch_ms);
    latch(0);
    delay(latch_ms);
    trigger_latch--;
  }

  lights(state_lights);
  
//  delay(200);
}

