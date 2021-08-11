#include "ELMduino.h"




/*
 bool ELM327::begin(Stream &stream, const bool& debug, const uint16_t& timeout, const char& protocol, const uint16_t& payloadLen, const byte& dataTimeout)

 Description:
 ------------
  * Constructor for the ELM327 Class; initializes ELM327

 Inputs:
 -------
  * Stream &stream      - Reference to Serial port connected to ELM327
  * bool debug          - Specify whether or not to print debug statements to "Serial"
  * uint16_t timeout    - Time in ms to wait for a query response
  * char protocol       - Protocol ID to specify the ELM327 to communicate with the ECU over
  * uint16_t payloadLen - Maximum number of bytes expected to be returned by the ELM327 after a query
  * byte dataTimeout    - Number of ms to wait after receiving data before the ELM327 will
						  return the data - see https://www.elmelectronics.com/help/obd/tips/#UnderstandingOBD
 Return:
 -------
  * bool - Whether or not the ELM327 was propperly
  initialized
*/
bool ELM327::begin(Stream &stream, const bool& debug, const uint16_t& timeout, const char& protocol, const uint16_t& payloadLen, const byte& dataTimeout)
{
	elm_port    = &stream;
	PAYLOAD_LEN = payloadLen;
	debugMode   = debug;
	timeout_ms  = timeout;

	payload = (char*)malloc(PAYLOAD_LEN + 1); // allow for terminating '\0'

	// test if serial port is connected
	if (!elm_port)
		return false;

	// try to connect
	if (!initializeELM(protocol, dataTimeout))
		return false;
  
	return true;
}




/*
 bool ELM327::initializeELM(const char& protocol, const byte& dataTimeout)

 Description:
 ------------
  * Initializes ELM327

 Inputs:
 -------
  * char protocol    - Protocol ID to specify the ELM327 to communicate with the ECU over
  * byte dataTimeout - Number of ms to wait after receiving data before the ELM327 will
                       return the data - see https://www.elmelectronics.com/help/obd/tips/#UnderstandingOBD

 Return:
 -------
  * bool - Whether or not the ELM327 was propperly initialized

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
bool ELM327::initializeELM(const char& protocol, const byte& dataTimeout)
{
	char command[10] = { '\0' };
	connected = false;

	sendCommand(SET_ALL_TO_DEFAULTS);
	delay(100);

	sendCommand(RESET_ALL);
	delay(100);

	sendCommand(ECHO_OFF);
	delay(100);

	sendCommand(PRINTING_SPACES_OFF);
	delay(100);

	sendCommand(ALLOW_LONG_MESSAGES);
	delay(100);

	// Set data timeout
	sprintf(command, SET_TIMEOUT_TO_H_X_4MS, dataTimeout / 4);
	sendCommand(command);
	delay(100);

	// Set protocol
	sprintf(command, TRY_PROT_H_AUTO_SEARCH, protocol);

	if (sendCommand(command) == ELM_SUCCESS)
	{
		if (strstr(payload, "OK") != NULL)
		{
			connected = true;
			return connected;
		}
	}

	if (debugMode)
	{
		Serial.print(F("Setting protocol via "));
		Serial.print(TRY_PROT_H_AUTO_SEARCH);
		Serial.print(F(" did not work - trying via "));
		Serial.println(SET_PROTOCOL_TO_H_SAVE);
	}
	
	// Set protocol and save
	sprintf(command, SET_PROTOCOL_TO_H_SAVE, protocol);

	if (sendCommand(command) == ELM_SUCCESS)
		if (strstr(payload, "OK") != NULL)
			connected = true;

	if (debugMode)
	{
		Serial.print(F("Setting protocol via "));
		Serial.print(SET_PROTOCOL_TO_H_SAVE);
		Serial.println(F(" did not work"));
	}

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
	if (debugMode)
	{
		Serial.print(F("Service: "));
		Serial.println(service);
		Serial.print(F("PID: "));
		Serial.println(pid);
	}

	query[0] = ((service >> 4) & 0xF) + '0';
	query[1] = (service & 0xF) + '0';

	// determine PID length (standard queries have 16-bit PIDs,
	// but some custom queries have PIDs with 32-bit values)
	if (pid & 0xFF00)
	{
		if (debugMode)
			Serial.println(F("Long query detected"));

		longQuery = true;

		query[2] = ((pid >> 12) & 0xF) + '0';
		query[3] = ((pid >> 8) & 0xF) + '0';
		query[4] = ((pid >> 4) & 0xF) + '0';
		query[5] = (pid & 0xF) + '0';

		upper(query, 6);
	}
	else
	{
		if (debugMode)
			Serial.println(F("Normal length query detected"));

		longQuery = false;

		query[2] = ((pid >> 4) & 0xF) + '0';
		query[3] = (pid & 0xF) + '0';
		query[4] = '\0';
		query[5] = '\0';

		upper(query, 4);
	}

	if (debugMode)
	{
		Serial.print(F("Query string: "));
		Serial.println(query);
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
  * char const *str    - string to search target within
  * char const *target - String to search for in str
  * uint8_t numOccur   - Which instance of target in str
  
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




float ELM327::conditionResponse(const uint64_t& response, const uint8_t& numExpectedBytes, const float& scaleFactor, const float& bias)
{
	return ((response >> (((numPayChars / 2) - numExpectedBytes) * 8)) * scaleFactor) + bias;
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
	if (debugMode)
		Serial.println(F("Clearing input serial buffer"));

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
	formatQueryArray(service, pid);
	sendCommand(query);
	
	return connected;
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
	if (strlen(queryStr) <= 4)
		longQuery = false;
	else
		longQuery = true;

	sendCommand(queryStr);
	
	return connected;
}




/*
 uint32_t ELM327::supportedPIDs_1_20()

 Description:
 ------------
  * Determine which of PIDs 0x1 through 0x20 are supported (bit encoded)

 Inputs:
 -------
  * void

 Return:
 -------
  * uint32_t - Bit encoded booleans of supported PIDs 0x1-0x20
*/
uint32_t ELM327::supportedPIDs_1_20()
{
	if (queryPID(SERVICE_01, SUPPORTED_PIDS_1_20))
		return conditionResponse(findResponse(), 4);

	return ELM_GENERAL_ERROR;
}




/*
 uint32_t ELM327::monitorStatus()

 Description:
 ------------
  * Monitor status since DTCs cleared (Includes malfunction indicator
  lamp (MIL) status and number of DTCs). See https://en.wikipedia.org/wiki/OBD-II_PIDs#Service_01_PID_01
  for more info

 Inputs:
 -------
  * void

 Return:
 -------
  * uint32_t - Bit encoded status (https://en.wikipedia.org/wiki/OBD-II_PIDs#Service_01_PID_01)
*/
uint32_t ELM327::monitorStatus()
{
	if (queryPID(SERVICE_01, MONITOR_STATUS_SINCE_DTC_CLEARED))
		return conditionResponse(findResponse(), 4);

	return ELM_GENERAL_ERROR;
}




/*
 uint16_t ELM327::freezeDTC()

 Description:
 ------------
  * Freeze DTC - see https://www.samarins.com/diagnose/freeze-frame.html for more info

 Inputs:
 -------
  * void

 Return:
 -------
  * uint16_t - Various vehicle information (https://www.samarins.com/diagnose/freeze-frame.html)
*/
uint16_t ELM327::freezeDTC()
{
	if (queryPID(SERVICE_01, FREEZE_DTC))
		return conditionResponse(findResponse(), 2);

	return ELM_GENERAL_ERROR;
}




/*
 uint16_t ELM327::fuelSystemStatus()

 Description:
 ------------
  * Freeze DTC - see https://en.wikipedia.org/wiki/OBD-II_PIDs#Service_01_PID_03 for more info

 Inputs:
 -------
  * void

 Return:
 -------
  * uint16_t - Bit encoded status (https://en.wikipedia.org/wiki/OBD-II_PIDs#Service_01_PID_03)
*/
uint16_t ELM327::fuelSystemStatus()
{
	if (queryPID(SERVICE_01, FUEL_SYSTEM_STATUS))
		return conditionResponse(findResponse(), 2);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::engineLoad()

 Description:
 ------------
  * Find the current engine load in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Engine load %
*/
float ELM327::engineLoad()
{
	if (queryPID(SERVICE_01, ENGINE_LOAD))
		return conditionResponse(findResponse(), 1, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::engineCoolantTemp()

 Description:
 ------------
  * Find the current engine coolant temp in C

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Engine load %
*/
float ELM327::engineCoolantTemp()
{
	if (queryPID(SERVICE_01, ENGINE_COOLANT_TEMP))
		return conditionResponse(findResponse(), 1, 1, -40.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::shortTermFuelTrimBank_1()

 Description:
 ------------
  * Find fuel trim %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Fuel trim %
*/
float ELM327::shortTermFuelTrimBank_1()
{
	if (queryPID(SERVICE_01, SHORT_TERM_FUEL_TRIM_BANK_1))
		return conditionResponse(findResponse(), 1, 100.0 / 128.0, -100.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::longTermFuelTrimBank_1()

 Description:
 ------------
  * Find fuel trim %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Fuel trim %
*/
float ELM327::longTermFuelTrimBank_1()
{
	if (queryPID(SERVICE_01, LONG_TERM_FUEL_TRIM_BANK_1))
		return conditionResponse(findResponse(), 1, 100.0 / 128.0, -100.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::shortTermFuelTrimBank_2()

 Description:
 ------------
  * Find fuel trim %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Fuel trim %
*/
float ELM327::shortTermFuelTrimBank_2()
{
	if (queryPID(SERVICE_01, SHORT_TERM_FUEL_TRIM_BANK_2))
		return conditionResponse(findResponse(), 1, 100.0 / 128.0, -100.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::longTermFuelTrimBank_2()

 Description:
 ------------
  * Find fuel trim %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Fuel trim %
*/
float ELM327::longTermFuelTrimBank_2()
{
	if (queryPID(SERVICE_01, LONG_TERM_FUEL_TRIM_BANK_2))
		return conditionResponse(findResponse(), 1, 100.0 / 128.0, -100.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::fuelPressure()

 Description:
 ------------
  * Find fuel pressure in kPa

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Fuel pressure in kPa
*/
float ELM327::fuelPressure()
{
	if (queryPID(SERVICE_01, FUEL_PRESSURE))
		return conditionResponse(findResponse(), 1, 3.0);

	return ELM_GENERAL_ERROR;
}




/*
 uint8_t ELM327::manifoldPressure()

 Description:
 ------------
  * Find intake manifold absolute pressure in kPa

 Inputs:
 -------
  * void

 Return:
 -------
  * uint8_t - Intake manifold absolute pressure in kPa
*/
uint8_t ELM327::manifoldPressure()
{
	if (queryPID(SERVICE_01, INTAKE_MANIFOLD_ABS_PRESSURE))
		return conditionResponse(findResponse(), 1);

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
		return conditionResponse(findResponse(), 2, 1.0 / 4.0);

	return ELM_GENERAL_ERROR;
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
		return conditionResponse(findResponse(), 1);

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
 float ELM327::timingAdvance()

 Description:
 ------------
  *  Find timing advance in degrees before Top Dead Center (TDC)

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Timing advance in degrees before Top Dead Center (TDC)
*/
float ELM327::timingAdvance()
{
	if (queryPID(SERVICE_01, TIMING_ADVANCE))
		return conditionResponse(findResponse(), 1, 1.0 / 2.0, -64.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::intakeAirTemp()

 Description:
 ------------
  *  Find intake air temperature in C

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Intake air temperature in C
*/
float ELM327::intakeAirTemp()
{
	if (queryPID(SERVICE_01, INTAKE_AIR_TEMP))
		return conditionResponse(findResponse(), 1, 1, -40.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::mafRate()

 Description:
 ------------
  *  Find mass air flow sensor (MAF) air flow rate rate in g/s

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Mass air flow sensor (MAF) air flow rate rate in g/s
*/
float ELM327::mafRate()
{
	if (queryPID(SERVICE_01, MAF_FLOW_RATE))
		return conditionResponse(findResponse(), 2, 1.0 / 100.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::throttle()

 Description:
 ------------
  *  Find throttle position in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Throttle position in %
*/
float ELM327::throttle()
{
	if (queryPID(SERVICE_01, THROTTLE_POSITION))
		return conditionResponse(findResponse(), 1, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 uint8_t ELM327::commandedSecAirStatus()

 Description:
 ------------
  *  Find commanded secondary air status

 Inputs:
 -------
  * void

 Return:
 -------
  * uint8_t - Bit encoded status (https://en.wikipedia.org/wiki/OBD-II_PIDs#Service_01_PID_12)
*/
uint8_t ELM327::commandedSecAirStatus()
{
	if (queryPID(SERVICE_01, COMMANDED_SECONDARY_AIR_STATUS))
		return conditionResponse(findResponse(), 1);

	return ELM_GENERAL_ERROR;
}




/*
 uint8_t ELM327::oxygenSensorsPresent_2banks()

 Description:
 ------------
  *  Find which oxygen sensors are present ([A0..A3] == Bank 1, Sensors 1-4. [A4..A7] == Bank 2...)

 Inputs:
 -------
  * void

 Return:
 -------
  * uint8_t - Bit encoded
*/
uint8_t ELM327::oxygenSensorsPresent_2banks()
{
	if (queryPID(SERVICE_01, OXYGEN_SENSORS_PRESENT_2_BANKS))
		return conditionResponse(findResponse(), 1);

	return ELM_GENERAL_ERROR;
}




/*
 uint8_t ELM327::obdStandards()

 Description:
 ------------
  *  Find the OBD standards this vehicle conforms to (https://en.wikipedia.org/wiki/OBD-II_PIDs#Service_01_PID_1C)

 Inputs:
 -------
  * void

 Return:
 -------
  * uint8_t - Bit encoded (https://en.wikipedia.org/wiki/OBD-II_PIDs#Service_01_PID_1C)
*/
uint8_t ELM327::obdStandards()
{
	if (queryPID(SERVICE_01, OBD_STANDARDS))
		return conditionResponse(findResponse(), 1);

	return ELM_GENERAL_ERROR;
}




/*
 uint8_t ELM327::oxygenSensorsPresent_4banks()

 Description:
 ------------
  *  Find which oxygen sensors are present (Similar to PID 13, but [A0..A7] == [B1S1, B1S2, B2S1, B2S2, B3S1, B3S2, B4S1, B4S2])

 Inputs:
 -------
  * void

 Return:
 -------
  * uint8_t - Bit encoded
*/
uint8_t ELM327::oxygenSensorsPresent_4banks()
{
	if (queryPID(SERVICE_01, OXYGEN_SENSORS_PRESENT_4_BANKS))
		return conditionResponse(findResponse(), 1);

	return ELM_GENERAL_ERROR;
}




/*
 bool ELM327::auxInputStatus()

 Description:
 ------------
  *  Find Power Take Off (PTO) status
  
 Inputs:
 -------
  * void

 Return:
 -------
  * bool - Power Take Off (PTO) status
*/
bool ELM327::auxInputStatus()
{
	if (queryPID(SERVICE_01, AUX_INPUT_STATUS))
		return conditionResponse(findResponse(), 1);

	return ELM_GENERAL_ERROR;
}




/*
 uint16_t ELM327::runTime()

 Description:
 ------------
  *  Find run time since engine start in s

 Inputs:
 -------
  * void

 Return:
 -------
  * uint16_t - Run time since engine start in s
*/
uint16_t ELM327::runTime()
{
	if (queryPID(SERVICE_01, RUN_TIME_SINCE_ENGINE_START))
		return conditionResponse(findResponse(), 2);

	return ELM_GENERAL_ERROR;
}




/*
 uint32_t ELM327::supportedPIDs_21_40()

 Description:
 ------------
  * Determine which of PIDs 0x1 through 0x20 are supported (bit encoded)

 Inputs:
 -------
  * void

 Return:
 -------
  * uint32_t - Bit encoded booleans of supported PIDs 0x21-0x20
*/
uint32_t ELM327::supportedPIDs_21_40()
{
	if (queryPID(SERVICE_01, SUPPORTED_PIDS_21_40))
		return conditionResponse(findResponse(), 4);

	return ELM_GENERAL_ERROR;
}




/*
 uint16_t ELM327::distTravelWithMIL()

 Description:
 ------------
  *  Find distance traveled with malfunction indicator lamp (MIL) on in km

 Inputs:
 -------
  * void

 Return:
 -------
  * uint16_t - Distance traveled with malfunction indicator lamp (MIL) on in km
*/
uint16_t ELM327::distTravelWithMIL()
{
	if (queryPID(SERVICE_01, DISTANCE_TRAVELED_WITH_MIL_ON))
		return conditionResponse(findResponse(), 2);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::fuelRailPressure()

 Description:
 ------------
  *  Find fuel Rail Pressure (relative to manifold vacuum) in kPa

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Fuel Rail Pressure (relative to manifold vacuum) in kPa
*/
float ELM327::fuelRailPressure()
{
	if (queryPID(SERVICE_01, FUEL_RAIL_PRESSURE))
		return conditionResponse(findResponse(), 2, 0.079);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::fuelRailGuagePressure()

 Description:
 ------------
  *  Find fuel Rail Gauge Pressure (diesel, or gasoline direct injection) in kPa

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Fuel Rail Gauge Pressure (diesel, or gasoline direct injection) in kPa
*/
float ELM327::fuelRailGuagePressure()
{
	if (queryPID(SERVICE_01, FUEL_RAIL_GUAGE_PRESSURE))
		return conditionResponse(findResponse(), 2, 10.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::commandedEGR()

 Description:
 ------------
  *  Find commanded Exhaust Gas Recirculation (EGR) in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Commanded Exhaust Gas Recirculation (EGR) in %
*/
float ELM327::commandedEGR()
{
	if (queryPID(SERVICE_01, COMMANDED_EGR))
		return conditionResponse(findResponse(), 1, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::egrError()

 Description:
 ------------
  *  Find Exhaust Gas Recirculation (EGR) error in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Exhaust Gas Recirculation (EGR) error in %
*/
float ELM327::egrError()
{
	if (queryPID(SERVICE_01, EGR_ERROR))
		return conditionResponse(findResponse(), 1, 100.0 / 128.0, -100);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::commandedEvapPurge()

 Description:
 ------------
  *  Find commanded evaporative purge in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Commanded evaporative purge in %
*/
float ELM327::commandedEvapPurge()
{
	if (queryPID(SERVICE_01, COMMANDED_EVAPORATIVE_PURGE))
		return conditionResponse(findResponse(), 1, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::fuelLevel()

 Description:
 ------------
  *  Find fuel tank level input in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Fuel tank level input in %
*/
float ELM327::fuelLevel()
{
	if (queryPID(SERVICE_01, FUEL_TANK_LEVEL_INPUT))
		return conditionResponse(findResponse(), 1, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 uint8_t ELM327::warmUpsSinceCodesCleared()

 Description:
 ------------
  *  Find num warm-ups since codes cleared

 Inputs:
 -------
  * void

 Return:
 -------
  * uint8_t - Num warm-ups since codes cleared
*/
uint8_t ELM327::warmUpsSinceCodesCleared()
{
	if (queryPID(SERVICE_01, WARM_UPS_SINCE_CODES_CLEARED))
		return conditionResponse(findResponse(), 1);

	return ELM_GENERAL_ERROR;
}




/*
 uint16_t ELM327::distSinceCodesCleared()

 Description:
 ------------
  *  Find distance traveled since codes cleared in km

 Inputs:
 -------
  * void

 Return:
 -------
  * uint16_t - Distance traveled since codes cleared in km
*/
uint16_t ELM327::distSinceCodesCleared()
{
	if (queryPID(SERVICE_01, DIST_TRAV_SINCE_CODES_CLEARED))
		return conditionResponse(findResponse(), 2);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::evapSysVapPressure()

 Description:
 ------------
  *  Find evap. system vapor pressure in Pa

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Evap. system vapor pressure in Pa
*/
float ELM327::evapSysVapPressure()
{
	if (queryPID(SERVICE_01, EVAP_SYSTEM_VAPOR_PRESSURE))
		return conditionResponse(findResponse(), 2, 1.0 / 4.0);

	return ELM_GENERAL_ERROR;
}




/*
 uint8_t ELM327::absBaroPressure()

 Description:
 ------------
  *  Find absolute barometric pressure in kPa

 Inputs:
 -------
  * void

 Return:
 -------
  * uint8_t - Absolute barometric pressure in kPa
*/
uint8_t ELM327::absBaroPressure()
{
	if (queryPID(SERVICE_01, ABS_BAROMETRIC_PRESSURE))
		return conditionResponse(findResponse(), 1);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::catTempB1S1()

 Description:
 ------------
  *  Find catalyst temperature in C

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Catalyst temperature in C
*/
float ELM327::catTempB1S1()
{
	if (queryPID(SERVICE_01, CATALYST_TEMP_BANK_1_SENSOR_1))
		return conditionResponse(findResponse(), 2, 1.0 / 10.0, -40.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::catTempB2S1()

 Description:
 ------------
  *  Find catalyst temperature in C

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Catalyst temperature in C
*/
float ELM327::catTempB2S1()
{
	if (queryPID(SERVICE_01, CATALYST_TEMP_BANK_2_SENSOR_1))
		return conditionResponse(findResponse(), 2, 1.0 / 10.0, -40.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::catTempB1S2()

 Description:
 ------------
  *  Find catalyst temperature in C

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Catalyst temperature in C
*/
float ELM327::catTempB1S2()
{
	if (queryPID(SERVICE_01, CATALYST_TEMP_BANK_1_SENSOR_2))
		return conditionResponse(findResponse(), 2, 1.0 / 10.0, -40.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::catTempB2S2()

 Description:
 ------------
  *  Find catalyst temperature in C

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Catalyst temperature in C
*/
float ELM327::catTempB2S2()
{
	if (queryPID(SERVICE_01, CATALYST_TEMP_BANK_2_SENSOR_2))
		return conditionResponse(findResponse(), 2, 1.0 / 10.0, -40.0);

	return ELM_GENERAL_ERROR;
}




/*
 uint32_t ELM327::supportedPIDs_41_60()

 Description:
 ------------
  * Determine which of PIDs 0x41 through 0x60 are supported (bit encoded)

 Inputs:
 -------
  * void

 Return:
 -------
  * uint32_t - Bit encoded booleans of supported PIDs 0x41-0x60
*/
uint32_t ELM327::supportedPIDs_41_60()
{
	if (queryPID(SERVICE_01, SUPPORTED_PIDS_41_60))
		return conditionResponse(findResponse(), 4);

	return ELM_GENERAL_ERROR;
}




/*
 uint32_t ELM327::monitorDriveCycleStatus()

 Description:
 ------------
  *  Find status this drive cycle (https://en.wikipedia.org/wiki/OBD-II_PIDs#Service_01_PID_41)

 Inputs:
 -------
  * void

 Return:
 -------
  * uint32_t - Bit encoded status (https://en.wikipedia.org/wiki/OBD-II_PIDs#Service_01_PID_41)
*/
uint32_t ELM327::monitorDriveCycleStatus()
{
	if (queryPID(SERVICE_01, MONITOR_STATUS_THIS_DRIVE_CYCLE))
		return conditionResponse(findResponse(), 4);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::ctrlModVoltage()

 Description:
 ------------
  *  Find control module voltage in V

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Control module voltage in V
*/
float ELM327::ctrlModVoltage()
{
	if (queryPID(SERVICE_01, CONTROL_MODULE_VOLTAGE))
		return conditionResponse(findResponse(), 2, 1.0 / 1000.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::absLoad()

 Description:
 ------------
  *  Find absolute load value in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Absolute load value in %
*/
float ELM327::absLoad()
{
	if (queryPID(SERVICE_01, ABS_LOAD_VALUE))
		return conditionResponse(findResponse(), 2, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::commandedAirFuelRatio()

 Description:
 ------------
  *  Find commanded air-fuel equivalence ratio

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Commanded air-fuel equivalence ratio
*/
float ELM327::commandedAirFuelRatio()
{
	if (queryPID(SERVICE_01, FUEL_AIR_COMMANDED_EQUIV_RATIO))
		return conditionResponse(findResponse(), 2, 2.0 / 65536.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::relativeThrottle()

 Description:
 ------------
  *  Find relative throttle position in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Relative throttle position in %
*/
float ELM327::relativeThrottle()
{
	if (queryPID(SERVICE_01, RELATIVE_THROTTLE_POSITION))
		return conditionResponse(findResponse(), 1, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::ambientAirTemp()

 Description:
 ------------
  *  Find ambient air temperature in C

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Ambient air temperature in C
*/
float ELM327::ambientAirTemp()
{
	if (queryPID(SERVICE_01, AMBIENT_AIR_TEMP))
		return conditionResponse(findResponse(), 1, 1, -40);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::absThrottlePosB()

 Description:
 ------------
  *  Find absolute throttle position B in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Absolute throttle position B in %
*/
float ELM327::absThrottlePosB()
{
	if (queryPID(SERVICE_01, ABS_THROTTLE_POSITION_B))
		return conditionResponse(findResponse(), 1, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::absThrottlePosC()

 Description:
 ------------
  *  Find absolute throttle position C in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Absolute throttle position C in %
*/
float ELM327::absThrottlePosC()
{
	if (queryPID(SERVICE_01, ABS_THROTTLE_POSITION_C))
		return conditionResponse(findResponse(), 1, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::absThrottlePosD()

 Description:
 ------------
  *  Find absolute throttle position D in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Absolute throttle position D in %
*/
float ELM327::absThrottlePosD()
{
	if (queryPID(SERVICE_01, ABS_THROTTLE_POSITION_D))
		return conditionResponse(findResponse(), 1, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::absThrottlePosE()

 Description:
 ------------
  *  Find absolute throttle position E in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Absolute throttle position E in %
*/
float ELM327::absThrottlePosE()
{
	if (queryPID(SERVICE_01, ABS_THROTTLE_POSITION_E))
		return conditionResponse(findResponse(), 1, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::absThrottlePosF()

 Description:
 ------------
  *  Find absolute throttle position F in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Absolute throttle position F in %
*/
float ELM327::absThrottlePosF()
{
	if (queryPID(SERVICE_01, ABS_THROTTLE_POSITION_F))
		return conditionResponse(findResponse(), 1, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::commandedThrottleActuator()

 Description:
 ------------
  *  Find commanded throttle actuator in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Commanded throttle actuator in %
*/
float ELM327::commandedThrottleActuator()
{
	if (queryPID(SERVICE_01, COMMANDED_THROTTLE_ACTUATOR))
		return conditionResponse(findResponse(), 1, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 uint16_t ELM327::timeRunWithMIL()

 Description:
 ------------
  *  Find time run with MIL on in min

 Inputs:
 -------
  * void

 Return:
 -------
  * uint16_t - Time run with MIL on in min
*/
uint16_t ELM327::timeRunWithMIL()
{
	if (queryPID(SERVICE_01, TIME_RUN_WITH_MIL_ON))
		return conditionResponse(findResponse(), 2);

	return ELM_GENERAL_ERROR;
}




/*
 uint16_t ELM327::timeSinceCodesCleared()

 Description:
 ------------
  *  Find time since trouble codes cleared in min

 Inputs:
 -------
  * void

 Return:
 -------
  * uint16_t - Time since trouble codes cleared in min
*/
uint16_t ELM327::timeSinceCodesCleared()
{
	if (queryPID(SERVICE_01, TIME_SINCE_CODES_CLEARED))
		return conditionResponse(findResponse(), 2);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::maxMafRate()

 Description:
 ------------
  *  Find maximum value for air flow rate from mass air flow sensor in g/s

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Maximum value for air flow rate from mass air flow sensor in g/s
*/
float ELM327::maxMafRate()
{
	if (queryPID(SERVICE_01, MAX_MAF_RATE))
		return conditionResponse(findResponse(), 1, 10.0);

	return ELM_GENERAL_ERROR;
}




/*
 uint8_t ELM327::fuelType()

 Description:
 ------------
  *  Find fuel type (https://en.wikipedia.org/wiki/OBD-II_PIDs#Fuel_Type_Coding)

 Inputs:
 -------
  * void

 Return:
 -------
  * uint8_t - Bit encoded (https://en.wikipedia.org/wiki/OBD-II_PIDs#Fuel_Type_Coding)
*/
uint8_t ELM327::fuelType()
{
	if (queryPID(SERVICE_01, FUEL_TYPE))
		return conditionResponse(findResponse(), 1);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::ethonolPercent()

 Description:
 ------------
  *  Find ethanol fuel in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Ethanol fuel in %
*/
float ELM327::ethonolPercent()
{
	if (queryPID(SERVICE_01, ETHONOL_FUEL_PERCENT))
		return conditionResponse(findResponse(), 1, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::absEvapSysVapPressure()

 Description:
 ------------
  *  Find absolute evap. system vapor pressure in kPa

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Absolute evap. system vapor pressure in kPa
*/
float ELM327::absEvapSysVapPressure()
{
	if (queryPID(SERVICE_01, ABS_EVAP_SYS_VAPOR_PRESSURE))
		return conditionResponse(findResponse(), 2, 1.0 / 200.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::evapSysVapPressure2()

 Description:
 ------------
  *  Find evap. system vapor pressure in Pa

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Evap. system vapor pressure in Pa
*/
float ELM327::evapSysVapPressure2()
{
	if (queryPID(SERVICE_01, EVAP_SYS_VAPOR_PRESSURE))
		return conditionResponse(findResponse(), 2, 1, -32767);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::absFuelRailPressure()

 Description:
 ------------
  *  Find absolute fuel rail pressure in kPa

 Inputs:
 -------
  * void

 Return:
 -------
  * float - absolute fuel rail pressure in kPa
*/
float ELM327::absFuelRailPressure()
{
	if (queryPID(SERVICE_01, FUEL_RAIL_ABS_PRESSURE))
		return conditionResponse(findResponse(), 2, 10.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::relativePedalPos()

 Description:
 ------------
  *  Find relative accelerator pedal position in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Relative accelerator pedal position in %
*/
float ELM327::relativePedalPos()
{
	if (queryPID(SERVICE_01, RELATIVE_ACCELERATOR_PEDAL_POS))
		return conditionResponse(findResponse(), 1, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::hybridBatLife()

 Description:
 ------------
  *  Find hybrid battery pack remaining life in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Hybrid battery pack remaining life in %
*/
float ELM327::hybridBatLife()
{
	if (queryPID(SERVICE_01, HYBRID_BATTERY_REMAINING_LIFE))
		return conditionResponse(findResponse(), 1, 100.0 / 255.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::oilTemp()

 Description:
 ------------
  *  Find engine oil temperature in C

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Engine oil temperature in C
*/
float ELM327::oilTemp()
{
	if (queryPID(SERVICE_01, ENGINE_OIL_TEMP))
		return conditionResponse(findResponse(), 1, 1, -40.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::fuelInjectTiming()

 Description:
 ------------
  *  Find fuel injection timing in degrees

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Fuel injection timing in degrees
*/
float ELM327::fuelInjectTiming()
{
	if (queryPID(SERVICE_01, FUEL_INJECTION_TIMING))
		return conditionResponse(findResponse(), 2, 1.0 / 128.0, -210.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::fuelRate()

 Description:
 ------------
  *  Find engine fuel rate in L/h

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Engine fuel rate in L/h
*/
float ELM327::fuelRate()
{
	if (queryPID(SERVICE_01, ENGINE_FUEL_RATE))
		return conditionResponse(findResponse(), 2, 1.0 / 20.0);

	return ELM_GENERAL_ERROR;
}




/*
 uint8_t ELM327::emissionRqmts()

 Description:
 ------------
  *  Find emission requirements to which vehicle is designed

 Inputs:
 -------
  * void

 Return:
 -------
  * uint8_t - Bit encoded (?)
*/
uint8_t ELM327::emissionRqmts()
{
	if (queryPID(SERVICE_01, EMISSION_REQUIREMENTS))
		return conditionResponse(findResponse(), 1);

	return ELM_GENERAL_ERROR;
}




/*
 uint32_t ELM327::supportedPIDs_61_80()

 Description:
 ------------
  * Determine which of PIDs 0x61 through 0x80 are supported (bit encoded)

 Inputs:
 -------
  * void

 Return:
 -------
  * uint32_t - Bit encoded booleans of supported PIDs 0x61-0x80
*/
uint32_t ELM327::supportedPIDs_61_80()
{
	if (queryPID(SERVICE_01, SUPPORTED_PIDS_61_80))
		return conditionResponse(findResponse(), 4);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::demandedTorque()

 Description:
 ------------
  *  Find driver's demanded engine torque in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Driver's demanded engine torque in %
*/
float ELM327::demandedTorque()
{
	if (queryPID(SERVICE_01, DEMANDED_ENGINE_PERCENT_TORQUE))
		return conditionResponse(findResponse(), 1, 1, -125.0);

	return ELM_GENERAL_ERROR;
}




/*
 float ELM327::torque()

 Description:
 ------------
  *  Find actual engine torque in %

 Inputs:
 -------
  * void

 Return:
 -------
  * float - Actual engine torque in %
*/
float ELM327::torque()
{
	if (queryPID(SERVICE_01, ACTUAL_ENGINE_TORQUE))
		return conditionResponse(findResponse(), 1, 1, -125.0);

	return ELM_GENERAL_ERROR;
}




/*
 uint16_t ELM327::referenceTorque()

 Description:
 ------------
  *  Find engine reference torque in Nm

 Inputs:
 -------
  * void

 Return:
 -------
  * uint16_t - Engine reference torque in Nm
*/
uint16_t ELM327::referenceTorque()
{
	if (queryPID(SERVICE_01, ENGINE_REFERENCE_TORQUE))
		return conditionResponse(findResponse(), 2);

	return ELM_GENERAL_ERROR;
}




/*
 uint16_t ELM327::auxSupported()

 Description:
 ------------
  *  Find auxiliary input/output supported

 Inputs:
 -------
  * void

 Return:
 -------
  * uint16_t - Bit encoded (?)
*/
uint16_t ELM327::auxSupported()
{
	if (queryPID(SERVICE_01, AUX_INPUT_OUTPUT_SUPPORTED))
		return conditionResponse(findResponse(), 2);

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
    
	// clear payload buffer
	memset(payload, '\0', PAYLOAD_LEN + 1);

	// reset input serial buffer and number of received bytes
	recBytes = 0;
	flushInputBuff();

	if (debugMode)
	{
		Serial.print(F("Sending the following command/query: "));
		Serial.println(cmd);
	}

	elm_port->print(cmd);
	elm_port->print('\r');

	// prime the timeout timer
	previousTime = millis();
	currentTime  = previousTime;

	// buffer the response of the ELM327 until either the
	// end marker is read or a timeout has occurred
    // last valid idx is PAYLOAD_LEN but want to keep on free for terminating '\0'
    // so limit counter to < PAYLOAD_LEN
	while ((counter < PAYLOAD_LEN) && !timeout())
	{
		if (elm_port->available())
		{
			char recChar = elm_port->read();

			if (debugMode)
			{
				Serial.print(F("\tReceived char: "));

				if (recChar == '\f')
					Serial.println(F("\\f"));
				else if (recChar == '\n')
					Serial.println(F("\\n"));
				else if (recChar == '\r')
					Serial.println(F("\\r"));
				else if (recChar == '\t')
					Serial.println(F("\\t"));
				else if (recChar == '\v')
					Serial.println(F("\\v"));
				else
					Serial.println(recChar);
			}

			if (recChar == '>')
			{
				if (debugMode)
					Serial.println(F("Delimiter found"));

				break;
			}
			else if (!isalnum(recChar))
				continue;
			
			payload[counter] = recChar;
			counter++;
		}
	}

	if (debugMode)
	{
		Serial.print(F("All chars received: "));
		Serial.println(payload);
	}

	if (timeout())
	{
		if (debugMode)
		{
			Serial.print(F("Timeout detected with overflow of "));
			Serial.print((currentTime - previousTime) - timeout_ms);
			Serial.println(F("ms"));
		}

		status = ELM_TIMEOUT;
		return status;
	}
	
	if (nextIndex(payload, "UNABLETOCONNECT") >= 0)
	{
		if (debugMode)
			Serial.println(F("ELM responded with errror \"UNABLE TO CONNECT\""));

		status = ELM_UNABLE_TO_CONNECT;
		return status;
	}

	connected = true;

	if (nextIndex(payload, "NODATA") >= 0)
	{
		if (debugMode)
			Serial.println(F("ELM responded with errror \"NO DATA\""));

		status = ELM_NO_DATA;
		return status;
	}

	if (nextIndex(payload, "STOPPED") >= 0)
	{
		if (debugMode)
			Serial.println(F("ELM responded with errror \"STOPPED\""));

		status = ELM_STOPPED;
		return status;
	}

	if (nextIndex(payload, "ERROR") >= 0)
	{
		if (debugMode)
			Serial.println(F("ELM responded with \"ERROR\""));

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
 uint64_t ELM327::findResponse()

 Description:
 ------------
  * Parses the buffered ELM327's response and returns the queried data

 Inputs:
 -------
  * void

 Return:
 -------
  * uint64_t - Query response value
*/
uint64_t ELM327::findResponse()
{
	uint8_t firstDatum = 0;
	char header[7]     = { '\0' };

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

	if (debugMode)
	{
		Serial.print(F("Expected response header: "));
		Serial.println(header);
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
		// "responses" per query. "numPayChars" represents the
		// correct number of bytes returned by the ELM327
		// regardless of how many "responses" were returned
		if (secondHeadIndex >= 0)
		{
			if (debugMode)
				Serial.println(F("Double response detected"));

			numPayChars = secondHeadIndex - firstDatum;
		}
		else
		{
			if (debugMode)
				Serial.println(F("Single response detected"));

			numPayChars = recBytes - firstDatum;
		}

		response = 0;
		for(uint8_t i = 0; i < numPayChars; i++)
		{
			uint8_t payloadIndex = firstDatum + i;
			uint8_t bitsOffset = 4 * (numPayChars - i - 1);
			response = response | (ctoi(payload[payloadIndex]) << bitsOffset);
		}

		// It is usefull to have the response bytes
		// broken-out because some PID algorithms (standard
		// and custom) require special operations for each
		// byte returned
		responseByte_0 = response & 0xFF;
		responseByte_1 = (response >> 8) & 0xFF;
		responseByte_2 = (response >> 16) & 0xFF;
		responseByte_3 = (response >> 24) & 0xFF;
		responseByte_4 = (response >> 32) & 0xFF;
		responseByte_5 = (response >> 40) & 0xFF;
		responseByte_6 = (response >> 48) & 0xFF;
		responseByte_7 = (response >> 56) & 0xFF;

		if (debugMode)
		{
			Serial.println(F("64-bit response: "));
			Serial.print(F("\tresponseByte_0: "));
			Serial.println(responseByte_0);
			Serial.print(F("\tresponseByte_1: "));
			Serial.println(responseByte_1);
			Serial.print(F("\tresponseByte_2: "));
			Serial.println(responseByte_2);
			Serial.print(F("\tresponseByte_3: "));
			Serial.println(responseByte_3);
			Serial.print(F("\tresponseByte_4: "));
			Serial.println(responseByte_4);
			Serial.print(F("\tresponseByte_5: "));
			Serial.println(responseByte_5);
			Serial.print(F("\tresponseByte_6: "));
			Serial.println(responseByte_6);
			Serial.print(F("\tresponseByte_7: "));
			Serial.println(responseByte_7);
		}
		
		return response;
	}

	if (debugMode)
		Serial.println(F("Response not detected"));

	return 0;
}




/*
 void ELM327::printError()

 Description:
 ------------
  * Prints appropriate error description if an error has occurred

 Inputs:
 -------
  * void

 Return:
 -------
  * void
*/
void ELM327::printError()
{
	Serial.print(F("Received: "));
	Serial.println(payload);

	if (status == ELM_SUCCESS)
		Serial.println(F("ELM_SUCCESS"));
	else if (status == ELM_NO_RESPONSE)
		Serial.println(F("ERROR: ELM_NO_RESPONSE"));
	else if (status == ELM_BUFFER_OVERFLOW)
		Serial.println(F("ERROR: ELM_BUFFER_OVERFLOW"));
	else if (status == ELM_UNABLE_TO_CONNECT)
		Serial.println(F("ERROR: ELM_UNABLE_TO_CONNECT"));
	else if (status == ELM_NO_DATA)
		Serial.println(F("ERROR: ELM_NO_DATA"));
	else if (status == ELM_STOPPED)
		Serial.println(F("ERROR: ELM_STOPPED"));
	else if (status == ELM_TIMEOUT)
		Serial.println(F("ERROR: ELM_TIMEOUT"));
	else if (status == ELM_TIMEOUT)
		Serial.println(F("ERROR: ELM_GENERAL_ERROR"));
	else
		Serial.println(F("No error detected"));

	delay(100);
}
