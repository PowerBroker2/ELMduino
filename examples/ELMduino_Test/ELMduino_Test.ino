#include <ELMduino.h>




ELM327 myELM327;

enum fsm{
  get_speed, 
  get_rpm};
fsm state = get_rpm;

float rpm;
float speed_kmph;
float speed_mph;
uint64_t currentTime = millis();
uint64_t previousTime = currentTime;
uint16_t samplePeriod = 100;




void setup()
{
  Serial.begin(115200);
  Serial3.begin(115200);
  
  delay(2000);

  myELM327.begin(Serial3);
}




void loop()
{
  currentTime = millis();
  if((currentTime - previousTime) >= samplePeriod)
  {
    previousTime += samplePeriod;

    switch(state)
    {
      case get_rpm:
        if(myELM327.queryRPM(rpm))
        {
          updateLEDs();
        }
        else
          Serial.println("\tTimeout");
        state = get_speed;
        break;
        
      case get_speed:
        if(myELM327.querySpeed(speed_kmph))
        {
          speed_mph = speed_kmph * 0.621371;
          updateLEDs();
        }
        else
          Serial.println("\tTimeout");
        state = get_rpm;
        break;
    }
  }
}




void updateLEDs()
{
  Serial.print(rpm); Serial.print(" "); Serial.println(speed_mph);
  
  return;
}
