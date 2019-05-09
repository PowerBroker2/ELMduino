#include <ELMduino.h>

ELM327 myELM327;

float value;

void setup()
{
  Serial.begin(115200);
  while(!Serial);
  
  myELM327.begin(Serial);

  if(!myELM327.queryPID(SERVICE_01, VEHICLE_SPEED, 2, value))
    Serial.println("\tTimeout");
  else
    Serial.println(value);
}

void loop()
{
  
}
