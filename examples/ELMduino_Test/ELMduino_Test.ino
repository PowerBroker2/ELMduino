#include <ELMduino.h>

ELM327 myELM327;

float value;
uint64_t currentTime = millis();
uint64_t previousTime = currentTime;
uint16_t samplePeriod = 200;

void setup()
{
  Serial.begin(115200);
  while(!Serial);
  
  if(!myELM327.begin(Serial))
    Serial.println("Couldn't connect to ELM327");

  if(!myELM327.queryPID(SERVICE_01, VEHICLE_SPEED, 2, value))
    Serial.println("\tTimeout");
  else
    Serial.println(value);
}

void loop()
{
  currentTime = millis();
  if((currentTime - previousTime) >= samplePeriod)
  {
    if(!myELM327.queryPID(SERVICE_01, VEHICLE_SPEED, 2, value))
      Serial.println("\tTimeout");
    else
      Serial.println(value);
  }
}
