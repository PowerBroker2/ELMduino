#include "ELMduino.h"




/*
 bool ELM327::begin(Stream &stream, uint16_t payloadLen, char protocol)

 Description:
 ------------
  * Constructor for the ELM327 Class; initializes ELM327

 Inputs:
 -------
  * Stream &stream - Pointer to Serial port connected
  to ELM327
  * uint16_t payloadLen - Maximum number of bytes expected
  to be returned by the ELM327 after a query
  * char protocol - Protocol ID to specify the
  ELM327 to communicate with the ECU over

 Return:
 -------
  * bool - Whether or not the ELM327 was propperly
  initialized
*/
bool ELM327::begin(Stream &stream, uint16_t payloadLen, char protocol)
{
	elm_port = &stream;
	PAYLOAD_LEN = payloadLen;

	payload = (char*)malloc(PAYLOAD_LEN);

	// test if serial port is connected
	if (!elm_port)
		return false;

	// try to connect
	if (!initializeELM(protocol))
		return false;
  
	return true;
}




/*
 bool ELM327::initializeELM(char protocol)

 Description:
 ------------
  * Initializes ELM327

 Inputs:
 -------
  * char protocol - Protocol ID to specify the
  ELM327 to communicate with the ECU over

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
  * 3        - ISO 9141-2 (5 baud init)
  * 4        - ISO 14230-4 KWP (5 baud init)
  * 5        - ISO 14230-4 KWP (fast init)
  * 6        - ISO 15765-4 CAN (11 bit ID, 500 kbaud)
  * 7        - ISO 15765-4 CAN (29 bit ID, 500 kbaud)
  * 8        - ISO 15765-4 CAN (11 bit ID, 250 kbaud)
  * 9        - ISO 15765-4 CAN (29 bit ID, 250 kbaud)
  * A        - SAE J1939 CAN (29 bit ID, 250* kbaud)
  * B        - User1 CAN (11* bit ID, 125* kbaud)
  * C        - User2 CAN (11* bit ID, 50* kbaud)

  * --> *user adjustable
*/
bool ELM327::initializeELM(char protocol)
{
	char command[10] = { '\0' };
	connected = false;
	
	sendCommand(RESET_ALL);
	delay(100);

	sendCommand(ECHO_OFF);
	delay(100);

	sendCommand(PRINTING_SPACES_OFF);
	delay(100);
	
	sendCommand(ALLOW_LONG_MESSAGES);
	delay(100);

	sprintf(command, SET_PROTOCOL_TO_H_SAVE, protocol);

	if (sendCommand(command) == ELM_SUCCESS) // Set protocol to auto
		if (strstr(payload, "OK") != NULL)
			connected = true;

	return connected;
}




/*
 void ELM327::formatQueryArray(uint8_t service, uint16_t pid)

 Description:
 ------------
  * Creates a query stack to be sent to ELM327

 Inputs:
 -------
  * uint16_t service - Service number of the queried PID
  * uint32_t pid     - PID number of the queried PID

 Return:
 -------
  * void
*/
void ELM327::formatQueryArray(uint8_t service, uint16_t pid)
{
	query[0] = ((service >> 4) & 0xF) + '0';
	query[1] = (service & 0xF) + '0';

	// determine PID length (standard queries have 16-bit PIDs,
	// but some custom queries have PIDs with 32-bit values)
	if (pid & 0xFF00)
	{
		longQuery = true;

		query[2] = ((pid >> 12) & 0xF) + '0';
		query[3] = ((pid >> 8) & 0xF) + '0';
		query[4] = ((pid >> 4) & 0xF) + '0';
		query[5] = (pid & 0xF) + '0';

		upper(query, 6);
	}
	else
	{
		longQuery = false;

		query[2] = ((pid >> 4) & 0xF) + '0';
		query[3] = (pid & 0xF) + '0';
		query[4] = '\0';
		query[5] = '\0';

		upper(query, 4);
	}
}




/*
 void ELM327::upper(char string[], uint8_t buflen)

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
void ELM327::upper(char string[], uint8_t buflen)
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
 int8_t ELM327::nextIndex(char const *str,
                          char const *target,
                          uint8_t numOccur)

 Description:
 ------------
  * Finds and returns the first char index of
  numOccur'th instance of target in str

 Inputs:
 -------
  * char const *str    - string to search target
  within
  * char const *target - String to search for in
  str
  * uint8_t numOccur   - Which instance of target
  in str
  
 Return:
 -------
  * int8_t - First char index of numOccur'th
  instance of target in str. -1 if there is no
  numOccur'th instance of target in str
*/
int8_t ELM327::nextIndex(char const *str,
                         char const *target,
                         uint8_t numOccur=1)
{
	char const *p = str;
	char const *r = str;
	uint8_t count;

	for (count = 0; ; ++count)
	{
		p = strstr(p, target);

		if (count == (numOccur - 1))
			break;

		if (!p)
			break;

		p++;
	}

	if (!p)
		return -1;

	return p - r;
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
                       uint16_t PID,
                       float  &value)

 Description:
 ------------
  * Queries ELM327 for a specific type of vehicle telemetry data

 Inputs:
 -------
  * uint8_t service - Service number
  * uint8_t PID     - PID number

 Return:
 -------
  * bool - Whether or not the query was submitted successfully
*/
bool ELM327::queryPID(uint8_t service,
                      uint16_t pid)
{
	if (connected)
	{
		formatQueryArray(service, pid);
		sendCommand(query);

		return true;
	}
	
	return false;
}




/*
 bool ELM327::queryPID(char queryStr[])

 Description:
 ------------
  * Queries ELM327 for a specific type of vehicle telemetry data

 Inputs:
 -------
  * char queryStr[] - Query string (service and PID)

 Return:
 -------
  * bool - Whether or not the query was submitted successfully
*/
bool ELM327::queryPID(char queryStr[])
{
	if (connected)
	{
		if (strlen(queryStr) <= 4)
			longQuery = false;
		else
			longQuery = true;

		sendCommand(queryStr);

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
  * int16_t - Vehicle speed in kph
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
	connected = false;

	for (byte i = 0; i < PAYLOAD_LEN; i++)
		payload[i] = '\0';

	// reset input buffer and number of received bytes
	recBytes = 0;
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
			char recChar = elm_port->read();

			if (recChar == '>')
				break;
			else if (isspace(recChar))
				continue;
			
			payload[counter] = recChar;
			counter++;
		}
	}

	if (timeout())
	{
		status = ELM_TIMEOUT;
		return status;
	}
	
	if (nextIndex(payload, "UNABLETOCONNECT") >= 0)
	{
		status = ELM_UNABLE_TO_CONNECT;
		return status;
	}

	connected = true;

	if (nextIndex(payload, "NODATA") >= 0)
	{
		status = ELM_NO_DATA;
		return status;
	}

	if (nextIndex(payload, "STOPPED") >= 0)
	{
		status = ELM_STOPPED;
		return status;
	}

	if (nextIndex(payload, "ERROR") >= 0)
	{
		status = ELM_GENERAL_ERROR;
		return status;
	}

	// keep track of how many bytes were received in
	// the ELM327's response (not counting the
	// end-marker '>') if a valid response is found
	recBytes = counter;

	status = ELM_SUCCESS;
	return status;
}




/*
 uint32_t ELM327::findResponse()

 Description:
 ------------
  * Parses the buffered ELM327's response and returns the queried data

 Inputs:
 -------
  * void

 Return:
 -------
  * uint32_t - Query response value
*/
uint32_t ELM327::findResponse()
{
	uint8_t firstDatum = 0;
	uint8_t payBytes = 0;
	char header[7] = { '\0' };

	if (longQuery)
	{
		header[0] = query[0] + 4;
		header[1] = query[1];
		header[2] = query[2];
		header[3] = query[3];
		header[4] = query[4];
		header[5] = query[5];
	}
	else
	{
		header[0] = query[0] + 4;
		header[1] = query[1];
		header[2] = query[2];
		header[3] = query[3];
	}

	int8_t firstHeadIndex  = nextIndex(payload, header);
	int8_t secondHeadIndex = nextIndex(payload, header, 2);

	if (firstHeadIndex >= 0)
	{
		if (longQuery)
			firstDatum = firstHeadIndex + 6;
		else
			firstDatum = firstHeadIndex + 4;

		// Some ELM327s (such as my own) respond with two
		// "responses" per query. "payBytes" represents the
		// correct number of bytes returned by the ELM327
		// regardless of how many "responses" were returned
		if (secondHeadIndex >= 0)
			payBytes = secondHeadIndex - firstDatum;
		else
			payBytes = recBytes - firstDatum;

		// Some PID queries return 4 hex digit values - the
		// rest return 2 hex digit values
		if (payBytes >= 4)
			return (ctoi(payload[firstDatum]) << 12) | (ctoi(payload[firstDatum + 1]) << 8) | (ctoi(payload[firstDatum + 2]) << 4) | ctoi(payload[firstDatum + 3]);
		else
			return (ctoi(payload[firstDatum]) << 4) | ctoi(payload[firstDatum + 1]);
	}

	return 0;
}
