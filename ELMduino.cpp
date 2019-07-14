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
	_serial = &stream;

	while (!_serial);

	// try to connect twice
	if (!initializeELM())
	{
		delay(1000);
		if (!initializeELM())
			return false;
	}
  
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
	flushInputBuff();

	// initialize scanner
	_serial->println("AT SP 0");

	// start timing the response
	previousTime = millis();
	currentTime = previousTime;

	while (_serial->read() != 'O')
		if (timeout())
		{
			connected = false;
			return false;
		}

	while (_serial->read() != 'K')
		if (timeout())
		{
			connected = false;
			return false;
		}

	connected = true;
	return true;
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
 void ELM327::formatHeaderArray()

 Description:
 ------------
  * Creates a sample response header as expected from
  the ELM327 based off of the PID queried 

 Inputs:
 -------
  * void

 Return:
 -------
  * void
*/
void ELM327::formatHeaderArray()
{
	responseHeader[0] = '4';
	responseHeader[1] = query[1];
	responseHeader[2] = query[2];
	responseHeader[3] = query[3];
}




/*
 void ELM327::upper(uint8_t string[],
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
void ELM327::upper(uint8_t string[],
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
  * void

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
  * converts a char to an int

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
	while (_serial->available())
		_serial->read();
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

		// determine the first 6 chars expected in the OBD scanner's response
		formatHeaderArray();

		// flush the input buffer
		flushInputBuff();

		// make the query
		_serial->write(query, 6);

		// start timing the response
		previousTime = millis();
		currentTime = previousTime;

		return true;
	}
	
	return false;
}




/*
 bool ELM327::querySpeed_kph()

 Description:
 ------------
  * Queries ELM327 for vehicle speed in kph

 Inputs:
 -------
  * void

 Return:
 -------
  * bool - Whether or not the query was submitted successfully
*/
bool ELM327::querySpeed_kph()
{
	return queryPID(SERVICE_01, VEHICLE_SPEED);
}




/*
 bool ELM327::queryRPM()

 Description:
 ------------
  * Queries ELM327 for vehicle RPM

 Inputs:
 -------
  * void

 Return:
 -------
  * bool - Whether or not the query was submitted successfully
*/
bool ELM327::queryRPM()
{
	return queryPID(SERVICE_01, ENGINE_RPM);
}




/*
 bool ELM327::available()

 Description:
 ------------
  * Parses incoming serial data and determines if a full (queried)
  message has been successfully received

 Inputs:
 -------
  * void

 Return:
 -------
  * bool - Whether or not the queried message has been received
*/
bool ELM327::available()
{
	while (_serial->available())
	{
		char recChar = _serial->read();

		if (recChar == '>')
		{
			messageComplete = true;
			messageIndex = 0;

			if (headerFound)
			{
				headerFound = false;
				return true;
			}
			else
				return false;
		}
		else if (!headerFound)
		{
			if (messageIndex == 4)
			{
				headerFound = true;
				if (recChar != ' ')
				{
					buff[messageIndex] = recChar;
					messageIndex++;
				}
			}
			else if (recChar == responseHeader[messageIndex])
			{
				buff[messageIndex] = recChar;
				messageIndex++;
			}
		}
		else if ((messageIndex < ELM_BUFF_LEN) && (recChar != ' '))
		{
			buff[messageIndex] = recChar;
			messageIndex++;
		}
	}

	return false;
}
