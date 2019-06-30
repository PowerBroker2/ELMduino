#include "ELMduino.h"




const float KMPH_MPH_CONVERT = 0.6213711922;
const uint16_t MIN_RPM = 800;
const uint16_t MAX_RPM = 5200;
const uint8_t SAMPLE_PERIOD = 100;
const uint8_t TENS_PLACE_START_PIN = 3;
const uint8_t ONES_PLACE_START_PIN = 4;
const uint8_t BAR_START_PIN = 2;



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




uint8_t speed_led_pin_array[2][7] = { //--------- one's place
                                     {23,  // A
                                      24,  // B
                                      25,  // C
                                      26,  // D
                                      27,  // E
                                      28,  // F
                                      36}, // G
                                      //--------- ten's place
                                     {29,  // A
                                      30,  // B
                                      31,  // C
                                      32,  // D
                                      33,  // E
                                      34,  // F
                                      35}  // G
                                    };

// 1 lit - 0 off
uint8_t seven_seg_pix_map[11][7] = { //--------- 0
                                    {1,  // A
                                     1,  // B
                                     1,  // C
                                     1,  // D
                                     1,  // E
                                     1,  // F
                                     0}, // G
                                     //--------- 1
                                    {0,  // A
                                     0,  // B
                                     0,  // C
                                     0,  // D
                                     1,  // E
                                     1,  // F
                                     0}, // G
                                     //--------- 2
                                    {1,  // A
                                     0,  // B
                                     1,  // C
                                     1,  // D
                                     0,  // E
                                     1,  // F
                                     1}, // G
                                     //--------- 3
                                    {1,  // A
                                     0,  // B
                                     0,  // C
                                     1,  // D
                                     1,  // E
                                     1,  // F
                                     1}, // G
                                     //--------- 4
                                    {0,  // A
                                     1,  // B
                                     0,  // C
                                     0,  // D
                                     1,  // E
                                     1,  // F
                                     1}, // G
                                     //--------- 5
                                    {1,  // A
                                     1,  // B
                                     0,  // C
                                     1,  // D
                                     1,  // E
                                     0,  // F
                                     1}, // G
                                     //--------- 6
                                    {1,  // A
                                     1,  // B
                                     1,  // C
                                     1,  // D
                                     1,  // E
                                     0,  // F
                                     1}, // G
                                     //--------- 7
                                    {1,  // A
                                     0,  // B
                                     0,  // C
                                     0,  // D
                                     1,  // E
                                     1,  // F
                                     0}, // G
                                     //--------- 8
                                    {1,  // A
                                     1,  // B
                                     1,  // C
                                     1,  // D
                                     1,  // E
                                     1,  // F
                                     1}, // G
                                     //--------- 9
                                    {1,  // A
                                     1,  // B
                                     0,  // C
                                     1,  // D
                                     1,  // E
                                     1,  // F
                                     1}, // G
                                     //--------- blank
                                    {0,  // A
                                     0,  // B
                                     0,  // C
                                     0,  // D
                                     0,  // E
                                     0,  // F
                                     0}, // G
                                   };
uint8_t rpm_array[10] = {39,
                         14,
                         15,
                         16,
                         17,
                         18,
                         19,
                         20,
                         21,
                         22};




void setup()
{
  Serial.begin(115200);
  Serial3.begin(115200);

  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  // wait a bit for the ELM327 to come online
  delay(2000);

  // initialize all LEDs in display
  setupLEDs();

  // connect to ELM327
  while(!myELM327.begin(Serial3))
  {
    Serial.println("Couldn't connect to ELM327, trying again...");
    delay(1000);
  }
}




void loop()
{
  // only query the ELM327 every so often
  currentTime = millis();
  if((currentTime - previousTime) >= SAMPLE_PERIOD)
  {
    previousTime += SAMPLE_PERIOD;

    switch(state)
    {
      case get_rpm:
        if(myELM327.queryRPM(rpm))
        {
          rpm = rpm / 4.0; // necessary conversion factor based off OBD-II standard
          updateLEDs();
        }
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

  // extra processing here:
}




void setupLEDs()
{
  for(uint8_t segment = 0; segment < 2; segment++)
    for(uint8_t pin = 0; pin < 7; pin++)
      pinMode(speed_led_pin_array[segment][pin], OUTPUT);

  // blank out display
  initSevenSeg(0);
  initSevenSeg(1);

  initRpmDisp();
  
  return;
}




void updateLEDs()
{
  Serial.print(rpm); Serial.print(" "); Serial.println(speed_mph);

  updateSpeedDisp(speed_mph);
  updateBar((uint16_t)rpm);
  
  return;
}




void updateSpeedDisp(float speed_mph)
{
  uint8_t tensPlace;
  uint8_t onesPlace;
  float adjSpeed_mph = speed_mph;

  if(speed_mph > 99)
    adjSpeed_mph = 99;
  
  tensPlace = (uint8_t)adjSpeed_mph / 10;
  if(tensPlace == 0)
    tensPlace = 10; // this will cause a blank to be sent to the display
    
  onesPlace = (uint8_t)(adjSpeed_mph + 0.5) % 10; // add 0.5 and type-cast in order to propperly round float
  
  updateSevenSeg(0, onesPlace);
  updateSevenSeg(1, tensPlace);
}




void initSevenSeg(uint8_t segNum)
{
  uint8_t value = 10;
  
  for(uint8_t i = 0; i < 7; i++)
  {
    if(seven_seg_pix_map[value][i])
      digitalWrite(speed_led_pin_array[segNum][i], LOW);
    else
      digitalWrite(speed_led_pin_array[segNum][i], HIGH);
  }
  
  return;
}




void updateSevenSeg(uint8_t segNum, uint8_t value)
{
  for(uint8_t i = 0; i < 7; i++)
  {
    if(seven_seg_pix_map[value][i])
      digitalWrite(speed_led_pin_array[segNum][i], LOW);
    else
      digitalWrite(speed_led_pin_array[segNum][i], HIGH);
  }
  
  return;
}




void initRpmDisp()
{
  for(uint8_t i = 0; i < 9; i++)
  {
    pinMode(rpm_array[i], OUTPUT);
    digitalWrite(rpm_array[i], HIGH);
  }
}




void updateBar(uint16_t rpm)
{
  uint16_t adjRPM = constrain(map(rpm, 700, 2000, 1, 10), 1, 10);
  
  for(uint8_t i = 0; i < 9; i++)
  {
    if(adjRPM >= i)
      digitalWrite(rpm_array[i], LOW);
    else
      digitalWrite(rpm_array[i], HIGH);
  }
  
  return;
}
