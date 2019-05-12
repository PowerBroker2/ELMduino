#include <ELMduino.h>

ELM327 myELM327;

float speed;
uint64_t currentTime = millis();
uint64_t previousTime = currentTime;
uint16_t samplePeriod = 200;

void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200);
  
  delay(2000);
  
  if(!myELM327.begin(Serial1))
    Serial.println("Couldn't connect to ELM327");

  if(!myELM327.querySpeed(speed))
  {
    //Serial.println("\tTimeout");
  }
  else
    Serial.println(speed);
}

void loop()
{
  currentTime = millis();
  if((currentTime - previousTime) >= samplePeriod)
  {
    if(!myELM327.querySpeed(speed))
    {
      //Serial.println("\tTimeout");
    }
    else
      Serial.println(speed);
  }
}
