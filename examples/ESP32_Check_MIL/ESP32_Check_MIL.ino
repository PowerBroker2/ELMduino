#include "ELMduino.h"
#include <BluetoothSerial.h>

#define ELM_PORT SerialBT
#define DEBUG_PORT Serial

typedef enum
{   MILSTATUS,
    DTCCODES
} dtc_states;

BluetoothSerial SerialBT;
ELM327 myELM327;
dtc_states dtc_state = MILSTATUS;
uint8_t numCodes = 0;
uint8_t milStatus =0;

// This example program demonstrates how to use ELMduino to check the MIL (Check Engine Light)
// status and the number of current DTC codes that are present. 

void setup()
{
    DEBUG_PORT.begin(115200);
    ELM_PORT.begin("ArduHUD", true);
    ELM_PORT.setPin("1234");

    DEBUG_PORT.println("Starting connection...");
    if (!ELM_PORT.connect("ELMULATOR"))
    {
        DEBUG_PORT.println("Couldn't connect to ELM327 - Phase 1");
        while (1)
            ;
    }

    if (!myELM327.begin(ELM_PORT))
    {
        DEBUG_PORT.println("ELM327 Couldn't connect to ECU - Phase 2");
        while (1)
            ;
    }

    DEBUG_PORT.println("Connected to ELM327");
    
}

void loop()
{
    myELM327.monitorStatus(); // Gets the number of current DTC codes present

    if (myELM327.nb_rx_state == ELM_SUCCESS)
    {            
        // We are only interested in the third byte of the response that
        // encodes the MIL status and number of codes present
        milStatus = (myELM327.responseByte_2 & 0x80);
        numCodes = (myELM327.responseByte_2 - 0x80);

        DEBUG_PORT.print("MIL (Check Engine Light) is: ");
        if (milStatus)
        {
            DEBUG_PORT.println("ON.");
        }
        else
        {
            DEBUG_PORT.println("OFF.");
        }
        
        DEBUG_PORT.print("Number of codes present: ");
        DEBUG_PORT.println(numCodes);

        DEBUG_PORT.println("Checking again in 10 seconds.\n\n");
        delay(10000);   // Wait 10 seconds before we query again.
    }
    else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
    {
        myELM327.printError();
        DEBUG_PORT.println("Trying again in 10 seconds.\n\n");
        delay(10000);
    }
}
