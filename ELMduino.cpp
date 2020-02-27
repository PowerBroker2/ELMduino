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

	// test if serial port is connected
	if (!elm_port)
		return false;

	// try to connect
	if (!initializeELM())
		return false;
  
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
	connected = false;

	sendCommand(ECHO_OFF);
	delay(100);

	sendCommand(PRINTING_SPACES_OFF);
	delay(100);

	if (sendCommand("AT SP 0") == ELM_SUCCESS) // Set protocol to auto
	{
		match = strstr(payload, "OK");

		if (match != NULL)
			connected = true;
	}

	return connected;
}




/*
 void ELM327::formatQueryArray(uint16_t service, uint64_t pid)

 Description:
 ------------
  * Creates a query stack to be sent to ELM327

 Inputs:
 -------
  * uint16_t service - Service number of the queried PID
  * uint64_t pid     - PID number of the queried PID

 Return:
 -------
  * void
*/
void ELM327::formatQueryArray(uint16_t service, uint32_t pid)
{
	query[0] = ((service >> 8) & 0xFF) + '0';
	query[1] = (service & 0xFF) + '0';

	// determine PID length (standard queries have 16-bit PIDs,
	// but some custom queries have PIDs with 32-bit values)
	if (pid & 0xFF00)
	{
		longQuery = true;

		query[2] = ((pid >> 24) & 0xFF) + '0';
		query[3] = ((pid >> 16) & 0xFF) + '0';
		query[4] = ((pid >> 8) & 0xFF) + '0';
		query[5] = (pid & 0xFF) + '0';

		upper(query, 6);
	}
	else
	{
		longQuery = false;

		query[2] = ((pid >> 8) & 0xFF) + '0';
		query[3] = (pid & 0xFF) + '0';

		upper(query, 4);
	}
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
		formatQueryArray(service, pid);
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
		return findResponse();

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
  * float - Vehicle RPM
*/
float ELM327::rpm()
{
	if (queryPID(SERVICE_01, ENGINE_RPM))
		return (findResponse() / 4.0);

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

	for (byte i = 0; i < PAYLOAD_LEN; i++)
		payload[i] = '\0';

	flushInputBuff();

	elm_port->print(cmd);
	elm_port->print('\r');

	// prime the timeout timer
	previousTime = millis();
	currentTime  = previousTime;

	// buffer the response of the ELM327 until either the
	// end marker is read or a timeout has occurred
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

	// keep track of how many bytes were received in
	// the ELM327's response (not counting the
	// end-marker '>')
	recBytes = counter;

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

	match = strstr(payload, "STOPPED");
	if (match != NULL)
	{
		for (byte i = 0; i < PAYLOAD_LEN; i++)
			payload[i] = '\0';

		return ELM_NO_DATA;
	}

	return ELM_SUCCESS;
}




/*
 uint16_t ELM327::findResponse()

 Description:
 ------------
  * Parses the buffered ELM327's response and returns the queried data

 Inputs:
 -------
  * void

 Return:
 -------
  * uint16_t - Response status
*/
uint16_t ELM327::findResponse()
{
	uint16_t dataLoc  = 0;
	uint16_t response = 0;
	uint8_t A = 0;
	uint8_t B = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waggressive-loop-optimizations"
	if (longQuery)
	{
		for (byte i = 0; i < (recBytes + 6); i++)
		{
			if (payload[i] == (query[0] + 4) &&
				payload[i + 1] == query[1] &&
				payload[i + 2] == query[2] &&
				payload[i + 3] == query[3] &&
				payload[i + 4] == query[4] &&
				payload[i + 5] == query[5])
				dataLoc = i + 7;
		}
	}
	else
	{
		for (byte i = 0; i < (recBytes + 4); i++)
		{
			if (payload[i] == (query[0] + 4) &&
				payload[i + 1] == query[1] &&
				payload[i + 2] == query[2] &&
				payload[i + 3] == query[3])
				dataLoc = i + 5;
		}
	}
#pragma GCC diagnostic pop

	if (dataLoc > 0)
	{
		if (recBytes >= 4)
		{
			B = (ctoi(payload[dataLoc]) * 16) + ctoi(payload[dataLoc + 1]);
			A = (ctoi(payload[dataLoc + 2]) * 16) + ctoi(payload[dataLoc + 3]);
		}
		else
		{
			B = 0;
			A = (ctoi(payload[dataLoc]) * 16) + ctoi(payload[dataLoc + 1]);
		}

		response = (B << 8) | A;
	}

	return response;
}
