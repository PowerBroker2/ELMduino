#include "ELMduino.h"




#define ELM_PORT Serial1




const bool DEBUG        = true;
const int  TIMEOUT      = 2000;
const bool HALT_ON_FAIL = false;




ELM327 myELM327;




typedef enum { ENG_RPM,
               SPEED } obd_pid_states;
obd_pid_states obd_state = ENG_RPM;

float rpm = 0;
float mph = 0;




void setup()
{
  Serial.begin(115200);
  ELM_PORT.begin(115200);

  Serial.println("Attempting to connect to ELM327...");

  if (!myELM327.begin(ELM_PORT, DEBUG, TIMEOUT))
  {
    Serial.println("Couldn't connect to OBD scanner");

    if (HALT_ON_FAIL)
      while (1);
  }

  Serial.println("Connected to ELM327");
}




void loop()
{
  switch (obd_state)
  {
    case ENG_RPM:
    {
      rpm = myELM327.rpm();
      
      if (myELM327.nb_rx_state == ELM_SUCCESS)
      {
        Serial.print("rpm: ");
        Serial.println(rpm);
        obd_state = SPEED;
      }
      else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
      {
        myELM327.printError();
        obd_state = SPEED;
      }
      
      break;
    }
    
    case SPEED:
    {
      mph = myELM327.mph();
      
      if (myELM327.nb_rx_state == ELM_SUCCESS)
      {
        Serial.print("mph: ");
        Serial.println(mph);
        obd_state = ENG_RPM;
      }
      else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
      {
        myELM327.printError();
        obd_state = ENG_RPM;
      }
      
      break;
    }
  }
}
