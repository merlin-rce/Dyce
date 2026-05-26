#include <Arduino.h>
#include <Encoder.h>
Encoder encoder(6, 5);

void setup()
{
  Serial.begin(9600);
  encoder.begin();
}

void loop()
{
  int step = encoder.read();
  if (step > 0)      Serial.println("COUNTERCLOCKWISE");
  else if (step < 0) Serial.println("CLOCKWISE");
}
