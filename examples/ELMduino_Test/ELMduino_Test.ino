#include "ELMduino.h"




#define DEBUG_PORT Serial
#define ELM_PORT   Serial3




const uint32_t DEBUG_BAUD = 115200;
const uint32_t ELM_BAUD   = 115200;
const uint16_t MIN_RPM    = 700;
const uint16_t MAX_RPM    = 3500;

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




ELM327 myELM;

enum fsm{get_speed, 
         get_rpm};
fsm state = get_rpm;




float    rpm;
float    speed_mph;




void setup()
{
  DEBUG_PORT.begin(DEBUG_BAUD);
  ELM_PORT.begin(ELM_BAUD);

  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  // initialize all LEDs in display
  setupLEDs();

  // connect to ELM327
  while(!myELM.begin(ELM_PORT))
  {
    DEBUG_PORT.println("Couldn't connect to ELM327, trying again...");
    delay(1000);
  }
}




void loop()
{
  switch(state)
  {
    //------------------------------------------------ RPM
    case get_rpm:
      myELM.queryRPM();
      
      if(myELM.available())
      {
        rpm = myELM.rpm();
        updateLEDs();
        state = get_speed;
      }
      else if(myELM.timeout())
      {
        DEBUG_PORT.println("\tTimeout");
        state = get_speed;
      }
      break;

    //------------------------------------------------ MPH
    case get_speed:
      myELM.querySpeed_kph();
      
      if(myELM.available())
      {
        speed_mph = myELM.mph();
        updateLEDs();
        state = get_rpm;
      }
      else if (myELM.timeout())
      {
        DEBUG_PORT.println("\tTimeout");
        state = get_rpm;
      }
      break;
  }

  //------------------------------------------------ Extra processes
  // here
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
  uint8_t adjSpeed_mph = (uint8_t)(speed_mph + 0.5); // add 0.5 and type-cast in order to propperly round float
  
  tensPlace = adjSpeed_mph / 10;
  if(tensPlace == 0)
    tensPlace = 10; // this will cause a blank to be sent to the display
    
  onesPlace = adjSpeed_mph % 10;
  
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
  for(uint8_t i = 0; i < 10; i++)
  {
    pinMode(rpm_array[i], OUTPUT);
    digitalWrite(rpm_array[i], HIGH);
  }
}




void updateRpmDisp(uint16_t rpm)
{
  uint16_t adjRPM = constrain(map(rpm, MIN_RPM, MAX_RPM, 0, 9), 0, 9);
  
  for(uint8_t i = 0; i < 10; i++)
  {
    if(adjRPM >= i)
      digitalWrite(rpm_array[i], LOW);
    else
      digitalWrite(rpm_array[i], HIGH);
  }
}
