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
uint16_t samplePeriod = 80;




void setup()
{
  Serial.begin(115200);
  Serial3.begin(115200);
  
  delay(2000);
}




void loop()
{
  currentTime = millis();
  if((currentTime - previousTime) >= samplePeriod)
  {
    previousTime = currentTime;

    switch(state)
    {
      case get_rpm:
        if(myELM327.queryRPM(rpm))
        {
          Serial.print("RPM: "); Serial.println(rpm);
          updateLEDs();
        }
        else
          Serial.println("\tTimeout");
        state = get_speed;
        break;
        
      case get_speed:
        if(!myELM327.querySpeed(speed_kmph))
        {
          speed_mph = speed_kmph * 0.621371;
          Serial.print("Speed (mph): "); Serial.println(speed_mph);
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
  
  
  return;
}
