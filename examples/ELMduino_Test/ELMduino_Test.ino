#include "ELMduino.h"




#define DEBUG_PORT Serial
#define ELM_PORT   Serial3




const uint16_t MIN_RPM              = 700;
const uint16_t MAX_RPM              = 2000;
const uint8_t  SAMPLE_PERIOD        = 100;
const uint8_t  TENS_PLACE_START_PIN = 3;
const uint8_t  ONES_PLACE_START_PIN = 4;
const uint8_t  BAR_START_PIN        = 2;

const uint8_t speed_led_pin_array[2][7] = { //--------- one's place
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
const uint8_t seven_seg_pix_map[11][7] = { //--------- 0
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
const uint8_t rpm_array[10] = {39,  // 1 (LED #) - Fully left in HUD - Green
                               14,  // 2
                               15,  // 3
                               16,  // 4
                               17,  // 5
                               18,  // 6
                               19,  // 7
                               20,  // 8
                               21,  // 9
                               22}; // 10 - Fully right in HUD - Red




ELM327 myELM327;

enum fsm{get_speed, 
         get_rpm};
fsm state = get_rpm;




float    rpm;
float    speed_mph;
uint64_t currentTime  = millis();
uint64_t previousTime = currentTime;




void setup()
{
  DEBUG_PORT.begin(115200);
  ELM_PORT.begin(115200);

  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  // initialize all LEDs in display
  setupLEDs();

  // wait a bit for the ELM327 to come online
  delay(2000);

  // connect to ELM327
  while(!myELM327.begin(ELM_PORT))
  {
    DEBUG_PORT.println("Couldn't connect to ELM327, trying again...");
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
          updateLEDs();
        else
          DEBUG_PORT.println("\tTimeout");
        state = get_speed;
        break;
        
      case get_speed:
        if(myELM327.querySpeed_mph(speed_mph))
          updateLEDs();
        else
          DEBUG_PORT.println("\tTimeout");
        state = get_rpm;
        break;
    }
  }

  // extra processing here:
}




void setupLEDs()
{
  initSevenSeg(0);
  initSevenSeg(1);
  initRpmDisp();
}




void updateLEDs()
{
  DEBUG_PORT.print(rpm); DEBUG_PORT.print(" "); DEBUG_PORT.println(speed_mph);

  updateSpeedDisp(speed_mph);
  updateRpmDisp((uint16_t)rpm);
}




void updateSpeedDisp(float speed_mph)
{
  uint8_t tensPlace;
  uint8_t onesPlace;
  uint8_t adjSpeed_mph = (uint8_t)(speed_mph + 0.5);

  if(speed_mph > 99)
    adjSpeed_mph = 99;
  
  tensPlace = adjSpeed_mph / 10;
  if(tensPlace == 0)
    tensPlace = 10; // this will cause a blank to be sent to the display
    
  onesPlace = adjSpeed_mph % 10; // add 0.5 and type-cast in order to propperly round float
  
  updateSevenSeg(0, onesPlace);
  updateSevenSeg(1, tensPlace);
}




void initSevenSeg(uint8_t segNum)
{
  uint8_t value = 10;
  
  for(uint8_t i = 0; i < 7; i++)
  {
    pinMode(speed_led_pin_array[segNum][i], OUTPUT);
    
    if(seven_seg_pix_map[value][i])
      digitalWrite(speed_led_pin_array[segNum][i], LOW);
    else
      digitalWrite(speed_led_pin_array[segNum][i], HIGH);
  }
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
}




void initRpmDisp()
{
  for(uint8_t i = 0; i < 9; i++)
  {
    pinMode(rpm_array[i], OUTPUT);
    digitalWrite(rpm_array[i], HIGH);
  }
}




void updateRpmDisp(uint16_t rpm)
{
  uint16_t adjRPM = constrain(map(rpm, MIN_RPM, MAX_RPM, 1, 10), 1, 10);
  
  for(uint8_t i = 0; i < 9; i++)
  {
    if(adjRPM >= i)
      digitalWrite(rpm_array[i], LOW);
    else
      digitalWrite(rpm_array[i], HIGH);
  }
}
