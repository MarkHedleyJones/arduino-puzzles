#include <Adafruit_NeoPixel.h>
#include <avr/power.h>
#include <Wire.h>

#define PIN 6

Adafruit_NeoPixel strip = Adafruit_NeoPixel(300, PIN, NEO_GRB + NEO_KHZ800);

int i = 0;
void setup() {
  Serial.begin(9600);
  strip.begin();
  for (i=0; i<strip.numPixels(); i++) strip.setPixelColor(i,strip.Color(255,0,0));
  strip.show(); // Initialize all pixels to 'off'
  Wire.begin(2);
  Wire.onReceive(receiveComms);
}

int progress = 0;
unsigned char update = false;

void receiveComms(int howMany) {
  delay(1);
  progress = Wire.parseInt();
  delay(1);
  while(Wire.available()) Wire.read();
  update = true;
}

void loop() {
  while(!update) delay(1);
  for(i=0; i <strip.numPixels(); i++) {
    if(i<progress) strip.setPixelColor(i,strip.Color(0,255,0));
    else strip.setPixelColor(i, strip.Color(255,0,0));
  }
  update = false;
  strip.show();
}
