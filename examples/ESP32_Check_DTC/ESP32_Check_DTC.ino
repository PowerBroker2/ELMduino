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

void setup()
{
    DEBUG_PORT.begin(115200);
    ELM_PORT.begin("ArduHUD", true);
    ELM_PORT.setPin("1234");

    DEBUG_PORT.println("Starting connection...");
    if (!ELM_PORT.connect("ELMULATOR"))
    {
        DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 1");
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

    delay(1000);

    // Demonstration of calling currentDTCCodes in blocking (default) mode
    // This is the simplest use case. Just scan for codes without first
    // checking if MIL is on or how many codes are present.
    
    DEBUG_PORT.println("Performing DTC check in blocking mode...");

    myELM327.currentDTCCodes(); 
    if (myELM327.nb_rx_state == ELM_SUCCESS)
    {
        DEBUG_PORT.println("Current DTCs found: ");
        
        for (int i = 0; i < myELM327.DTC_Response.codesFound; i++)
        {
            DEBUG_PORT.println(myELM327.DTC_Response.codes[i]);
        }
        delay(10000); // Pause for 10 sec after successful fetch of DTC codes.
    }

    else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
    {
        myELM327.printError();
    }
}

void loop()
{
     // This is the typical use case: First check if any codes are present, and then make a request to get them.
     // monitorStatus() is a non-blocking call and must be called repeatedly until a response is found.
    switch (dtc_state)
    {
    case MILSTATUS: 
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
            dtc_state = DTCCODES;
            break;
        }
        else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
        {
            myELM327.printError();
            dtc_state = DTCCODES;
        }
        break;

    case DTCCODES: // If current DTC codes were found in previous request, then retrieve the codes
        if (numCodes > 0)
        {            
            myELM327.currentDTCCodes(false); // Call in NB mode
          
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                DEBUG_PORT.println("Current DTCs found: ");
                
                for (int i = 0; i < myELM327.DTC_Response.codesFound; i++)
                {
                    DEBUG_PORT.println(myELM327.DTC_Response.codes[i]);
                }
                dtc_state = MILSTATUS;
                delay(10000); // Pause for 10 sec after successful fetch of DTC codes.
            }

            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                dtc_state = MILSTATUS;
            }
        }
        break;

    default:
        break;
    }
}
