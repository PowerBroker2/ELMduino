#include "ELMduino.h"




bool ELM327::begin(Stream &stream)
{
	_serial = &stream;

	// wait to connect to Bluetooth
	while (!_serial);

	// try to connect twice
	if (!initializeELM())
	{
		delay(1000);
		if (!initializeELM())
			return false;
	}

	temp.reserve(25);
  
	return true;
}




/*
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
* 
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
			return false;

	while (_serial->read() != 'K')
		if (timeout())
			return false;

	return true;
}




void ELM327::formatString(uint8_t string[], uint8_t buflen)
{
	for (uint8_t i = 0; i < buflen; i++)
	{
		if (string[i] > 'Z')
		{
			string[i] = string[i] - 32;
		}
	}

	return;
}




bool ELM327::findHeader(uint8_t responseHeader[], uint8_t headerlen)
{
	for (uint8_t i = 0; i < headerlen; i++)
	{
		while (_serial->read() != responseHeader[i])
			if (timeout())
				return false;
	}

	return true;
}




bool ELM327::findPayload(uint8_t payloadSize)
{
	uint8_t c;
	uint8_t k = 0;

	// zero out the entire payload buffer array
	for (uint8_t i = 0; i < MAX_PAYLOAD_LEN; i++)
		payload[i] = 0;

	// find if the payload was found in the serial input buffer before time runs out
	while (_serial->available() <= payloadSize)
		if (timeout())
			return false;

	// read-in all the payload chars - don't include space chars
	for (uint8_t i = 0; i < payloadSize; i++)
	{
		c = _serial->read();

		// don't include spaces in the payload
		if (c != ' ')
		{
			Serial.write(c);
			payload[k] = c;
			k++;
		}	
	}
	Serial.println();

	flushInputBuff();

	return true;
}




uint32_t ELM327::findData(uint8_t payloadSize)
{
	uint32_t numShifts = 0;
	uint32_t shifter = 1;
	uint32_t data = 0;

	// i = payloadSize - 1 - (# of space chars originally spat out by OBD scanner
	//                        within the payload field)
	for (int8_t i = (payloadSize - 1 - ((payloadSize / 2)) - 1); i >= 0; i--)
	{
		for (uint8_t k = 0; k < numShifts; k++)
			shifter = shifter * 16;

		data = data + (ctoi(payload[i]) * shifter);

		numShifts++;
		shifter = 1;
	}

	return data;
}




bool ELM327::timeout()
{
	currentTime = millis();
	if ((currentTime - previousTime) >= timeout_ms)
		return true;

	return false;
}




uint8_t ELM327::ctoi(uint8_t value)
{
	if (value >= 'A')
		return value - 'A' + 10;
	else
		return value - '0';
}




void ELM327::flushInputBuff()
{
	while (_serial->available())
		_serial->read();

	return;
}




bool ELM327::queryPID(uint8_t service, uint8_t PID, uint8_t payloadSize, float &value)
{
	// find strings containing Service# in hex (with leading zeros)
	// each should only be 2 chars wide and all letters (A, B, C, D, E, F)
	// must be capitalized
	temp = String(service, HEX);
	
	if (temp.length() == 1)
	{
		hexService[0] = '0';
		hexService[1] = temp[0];
	}
	else if (temp.length() == 2)
	{
		hexService[0] = temp[0];
		hexService[1] = temp[1];
	}
	formatString(hexService, SERVICE_LEN);

	// find strings containing PID in hex (with leading zeros)
	// each should only be 2 chars wide and all letters (A, B, C, D, E, F)
	// must be capitalized
	temp = String(PID, HEX);

	if (temp.length() == 1)
	{
		hexPid[0] = '0';
		hexPid[1] = temp[0];
	}
	else if (temp.length() == 2)
	{
		hexPid[0] = temp[0];
		hexPid[1] = temp[1];
	}
	formatString(hexPid, PID_LEN);

	// find the string needed to be passed to the OBD scanner to make the query
	query[0] = hexService[0];
	query[1] = hexService[1];
	query[2] = hexPid[0];
	query[3] = hexPid[1];
	query[4] = '\n';
	query[5] = '\r';

	// find the first 6 chars expected in the OBD scanner's response
	responseHeader[0] = '4';
	responseHeader[1] = hexService[1];
	responseHeader[2] = ' ';
	responseHeader[3] = hexPid[0];
	responseHeader[4] = hexPid[1];
	responseHeader[5] = ' ';

	// make the query
	_serial->write(query, 6);

	// start timing the response
	previousTime = millis();
	currentTime = previousTime;

	// find if the header was found in the serial input buffer before time runs out
	if (!findHeader(responseHeader, HEADER_LEN))
		return false;

	// find if the payload was found in the serial input buffer before time runs out
	// and read-in all the payload chars
	if (!findPayload(payloadSize))
		return false;

	value = findData(payloadSize);

	return true;
}




bool ELM327::querySpeed(float &value)
{
	return queryPID(SERVICE_01, VEHICLE_SPEED, 2, value);
}




bool ELM327::queryRPM(float &value)
{
	return queryPID(SERVICE_01, ENGINE_RPM, 5, value);
}
