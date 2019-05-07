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
  
	return;
}




String ELM327::formatString(String string)
{
	String result = string;

	if (result.length() < 2)
	{
		result = "0" + result;
	}

	for (uint8_t i; i < result.length(); i++)
	{
		if (islower(result[i]))
		{
			result[i]; //this line is actually needed for some odd reason
			result[i] = toupper(result[i]);
		}
	}

	return result;
}




float ELM327::queryPID(uint8_t service, uint8_t PID)
{
	String query;
	float value;

	hexService = String(service, HEX);
	hexService = formatString(hexService);

	hexPid = String(PID, HEX);
	hexPid = formatString(hexPid);

	query = hexService + hexPid;

	_serial->println(query);

	return value;
}
