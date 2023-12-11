#include "ELMduino.h"
#include <BluetoothSerial.h>

#define ELM_PORT SerialBT
#define DEBUG_PORT Serial

typedef enum
{
    CHECKONE,
    MILSTATUS,
    DTCCODES
} dtc_states;

BluetoothSerial SerialBT;
ELM327 myELM327;
dtc_states dtc_state = CHECKONE;
uint8_t numCodes = 0;
uint32_t dtcStatus = 0;

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

    if (!myELM327.begin(ELM_PORT, false, 2000))
    {
        DEBUG_PORT.println("ELM327 Couldn't connect to ECU - Phase 2");
        while (1)
            ;
    }

    DEBUG_PORT.println("Connected to ELM327");
}

void loop()
{
    switch (dtc_state)
    {
    case CHECKONE:    // Runs once after startup to test getDTCCodes(). Enable by setting initial dtc_states
        numCodes = 3; // Force checking for a single code without first getting a code count
        dtc_state = DTCCODES;
        break;

    // This is the typical use case: First check if any codes are present, and then make a request to get them.
    // No need to fetch the codes if none are present and we need know how many codes to expect back.
    case MILSTATUS: // Gets the number of current DTC codes present

        dtcStatus = myELM327.monitorStatus();

        if (myELM327.nb_rx_state == ELM_SUCCESS)
        {            
            // We are only interested in the third byte of the response that
            // encodes the MIL status and number of codes present
            numCodes = myELM327.responseByte_3 - 0x80;

            DEBUG_PORT.print("Number of codes present: ");
            DEBUG_PORT.println(numCodes);
            dtc_state = DTCCODES;
        }
        else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
        {
            myELM327.printError();
            dtc_state = DTCCODES;
        }
        break;

    case DTCCODES: // If current DTC codes were found in previous request, then get the codes
        if (numCodes > 0)
        {
            char foundCodes[numCodes][6] = {0};
          
            myELM327.currentDTCCodes(foundCodes, numCodes, false); 

            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                DEBUG_PORT.println("Current DTCs found: ");

                // Get the actual number of codes returned in case it is different than expected (numCodes)
                size_t n = sizeof(foundCodes) / sizeof(foundCodes[0]);

                for (int i = 0; i < n; i++)
                {
                    DEBUG_PORT.println(foundCodes[i]);
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
