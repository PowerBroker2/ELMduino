/* 

This example shows how to perform several opearations for using custom (non-OBD2 standard)
queries. 

Custom Header
We perform a query that requires a custom header to be sent first. The custom header in this 
case determines which ECU in the vehicle's system we want to query.

Custom PID
The query sent to the vehicle is also a custom (non OBD2 standard) PID using the 0x22
(enhanced data) mode.

Manually Processing Response
In this example, we manually extract the data value from the query response and perform
some post-processing to calculate the correct value. 

Managing Query State
We also demonstrate managing the query state used by the loop() method. This is typically
managed internally by ELMduino for standard PID methods. 

Customize all the things!
The header value, PID, data value bytes and adjustment formula are generally unique for 
each different vehicle and PID. You will need to source the correct values for those for 
your specific vehicle. This example almost certainly will not work for you "as-is".

*/

#include "BluetoothSerial.h"
#include "ELMduino.h"

BluetoothSerial SerialBT;
#define ELM_PORT SerialBT
#define DEBUG_PORT Serial

ELM327 myELM327;

int nb_query_state = SEND_COMMAND; // Set the inital query state ready to send a command

void setup() 
{    
    DEBUG_PORT.begin(115200);
    // SerialBT.setPin("1234");
    ELM_PORT.begin("ArduHUD", true);

    if (!ELM_PORT.connect("OBDII"))
    {
        DEBUG_PORT.println("Couldn't connect to ELM327 device.");
        while (1);
    }

    if (!myELM327.begin(ELM_PORT, true, 2000))
    {
        Serial.println("ELM327 device couldn't connect to ECU.");
        while (1);
    }

    Serial.println("Connected to ELM327");

    // Set a custom header using ATSH command. 
    myELM327.sendCommand_Blocking("ATSH DA18F1");
}
 
void loop() 
{
    if (nb_query_state == SEND_COMMAND)         // We are ready to send a new command
    {
        myELM327.sendCommand("2204FE");         // Send the custom PID commnad
        nb_query_state = WAITING_RESP;          // Set the query state so we are waiting for response
    }
    else if (nb_query_state == WAITING_RESP)    // Our query has been sent, check for a response
    {
        myELM327.get_response();                // Each time through the loop we will check again
    }
    
    if (myELM327.nb_rx_state == ELM_SUCCESS)    // Our response is fully received, let's get our data
    {      
        int A = myELM327.payload[6];            // Parse the temp value A from the response
        float temperature = A - 40;             // Apply the formula for the temperature
        Serial.println(temperature);            // Print the adjusted value
        nb_query_state = SEND_COMMAND;          // Reset the query state for the next command
        delay(5000);                            // Wait 5 seconds until we query again
    }
    
    else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
    {                                           // If state == ELM_GETTING_MSG, response is not yet complete. Restart the loop.
        nb_query_state = SEND_COMMAND;          // Reset the query state for the next command
        myELM327.printError();
        delay(5000);                            // Wait 5 seconds until we query again
    }
}