# ELMduino
[![GitHub version](https://badge.fury.io/gh/PowerBroker2%2FELMduino.svg)](https://badge.fury.io/gh/PowerBroker2%2FELMduino) [![arduino-library-badge](https://www.ardu-badge.com/badge/ELMDuino.svg?)](https://www.ardu-badge.com/ELMDuino)<br /><br />
This is a simple yet powerful library to effortlessly interface your Arduino with an ELM327 OBD-II scanner. With this library, you can query any and all OBD-II supported PIDs to collect a wide variety of car data (i.e. speed, rpm, engine temp, etc). Also, you can use ELMduino to view and clear your car's trouble codes - no need to go to AutoZone anymore!

# Example Project:

[![youtube](https://user-images.githubusercontent.com/20977405/102416634-2fa0ab80-3fc8-11eb-8baf-d24a6a27bd3f.PNG)](https://www.youtube.com/watch?v=JxBvukUipc4&feature=youtu.be)

# Install:
Install ELMduino using the Arduino IDE's Libraries Manager (search "ELMduino.h")

# Available Features:
See the contents of ELMduino.h for lists of supported OBD PID processing functions, OBD protocols, standard PIDs, and AT commands. The associated functions are documented in "doc strings" in ELMduino.cpp.

# Note:
If you're having difficulty in connecting/keeping connection to your ELM327, try using 38400 baud instead of 115200. If you still have trouble, try all other possible bauds. Lastly, if using BluetoothSerial on an ESP32, try using the ELM327's MAC address instead of the device name "OBDII" and [remove paired devices using this sketch](https://github.com/espressif/arduino-esp32/blob/master/libraries/BluetoothSerial/examples/bt_remove_paired_devices/bt_remove_paired_devices.ino).

# Concept of Execution
The library is non-blocking. This means when you query a PID e.g. `myELM327.rpm()`, the code does not wait around for the response, which would block your other code in the main loop from executing. With ELMDuino, your main loop can continue do other tasks. To make this work, you need to repeatedly call the PID query function and check the non-blocking receive state (`myELM327.nb_rx_state`) until it is equal to `ELM_SUCCESS`. If the status is not `ELM_SUCCESS`, the library could still be waiting for a response to be received. This is indicated by `myELM327.nb_rx_state` being equal to `ELM_GETTING_MSG`. If the status is not equal to either of these values (ELM_SUCCESS or ELM_GETTING_MSG), it indicates an error has occurred. You can call `myELM327.printError()` to check what the problem was. See the simple example below which queries the engine speed in RPM.

Just to be clear, do not try to query more than one PID at a time. You must wait for the current PID query to complete before starting the next one.

# Example Code:
```C++
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
```
