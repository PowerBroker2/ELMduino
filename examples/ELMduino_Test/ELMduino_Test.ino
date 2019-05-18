#include <Adafruit_PWMServoDriver.h>
#include "ELMduino.h"




const float KMPH_MPH_CONVERT = 0.6213711922;
const uint16_t MIN_RPM = 800;
const uint16_t MAX_RPM = 5200;
const uint16_t MAX_SHIELD_DUTY_CYCLE = 4095;
const uint8_t MAX_NORMAL_DUTY_CYCLE = 255;
const uint8_t SAMPLE_PERIOD = 100;
const uint8_t BRIGHTNESS_PIN = 0;
const uint8_t BAR_START_PIN = 2;



Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
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
uint16_t shieldDutyCycle;
uint16_t normalDutyCycle;




// 0 lit - 1 off
uint8_t seven_seg_pix_map[10][7] = { //--------- 0
                                    {0,  // E
                                     1,  // G
                                     0,  // D
                                     0,  // F
                                     0,  // C
                                     0,  // A
                                     0}, // B
                                     //--------- 1
                                    {0,  // E
                                     1,  // G
                                     1,  // D
                                     0,  // F
                                     1,  // C
                                     1,  // A
                                     1}, // B
                                     //--------- 2
                                    {1,  // E
                                     0,  // G
                                     0,  // D
                                     0,  // F
                                     0,  // C
                                     0,  // A
                                     1}, // B
                                     //--------- 3
                                    {0,  // E
                                     0,  // G
                                     0,  // D
                                     0,  // F
                                     1,  // C
                                     0,  // A
                                     1}, // B
                                     //--------- 4
                                    {0,  // E
                                     0,  // G
                                     1,  // D
                                     0,  // F
                                     1,  // C
                                     1,  // A
                                     0}, // B 
                                     //--------- 5
                                    {0,  // E
                                     0,  // G
                                     0,  // D
                                     1,  // F
                                     1,  // C
                                     0,  // A
                                     0}, // B
                                     //--------- 6
                                    {0,  // E
                                     0,  // G
                                     0,  // D
                                     1,  // F
                                     0,  // C
                                     0,  // A
                                     0}, // B
                                     //--------- 7
                                    {0,  // E
                                     1,  // G
                                     1,  // D
                                     0,  // F
                                     1,  // C
                                     0,  // A
                                     1}, // B
                                     //--------- 8
                                    {0,  // E
                                     0,  // G
                                     0,  // D
                                     0,  // F
                                     0,  // C
                                     0,  // A
                                     0}, // B
                                     //--------- 9
                                    {0,  // E
                                     0,  // G
                                     0,  // D
                                     0,  // F
                                     1,  // C
                                     0,  // A
                                     0}, // B
                                    };




void setup()
{
  Serial.begin(115200);
  Serial3.begin(115200);
  
  delay(2000);

  pwm.begin();
  pwm.setPWMFreq(490);
  Wire.setClock(400000);
  
  myELM327.begin(Serial3);
}




void loop()
{
  currentTime = millis();
  if((currentTime - previousTime) >= SAMPLE_PERIOD)
  {
    previousTime += SAMPLE_PERIOD;

    switch(state)
    {
      case get_rpm:
        if(myELM327.queryRPM(rpm))
          updateLEDs();
        else
          Serial.println("\tTimeout");
        state = get_speed;
        break;
        
      case get_speed:
        if(myELM327.querySpeed(speed_kmph))
        {
          speed_mph = speed_kmph * KMPH_MPH_CONVERT;
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
  uint8_t tensPlace;
  uint8_t onesPlace;
  
  Serial.print(rpm); Serial.print(" "); Serial.println(speed_mph);

  tensPlace = (uint8_t)speed_mph / 10;
  onesPlace = (uint8_t)(speed_mph + 0.5) % 10; // add 0.5 and type-cast in order to propperly round float

  updateBrightness();

  updateSevenSeg(0, onesPlace);
  updateSevenSeg(1, tensPlace);

  updateBar((uint16_t)rpm);
  
  return;
}




void updateBrightness()
{
  uint16_t brightness = analogRead(BRIGHTNESS_PIN);

  brightness = 0; // for testing only - comment out once pot is installed
  
  shieldDutyCycle = map(brightness, 0, 1023, 0, MAX_SHIELD_DUTY_CYCLE);
  normalDutyCycle = map(brightness, 0, 1023, 0, MAX_NORMAL_DUTY_CYCLE);
  
  return;
}




void updateSevenSeg(uint8_t segNum, uint8_t value)
{
  uint8_t startPin = segNum * 7;

  for(uint8_t pwmPin = startPin; pwmPin < (startPin + 7); pwmPin++)
  {
    if(seven_seg_pix_map[value][pwmPin - startPin])
      pwm.setPin(pwmPin, MAX_SHIELD_DUTY_CYCLE); // segment set high (off)
    else
      pwm.setPin(pwmPin, shieldDutyCycle); // segment set low-ish (on)
  }
  
  return;
}




void updateBar(uint16_t rpm)
{
  uint8_t BAR_END_PIN = BAR_START_PIN + 10;
  uint8_t barValue = (uint8_t)(mapFloat(rpm, MIN_RPM, MAX_RPM, BAR_START_PIN, BAR_END_PIN) + 0.5); // add 0.5 and type-cast in order to propperly round float
  
  for(uint8_t barPin = BAR_START_PIN; barPin < BAR_END_PIN; barPin++)
  {
    if(barPin >= barValue)
      analogWrite(barPin, normalDutyCycle); // segment set low-ish (on)
    else
      analogWrite(barPin, MAX_NORMAL_DUTY_CYCLE); // segment set high (off)
  }
  
  return;
}




float mapFloat(long x, long in_min, long in_max, long out_min, long out_max)
{
 return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}



