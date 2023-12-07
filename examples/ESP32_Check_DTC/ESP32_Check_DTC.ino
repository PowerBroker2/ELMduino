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
dtc_states dtc_state = MILSTATUS;
uint8_t numCodes = 0;
uint32_t dtcStatus = 0;

void setup()
{

    DEBUG_PORT.begin(115200);
    // SerialBT.setPin("1234");
    ELM_PORT.begin("ArduHUD", true);

    DEBUG_PORT.println("Starting connection...");
    if (!ELM_PORT.connect("OBDII"))
    {
        DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 1");
        while (1)
            ;
    }

    if (!myELM327.begin(ELM_PORT, true, 2000))
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
    case CHECKONE:    // Force checking for a single code without first getting a code count
        numCodes = 1; // Runs once after startup to test getDTCCodes(). Enable by setting initial dtc_state.
        dtc_state = DTCCODES;
        break;

    case MILSTATUS: // Gets the number of current DTC codes present

        dtcStatus = myELM327.monitorStatus();

        if (myELM327.nb_rx_state == ELM_SUCCESS)
        {
            DEBUG_PORT.print("monitorStatus() response: ");
            DEBUG_PORT.println(dtcStatus);
            // DEBUG_PORT.println((dtcStatus >> 24) & 0x80);

            // We are only interested in the first byte of the response that
            // encodes the MIL status and number of codes present
            // milStatusOn = ((dtcStatus >> 24) & 0x80) > 0;        // If MSB of the first byte is set (0x80 hex) then the light is on
            numCodes = (uint8_t)((dtcStatus >> 24) - 0x80); // The number of codes present is stored in the last 7 bits
            DEBUG_PORT.print("Number of Codes: ");
            DEBUG_PORT.println(numCodes);
            dtc_state = DTCCODES;
        }
        else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
        {
            myELM327.printError();
            dtc_state = DTCCODES;
        }
        break;

    case DTCCODES: // If current DTC codes were found in previous request, thrn get the codes
        if (numCodes > 0)
        {
            char *foundCodes[numCodes];

            myELM327.currentDTCCodes(foundCodes, numCodes);
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                DEBUG_PORT.print("Response to DTC request: ");
                DEBUG_PORT.println(myELM327.response);
                DEBUG_PORT.println("Codes found: ");

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
