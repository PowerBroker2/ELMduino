#include "ELMduino.h"




/*
 bool ELM327::begin(Stream &stream)

 Description:
 ------------
  * Constructor for the ELM327 Class; initializes ELM327

 Inputs:
 -------
  * Stream &stream - Pointer to Serial port connected
  to ELM327

 Return:
 -------
  * bool - Whether or not the ELM327 was propperly
  initialized
*/
bool ELM327::begin(Stream &stream)
{
	elm_port = &stream;

	while (!elm_port);

	// wait 3 sec for the ELM327 to initialize
	delay(3000);

	// try to connect
	while (!initializeELM())
		delay(1000);
	
	delay(100);
  
	return true;
}




/*
 bool ELM327::initializeELM()

 Description:
 ------------
  * Initializes ELM327

 Inputs:
 -------
  * void

 Return:
 -------
  * bool - Whether or not the ELM327 was propperly
  initialized

 Notes:
 ------
  * Protocol - Description
  * 0        - Automatic
  * 1        - SAE J1850 PWM (41.6 kbaud)
  * 2        - SAE J1850 PWM (10.4 kbaud)
  * 4        - ISO 9141-2 (5 baud init)
  * 5        - ISO 14230-4 KWP (5 baud init)
  * 6        - ISO 14230-4 KWP (fast init)
  * 7        - ISO 15765-4 CAN (11 bit ID, 500 kbaud)
  * 8        - ISO 15765-4 CAN (29 bit ID, 500 kbaud)
  * 9        - ISO 15765-4 CAN (11 bit ID, 250 kbaud)
  * A        - ISO 15765-4 CAN (29 bit ID, 250 kbaud)
  * B        - User1 CAN (11* bit ID, 125* kbaud)
  * C        - User2 CAN (11* bit ID, 50* kbaud)

  * --> *user adjustable
*/
bool ELM327::initializeELM()
{
	char *match;

	sendCommand("AT E0"); // echo off

	delay(100);

	if (sendCommand("AT SP 0") == ELM_SUCCESS) // automatic protocol
	{
		match = strstr(payload, "OK");

		if (match != NULL)
			connected = true;
		else
			connected = false;
	}
	else
		connected = false;

	return connected;
}




/*
 void ELM327::formatQueryArray(uint16_t service, uint16_t pid)

 Description:
 ------------
  * Creates a query stack to be sent to ELM327

 Inputs:
 -------
  * uint16_t service - Service number of the queried PID
  * uint16_t pid     - PID number of the queried PID

 Return:
 -------
  * void
*/
void ELM327::formatQueryArray(uint16_t service, uint16_t pid)
{
	query[0] = ((service >> 8) & 0xFF) + '0';
	query[1] = (service & 0xFF) + '0';
	query[2] = ((pid >> 8) & 0xFF) + '0';
	query[3] = (pid & 0xFF) + '0';
	query[4] = '\n';
	query[5] = '\r';

	upper(query, 6);
}




/*
 void ELM327::upper(char string[],
                    uint8_t buflen)

 Description:
 ------------
  * Converts all elements of char array string[] to
  uppercase ascii

 Inputs:
 -------
  * uint8_t string[] - Char array
  * uint8_t buflen   - Length of char array

 Return:
 -------
  * void
*/
void ELM327::upper(char string[],
                   uint8_t buflen)
{
	for (uint8_t i = 0; i < buflen; i++)
	{
		if (string[i] > 'Z')
			string[i] -= 32;
		else if ((string[i] > '9') && (string[i] < 'A'))
			string[i] += 7;
	}
}




/*
 bool ELM327::timeout()

 Description:
 ------------
  * Determines if a time-out has occurred

 Inputs:
 -------
  * uint8_t string[] - Char array
  * uint8_t buflen   - Length of char array

 Return:
 -------
  * bool - whether or not a time-out has occurred
*/
bool ELM327::timeout()
{
	currentTime = millis();
	if ((currentTime - previousTime) >= timeout_ms)
		return true;
	return false;
}




/*
 uint8_t ELM327::ctoi(uint8_t value)

 Description:
 ------------
  * converts a decimal or hex char to an int

 Inputs:
 -------
  * uint8_t value - char to be converted

 Return:
 -------
  * uint8_t - int value of parameter "value"
*/
uint8_t ELM327::ctoi(uint8_t value)
{
	if (value >= 'A')
		return value - 'A' + 10;
	else
		return value - '0';
}




/*
 void ELM327::flushInputBuff()

 Description:
 ------------
  * Flushes input serial buffer

 Inputs:
 -------
  * void

 Return:
 -------
  * void
*/
void ELM327::flushInputBuff()
{
	while (elm_port->available())
		elm_port->read();
}




/*
 bool ELM327::queryPID(uint8_t service,
                       uint8_t PID,
                       uint8_t payloadSize,
                       float  &value)

 Description:
 ------------
  * Queries ELM327 for a specific type of vehicle telemetry data

 Inputs:
 -------
  * uint8_t service     - Service number
  * uint8_t PID         - PID number

 Return:
 -------
  * bool - Whether or not the query was submitted successfully
*/
bool ELM327::queryPID(uint16_t service,
                      uint16_t pid)
{
	if (connected)
	{
		// determine the string needed to be passed to the OBD scanner to make the query
		formatQueryArray(service, pid);

		// make the query
		status = sendCommand(query);

		return true;
	}
	
	return false;
}




/*
 int32_t ELM327::kph()

 Description:
 ------------
  *  Queries and parses received message for/returns vehicle speed data (kph)

 Inputs:
 -------
  * void

 Return:
 -------
  * int32_t - Vehicle speed in kph
*/
int32_t ELM327::kph()
{
	if (queryPID(SERVICE_01, VEHICLE_SPEED))
		return findResponse(false);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::mph()

 Description:
 ------------
  *  Queries and parses received message for/returns vehicle speed data (mph)

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Vehicle speed in mph
*/
float ELM327::mph()
{
	float mph = kph();

	if (status == ELM_SUCCESS)
		return mph * KPH_MPH_CONVERT;

	return ELM_GENERAL_ERROR;
}





/*
 float ELM327::rpm()

 Description:
 ------------
  * Queries and parses received message for/returns vehicle RMP data

 Inputs:
 -------
  * void

 Return:
 -------
  * uint32_t - Vehicle RPM
*/
float ELM327::rpm()
{
	if (queryPID(SERVICE_01, ENGINE_RPM))
		return (findResponse(true) / 4);

	return ELM_GENERAL_ERROR;
}




/*
 int8_t ELM327::sendCommand(const char *cmd)

 Description:
 ------------
  * Sends a command/query and reads/buffers the ELM327's response

 Inputs:
 -------
  * const char *cmd - Command/query to send to ELM327

 Return:
 -------
  * int8_t - Response status
*/
int8_t ELM327::sendCommand(const char *cmd)
{
	uint8_t counter = 0;

	// flush the input buffer
	flushInputBuff();

	// send the command with carriage return
	elm_port->print(cmd);
	elm_port->print('\r');

	// prime the timer
	previousTime = millis();
	currentTime  = previousTime;

	for (byte i = 0; i < PAYLOAD_LEN; i++)
		payload[i] = '\0';

	// buffer the response of the ELM327 until either the
	//end marker is read or a timeout has occurred
	while ((counter < (PAYLOAD_LEN + 1)) && !timeout())
	{
		if (elm_port->available())
		{
			payload[counter] = elm_port->read();

			if (payload[counter] == '>')
				break;
			else
				counter++;
		}
	}

	if (timeout())
		return ELM_UNABLE_TO_CONNECT;

	char *match;

	match = strstr(payload, "UNABLE TO CONNECT");
	if (match != NULL)
	{
		for (byte i = 0; i < PAYLOAD_LEN; i++)
			payload[i] = '\0';

		return ELM_UNABLE_TO_CONNECT;
	}

	match = strstr(payload, "NO DATA");
	if (match != NULL)
	{
		for (byte i = 0; i < PAYLOAD_LEN; i++)
			payload[i] = '\0';

		return ELM_NO_DATA;
	}

	return ELM_SUCCESS;
}




/*
 int8_t ELM327::findResponse(bool longResponse)

 Description:
 ------------
  * Parses the buffered ELM327's response and returns the queried data

 Inputs:
 -------
  * bool longResponse - Some responses are 2 hex digits wide and others are 4.
  If the response is expected to be 2 hex digits wide, set longResponse to false,
  else set longResponse to true

 Return:
 -------
  * int8_t - Response status
*/
int ELM327::findResponse(bool longResponse)
{
	byte maxIndex = 1;
	int dataLoc   = 0;
	int response  = 0;
	char header[5];
	char data[4];

	header[0] = '4';
	header[1] = query[1];
	header[2] = ' ';
	header[3] = query[2];
	header[4] = query[3];

	Serial.print("Received: ");
	Serial.write((const uint8_t*)payload, PAYLOAD_LEN);
	Serial.println();


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waggressive-loop-optimizations"
	for (byte i = 0; i < (PAYLOAD_LEN + 5); i++)
	{
		if (payload[i] == header[0] &&
			payload[i + 1] == header[1] &&
			payload[i + 2] == header[2] &&
			payload[i + 3] == header[3] &&
			payload[i + 4] == header[4])
			dataLoc = i + 6;
	}
#pragma GCC diagnostic pop

	if (dataLoc > 0)
	{
		data[0] = ctoi(payload[dataLoc]);
		data[1] = ctoi(payload[dataLoc + 1]);

		if (longResponse)
		{
			data[2] = ctoi(payload[dataLoc + 3]);
			data[3] = ctoi(payload[dataLoc + 4]);

			maxIndex = 3;
		}

		for (int i = maxIndex; i >= 0; i--)
			response += data[i] * pow(16, (maxIndex - i));
	}
	else
		Serial.println("Header NOT found");

	return response;
}
