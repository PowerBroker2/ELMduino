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

void checkStatus()
{
    uint32_t dtcStatus = 0;
    bool milStatusOn = 0;

    dtcStatus = myELM327.monitorStatus();

    if (myELM327.nb_rx_state == ELM_SUCCESS)
    {
        DEBUG_PORT.print("Response: ");
        DEBUG_PORT.println(dtcStatus);
        DEBUG_PORT.println((dtcStatus >> 24) & 0x80);
        DEBUG_PORT.println((dtcStatus >> 24) - 0x80);

        // We are only interested in the first byte of the response that
        // encodes the MIL status and number of codes present
        milStatusOn = ((dtcStatus >> 24) & 0x80) > 0;   // if MSB of the first byte is set (0x80 hex) then the light is on
        numCodes = (uint8_t)((dtcStatus >> 24) - 0x80); // and the number of codes present is stored in the last 7 bits

        DEBUG_PORT.print("MIL is on: ");
        DEBUG_PORT.println(milStatusOn);
        DEBUG_PORT.print("Number of Codes: ");
        DEBUG_PORT.println(numCodes);
    }
    else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
    {
        myELM327.printError();
    }
}

void getDTCCodes(uint8_t &numCodes)
{
    uint8_t dtcCodes = 0;

    char *foundCodes[numCodes];

    myELM327.currentDTCCodes(foundCodes, numCodes);

    if (myELM327.nb_rx_state == ELM_SUCCESS)
    {
        DEBUG_PORT.print("Response: ");
        DEBUG_PORT.println(dtcCodes);
        DEBUG_PORT.println("Codes found: ");

        size_t n = sizeof(foundCodes) / sizeof(foundCodes[0]);

        for (int i = 0; i < n; i++)
        {
            DEBUG_PORT.println(foundCodes[i]);
        }
    }

    else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
    {
        myELM327.printError();
    }
}

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
        DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 2");
        while (1)
            ;
    }

    DEBUG_PORT.println("Connected to ELM327");
}

void loop()
{
    switch (dtc_state)
    {
    case CHECKONE:      // Returns only one code (if present) without first getting a code count
        numCodes = 1;
        getDTCCodes(numCodes);
        numCodes = 0;
        dtc_state = MILSTATUS;
        break;

    case MILSTATUS:     // Gets the number of current DTC codes present
        checkStatus();
        dtc_state = DTCCODES;
        break;

    case DTCCODES:      // If current DTC codes were found in previous request, get the codes
        if (numCodes > 0) 
        {
            getDTCCodes(numCodes);
            dtc_state = CHECKONE;
        }
        break;

    default:
        break;
    }

    delay(10000);
}
