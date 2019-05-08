#include "ELMduino.h"




void ELM327::begin(Stream &stream)
{
	_serial = &stream;

	//wait to connect to Bluetooth
	while (!_serial);

	//initialize scanner
	_serial->println("AT SP 0");

	//flush the input buffer
	while (_serial->available())
		_serial->read();

	temp.reserve(25);
  
	return;
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
		{
			if (timeout())
				return false;
		}
	}

	return true;
}




bool ELM327::findPayload(uint8_t payloadSize)
{
	// find if the payload was found in the serial input buffer before time runs out
	while (_serial->available() <= payloadSize)
	{
		if (timeout())
			return false;
	}

	// read-in all the payload chars
	for (uint8_t i = 0; i < payloadSize; i++)
	{
		payload[i] = _serial->read();
	}

	return true;
}




bool ELM327::timeout()
{
	currentTime = millis();
	if ((currentTime - previousTime) >= timeout_ms)
		return true;

	return false;
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

	// make the query
	_serial->write(query, 6);

	// start timing the response
	previousTime = millis();
	currentTime = previousTime;

	// find the first 6 chars expected in the OBD scanner's response
	responseHeader[0] = '4';
	responseHeader[1] = hexService[1];
	responseHeader[2] = " "; 
	responseHeader[3] = hexPid[0];
	responseHeader[4] = hexPid[1];
	responseHeader[5] = " ";

	// find if the header was found in the serial input buffer before time runs out
	if (!findHeader(responseHeader, HEADER_LEN))
		return false;

	// find if the payload was found in the serial input buffer before time runs out
	// and read-in all the payload chars
	if (!findPayload(payloadSize))
		return false;

	return true;
}
