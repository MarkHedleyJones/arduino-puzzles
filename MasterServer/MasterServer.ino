
#include <Wire.h>

void setup()
{
  Serial.begin(9600);
  Wire.begin(); // join i2c bus (address optional for master)
}

char inChar=-1;
char index=0;
char message[21] = "ba";
byte x = 0;
int device;
void loop()
{
  index=0;
  inChar=-1;
  device=0;
  Serial.println("Ready for command");
  while (Serial.available() <= 0) delay(1);
  device=Serial.parseInt();
  delay(1);
  while (Serial.available() > 0) {
    inChar = Serial.read();
    if (inChar != ':') {
      message[index] = inChar;
      index++;
      message[index] = '\0';
    }
    delay(1);
  }
  Serial.print("Sending command to device: ");
  Serial.println(device);
  Serial.print("Command: ");
  Serial.print(message);

  Serial.println("Sending command...");
  Wire.beginTransmission(device); // transmit to device #4
  Wire.print(message);        // sends five bytes
  Wire.endTransmission();    // stop transmitting
  Serial.println("Sent");
  delay(10);
  if (device == 1) {
    Serial.print("Receiving response from ");
    Serial.println(device);
//    Wire.beginTransmission(device);
    Serial.println("Opened device");
    delay(1);
    int available = Wire.requestFrom(device,(char) 20);
    Serial.println(available);
    inChar=-1;
    for (index=0; index < 20; index++)
    {
      inChar = Wire.read();
      message[index] = inChar;
      message[index+1] = '\0';
      delay(1);
    }
    Wire.read();
//    Wire.endTransmission();
    Serial.print("Response: ");
    Serial.println(message);
    // x++;
    // delay(500);
  }
}
