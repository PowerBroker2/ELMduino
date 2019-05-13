#include <ELMduino.h>

ELM327 myELM327;

float rpm;
uint64_t currentTime = millis();
uint64_t previousTime = currentTime;
uint16_t samplePeriod = 80;

void setup()
{
  Serial.begin(115200);
  Serial3.begin(115200);
  
  delay(2000);
  
  if(!myELM327.begin(Serial3))
    Serial.println("Couldn't connect to ELM327");

  if(!myELM327.queryRPM(rpm))
  {
    Serial.println("\tTimeout");
  }
  else
    Serial.print("RPM: "); Serial.println(rpm);
}

void loop()
{
  currentTime = millis();
  if((currentTime - previousTime) >= samplePeriod)
  {
    previousTime = currentTime;
    
    if(!myELM327.queryRPM(rpm))
    {
      Serial.println("\tTimeout");
    }
    else
      Serial.print("RPM: "); Serial.println(rpm);
  }
}
