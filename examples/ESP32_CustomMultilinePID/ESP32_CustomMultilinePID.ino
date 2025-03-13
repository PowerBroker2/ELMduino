/* 

This example shows how to query and process OBD2 standard PIDs that
return a multiline response and require custom processing.

Processing Response with a custom calculation function
In this example, we manually extract the data value from the query response and perform
some post-processing to calculate the correct value. 

Managing Query State
We also demonstrate managing the query state used by the loop() method. This is typically
managed internally by ELMduino for standard PID methods. 

*/

#include "BluetoothSerial.h"
#include "ELMduino.h"

BluetoothSerial SerialBT;
#define ELM_PORT SerialBT
#define DEBUG_PORT Serial

ELM327 myELM327;

int nb_query_state = SEND_COMMAND; // Set the inital query state ready to send a command


// Applies calculation formula for PID 0x22 (Fuel Rail Pressure)
double calcDPF() {
    double B = myELM327.payload[6];
    double C = myELM327.payload[5];
    return ((B * 256) + C) / 100;
}

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

}
 
void loop() 
{
    if (nb_query_state == SEND_COMMAND)         // We are ready to send a new command
    {
        myELM327.sendCommand("017A");          // Send the PID commnad
        nb_query_state = WAITING_RESP;          // Set the query state so we are waiting for response
    }
    else if (nb_query_state == WAITING_RESP)    // Our query has been sent, check for a response
    {
        myELM327.get_response();                // Each time through the loop we will check again
    }
    
    if (myELM327.nb_rx_state == ELM_SUCCESS)    // Our response is fully received, let's get our data
    {          
        double dpf = myELM327.conditionResponse(calcDPF); // Apply the formula for dpf                                             
        Serial.println(dpf);                    // Print the adjusted value
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