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




float ELM327::queryPID(uint8_t service, uint8_t PID)
{
	float value;

	hexService = String(service, HEX);
	hexPid = String(PID, HEX);

	Serial.print(hexService);
	Serial.println(hexPid);

	return value;
}
