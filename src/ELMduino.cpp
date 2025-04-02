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
  * bool - Whether or not the ELM327 was properly initialized
*/
bool ELM327::begin(      Stream&   stream,
                   const bool&     debug,
                   const uint16_t& timeout,
                   const char&     protocol,
                   const uint16_t& payloadLen,
                   const byte&     dataTimeout)
{
    elm_port    = &stream;
    PAYLOAD_LEN = payloadLen;
    debugMode   = debug;
    timeout_ms  = timeout;
    payload = (char *)malloc(PAYLOAD_LEN + 1); // allow for terminating '\0'

    // test if serial port is connected
    if (!elm_port)
        return false;

    // try to connect
    if (!initializeELM(protocol, dataTimeout))
        return false;

    return true;
}

ELM327::~ELM327() {
    if (payload) free(payload);
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
bool ELM327::initializeELM(const char& protocol,
                           const byte& dataTimeout)
{
    char command[10] = {'\0'};
    connected = false;

    sendCommand_Blocking(SET_ALL_TO_DEFAULTS);
    delay(100);

    sendCommand_Blocking(RESET_ALL);
    delay(100);

    sendCommand_Blocking(ECHO_OFF);
    delay(100);

    sendCommand_Blocking(PRINTING_SPACES_OFF);
    delay(100);

    sendCommand_Blocking(ALLOW_LONG_MESSAGES);
    delay(100);

    // // Set data timeout
    snprintf(command, sizeof(command), SET_TIMEOUT_TO_H_X_4MS, dataTimeout / 4);
    sendCommand_Blocking(command);
    delay(100);

    // Automatic searching for protocol requires setting the protocol to AUTO and then
    // sending an OBD command to initiate the protocol search. The OBD command "0100"
    // requests a list of supported PIDs 0x00 - 0x20 and is guaranteed to work
    if (protocol == '0')
    {
        // Tell the ELM327 to do an auto protocol search. If a valid protocol is found, it will be saved to memory.
        // Some ELM clones may not have memory enabled and thus will perform the search every time.
        snprintf(command, sizeof(command), SET_PROTOCOL_TO_AUTO_H_SAVE, protocol);
        if (sendCommand_Blocking(command) == ELM_SUCCESS)
        {
            if (strstr(payload, RESPONSE_OK) != NULL)
            {
                // Protocol search can take a comparatively long time. Temporarily set
                // the timeout value to 30 seconds, then restore the previous value.
                uint16_t prevTimeout = timeout_ms;
                timeout_ms = 30000;

                int8_t state = sendCommand_Blocking("0100");

                if (state == ELM_SUCCESS)
                {
                    timeout_ms = prevTimeout;
                    connected = true;
                    return connected;
                }
                else if (state == ELM_BUFFER_OVERFLOW)
                {
                    while (elm_port->available())
                        elm_port->read();
                }

                timeout_ms = prevTimeout;
            }
        }
    }
    else
    {
        // Set protocol
        snprintf(command, sizeof(command), TRY_PROT_H_AUTO_SEARCH, protocol);

        if (sendCommand_Blocking(command) == ELM_SUCCESS)
        {
            if (strstr(payload, RESPONSE_OK) != NULL)
            {
                connected = true;
                return connected;
            }
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
    snprintf(command, sizeof(command), SET_PROTOCOL_TO_H_SAVE, protocol);

    if (sendCommand_Blocking(command) == ELM_SUCCESS)
    {
        if (strstr(payload, RESPONSE_OK) != NULL)
        {
            connected = true;
            return connected;
        }
    }
    
    if (debugMode)
    {
        Serial.print(F("Setting protocol via "));
        Serial.print(SET_PROTOCOL_TO_H_SAVE);
        Serial.println(F(" did not work"));
    }

    return connected;
}

/*
 void ELM327::formatQueryArray(uint8_t service, uint16_t pid, uint8_t num_responses)

 Description:
 ------------
  * Creates a query stack to be sent to ELM327

 Inputs:
 -------
  * uint16_t service - Service number of the queried PID
  * uint32_t pid     - PID number of the queried PID
  * uint8_t num_responses - see function header for "queryPID()"

 Return:
 -------
  * void
*/
void ELM327::formatQueryArray(const uint8_t&  service,
                              const uint16_t& pid,
                              const uint8_t&  num_responses)
{
    if (debugMode)
    {
        Serial.print(F("Service: "));
        Serial.println(service);
        Serial.print(F("PID: "));
        Serial.println(pid);
    }

    isMode0x22Query = (service == 0x22 && pid <= 0xFF); // mode 0x22 responses always zero-pad the pid to 4 chars, even for a 2-char pid

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

        if (specifyNumResponses)
        {
            if (num_responses > 0xF)
            {
                query[6] = ((num_responses >> 4) & 0xF) + '0';
                query[7] = (num_responses & 0xF) + '0';
                query[8] = '\0';

                upper(query, 8);
            }
            else
            {
                query[6] = (num_responses & 0xF) + '0';
                query[7] = '\0';
                query[8] = '\0';

                upper(query, 7);
            }
        }
        else
        {
            query[6] = '\0';
            query[7] = '\0';
            query[8] = '\0';

            upper(query, 6);
        }
    }
    else
    {
        if (debugMode)
            Serial.println(F("Normal length query detected"));

        longQuery = false;

        query[2] = ((pid >> 4) & 0xF) + '0';
        query[3] = (pid & 0xF) + '0';

        if (specifyNumResponses)
        {
            if (num_responses > 0xF)
            {
                query[4] = ((num_responses >> 4) & 0xF) + '0';
                query[5] = (num_responses & 0xF) + '0';
                query[6] = '\0';
                query[7] = '\0';
                query[8] = '\0';

                upper(query, 6);
            }
            else
            {
                query[4] = (num_responses & 0xF) + '0';
                query[5] = '\0';
                query[6] = '\0';
                query[7] = '\0';
                query[8] = '\0';

                upper(query, 5);
            }
        }
        else
        {
            query[4] = '\0';
            query[5] = '\0';
            query[6] = '\0';
            query[7] = '\0';
            query[8] = '\0';

            upper(query, 4);
        }
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
void ELM327::upper(char   string[],
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
                         uint8_t     numOccur)
{
    char const *p = str;
    char const *r = str;
    uint8_t count;

    for (count = 0;; ++count)
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
 void ELM327::removeChar(char *from,
                          char const *remove)

 Description:
 ------------
  * Removes all instances of each char in string "remove" from the string "from"

 Inputs:
 -------
  * char *from         - String to remove target(s) from
  * char const *remove - Chars to find/remove 

 Return:
 -------
  * void
*/
void ELM327::removeChar(char *from, const char *remove)
{
    size_t i = 0, j = 0;
    while (from[i]) {
        if (!strchr(remove, from[i]))
            from[j++] = from[i];
        i++;
    }
    from[j] = '\0'; 
}
/*
 double ELM327::conditionResponse(const uint8_t &numExpectedBytes, const float &scaleFactor, const float &bias)

 Description:
 ------------
  * Converts the ELM327's response into its correct, numerical value. Returns 0 if numExpectedBytes > numPayChars

 Inputs:
 -------
  * uint64_t response        - ELM327's response
  * uint8_t numExpectedBytes - Number of valid bytes from the response to process
  * double scaleFactor       - Amount to scale the response by
  * float bias               - Amount to bias the response by

 Return:
 -------
  * double - Converted numerical value
*/
double ELM327::conditionResponse(const uint8_t& numExpectedBytes,
                                 const double&  scaleFactor,
                                 const double&   bias)
{
    uint8_t numExpectedPayChars = numExpectedBytes * 2;
    uint8_t payCharDiff         = numPayChars - numExpectedPayChars;

    if (numExpectedBytes > 8)
    {
        if (debugMode)
            Serial.println(F("WARNING: Number of expected response bytes is greater than 8 - returning 0"));

        return 0;
    }

    if (numPayChars < numExpectedPayChars)
    {
        if (debugMode)
            Serial.println(F("WARNING: Number of payload chars is less than the number of expected response chars returned by ELM327 - returning 0"));

        return 0;
    }
    else if (numPayChars & 0x1)
    {
        if (debugMode)
            Serial.println(F("WARNING: Number of payload chars returned by ELM327 is an odd value - returning 0"));

        return 0;
    }
    else if (numExpectedPayChars == numPayChars)
    {
        if (scaleFactor == 1 && bias == 0) // No scale/bias needed
            return response;
        else
            return (response * scaleFactor) + bias;
    }

    // If there were more payload bytes returned than we expected, test the first and last bytes in the
    // returned payload and see which gives us a higher value. Sometimes ELM327's return leading zeros
    // and others return trailing zeros. The following approach gives us the best chance at determining
    // where the real data is. Note that if the payload returns BOTH leading and trailing zeros, this
    // will not give accurate results!

    if (debugMode)
        Serial.println(F("Looking for lagging zeros"));

    uint16_t numExpectedBits  = numExpectedBytes * 8;
    uint64_t laggingZerosMask = 0;

    for (uint16_t i = 0; i < numExpectedBits; i++)
        laggingZerosMask |= (1 << i);

    if (!(laggingZerosMask & response)) // Detect all lagging zeros in `response`
    {
        if (debugMode)
            Serial.println(F("Lagging zeros found"));

        if (scaleFactor == 1 && bias == 0) // No scale/bias needed
            return (response >> (4 * payCharDiff));
        else
            return ((response >> (4 * payCharDiff)) * scaleFactor) + bias;
    }
    else
    {
        if (debugMode)
            Serial.println(F("Lagging zeros not found - assuming leading zeros"));

        if (scaleFactor == 1 && bias == 0) // No scale/bias needed
            return response;
        else
            return (response * scaleFactor) + bias;
    }
}

/*
 double ELM327::conditionResponse(double (*func)())

 Description:
 ------------
  * Provides a means to pass in a user-defined function to process the response. Used for PIDs that 
    don't use the common scaleFactor + Bias formula to calculate the value from the response data. Also
    useful for processing OEM custom PIDs which are too numerous and varied to encode in the lib. 

 Inputs:
 -------
  * (*func)() - pointer to function to do calculate response value
  
 Return:
 -------
  * double - Converted numerical value
*/

double ELM327::conditionResponse(double (*func)()) {
    return func();
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
  void ELM327::queryPID(const uint8_t& service, const uint16_t& pid, const uint8_t& num_responses)

  Description:
  ------------
  * Create a PID query command string and send the command

  Inputs:
  -------
  * uint8_t service       - The diagnostic service ID. 01 is "Show current data"
  * uint16_t pid          - The Parameter ID (PID) from the service
  * uint8_t num_responses - Number of lines of data to receive - see ELM datasheet "Talking to the vehicle".
                            This can speed up retrieval of information if you know how many responses will be sent.
                            Basically the OBD scanner will not wait for more responses if it does not need to go through
                            final timeout. Also prevents OBD scanners from sending mulitple of the same response.

  Return:
  -------
  * void
*/
void ELM327::queryPID(const uint8_t&  service,
                      const uint16_t& pid,
                      const uint8_t&  num_responses)
{
    formatQueryArray(service, pid, num_responses);
    sendCommand(query);
}

/*
 void ELM327::queryPID(char queryStr[])

 Description:
 ------------
  * Queries ELM327 for a specific type of vehicle telemetry data

 Inputs:
 -------
  * char queryStr[] - Query string (service and PID)

 Return:
 -------
  * void
*/
void ELM327::queryPID(char queryStr[])
{
    if (strlen(queryStr) <= 4)
        longQuery = false;
    else
        longQuery = true;

    sendCommand(queryStr);
}

/*
 double ELM327::processPID(const uint8_t& service, const uint16_t& pid, const uint8_t& num_responses, const uint8_t& numExpectedBytes, const float& scaleFactor, const float& bias)

 Description:
 ------------
  * Queries ELM327 for a specific type of vehicle telemetry data

 Inputs:
 -------
  * uint8_t service          - The diagnostic service ID. 01 is "Show current data"
  * uint16_t pid             - The Parameter ID (PID) from the service
  * uint8_t num_responses    - Number of lines of data to receive - see ELM datasheet "Talking to the vehicle".
                               This can speed up retrieval of information if you know how many responses will be sent.
                               Basically the OBD scanner will not wait for more responses if it does not need to go through
                               final timeout. Also prevents OBD scanners from sending mulitple of the same response.
  * uint8_t numExpectedBytes - Number of valid bytes from the response to process
  * float scaleFactor        - Amount to scale the response by
  * float bias               - Amount to bias the response by

 Return:
 -------
  * double - The PID value if successfully received, else 0.0
*/
double ELM327::processPID(const uint8_t&  service,
                          const uint16_t& pid,
                          const uint8_t&  num_responses,
                          const uint8_t&  numExpectedBytes,
                          const double&   scaleFactor,
                          const float&    bias)
{
    if (nb_query_state == SEND_COMMAND)
    {
        queryPID(service, pid, num_responses);
        nb_query_state = WAITING_RESP;
    }
    else if (nb_query_state == WAITING_RESP)
    {
        get_response();
        if (nb_rx_state == ELM_SUCCESS)
        {
            nb_query_state = SEND_COMMAND; // Reset the query state machine for next command
            findResponse();
            
            /* This data manipulation seems duplicative of the responseByte_0, responseByte_1, etc vars and it is.
               The duplcation is deliberate to provide a clear way for the calculator functions to access the relevant
               data bytes from the response in the format they are commonly expressed in and without breaking backward
               compatability with existing code that may be using the responseByte_n vars. 
               
               In addition, we need to place the response values into static vars that can be accessed by the (static) 
               calculator functions. A future (breaking!) change could be made to eliminate this duplication. 
            */
            uint8_t responseBits = numExpectedBytes * 8;
            uint8_t extractedBytes[8] = {0};  // Store extracted bytes

            // Extract bytes only if shift is non-negative
            for (int i = 0; i < numExpectedBytes; i++)
            {
                int shiftAmount = responseBits - (8 * (i + 1));             // Compute shift amount
                if (shiftAmount >= 0) {                                     //  Ensure valid shift
                    extractedBytes[i] = (response >> shiftAmount) & 0xFF;   // Extract byte
                }
            }

            // Assign extracted values to response_A, response_B, ..., response_H safely
            response_A = extractedBytes[0];
            response_B = extractedBytes[1];
            response_C = extractedBytes[2];
            response_D = extractedBytes[3];
            response_E = extractedBytes[4];
            response_F = extractedBytes[5];
            response_G = extractedBytes[6];
            response_H = extractedBytes[7];
            
            double (*calculator)() = selectCalculator(pid);

            if (nullptr == calculator) {
                //Use the default scaleFactor + Bias calculation
                return conditionResponse(numExpectedBytes, scaleFactor, bias);
            }
            else {
                return conditionResponse(calculator);
            }
        }
        else if (nb_rx_state != ELM_GETTING_MSG)
            nb_query_state = SEND_COMMAND; // Error or timeout, so reset the query state machine for next command
    }
    return 0.0;
}
/*
 double ELM327::selectCalculator(uint16_t pid))()

 Description:
 ------------
  * Selects the appropriate calculation function for a given PID.

 Inputs:
 -------
  * uint16_t pid             - The Parameter ID (PID) from the service
  

 Return:
 -------
  * double (*func()) - Pointer to a function to be used to calculate the value for this PID.
    Returns nullptr if the PID is calculated using the default scaleFactor + Bias formula as
    implemented in conditionResponse(). (Maintained for backward compatibility)
*/
double (*ELM327::selectCalculator(uint16_t pid))() 
{       
    switch (pid)
    {
        case ENGINE_LOAD:
        case ENGINE_COOLANT_TEMP:
        case SHORT_TERM_FUEL_TRIM_BANK_1:
        case LONG_TERM_FUEL_TRIM_BANK_1:
        case SHORT_TERM_FUEL_TRIM_BANK_2:
        case LONG_TERM_FUEL_TRIM_BANK_2:
        case FUEL_PRESSURE:
        case INTAKE_MANIFOLD_ABS_PRESSURE:
        case VEHICLE_SPEED:
        case TIMING_ADVANCE:
        case INTAKE_AIR_TEMP:
        case THROTTLE_POSITION:
        case COMMANDED_EGR:
        case EGR_ERROR:
        case COMMANDED_EVAPORATIVE_PURGE:
        case FUEL_TANK_LEVEL_INPUT:
        case WARM_UPS_SINCE_CODES_CLEARED:
        case ABS_BAROMETRIC_PRESSURE:
        case RELATIVE_THROTTLE_POSITION:
        case AMBIENT_AIR_TEMP:
        case ABS_THROTTLE_POSITION_B:
        case ABS_THROTTLE_POSITION_C:
        case ABS_THROTTLE_POSITION_D:
        case ABS_THROTTLE_POSITION_E:
        case ABS_THROTTLE_POSITION_F:
        case COMMANDED_THROTTLE_ACTUATOR:
        case ETHANOL_FUEL_PERCENT:
        case RELATIVE_ACCELERATOR_PEDAL_POS:
        case HYBRID_BATTERY_REMAINING_LIFE:
        case ENGINE_OIL_TEMP:
        case DEMANDED_ENGINE_PERCENT_TORQUE:
        case ACTUAL_ENGINE_TORQUE:
            return nullptr; 

        case ENGINE_RPM:
            return calculator_0C;

        case MAF_FLOW_RATE:
            return calculator_10;

        case OXYGEN_SENSOR_1_A:
        case OXYGEN_SENSOR_2_A:
        case OXYGEN_SENSOR_3_A:
        case OXYGEN_SENSOR_4_A:
        case OXYGEN_SENSOR_5_A:
        case OXYGEN_SENSOR_6_A:
        case OXYGEN_SENSOR_7_A:
        case OXYGEN_SENSOR_8_A:
        case OXYGEN_SENSOR_1_B:
        case OXYGEN_SENSOR_2_B:
        case OXYGEN_SENSOR_3_B:
        case OXYGEN_SENSOR_4_B:
        case OXYGEN_SENSOR_6_B:
        case OXYGEN_SENSOR_7_B:
        case OXYGEN_SENSOR_8_B:
        case OXYGEN_SENSOR_1_C:
        case OXYGEN_SENSOR_2_C:
        case OXYGEN_SENSOR_3_C:
        case OXYGEN_SENSOR_4_C:
        case OXYGEN_SENSOR_5_C:
        case OXYGEN_SENSOR_6_C:
        case OXYGEN_SENSOR_7_C:
        case OXYGEN_SENSOR_8_C:
            return calculator_14;

        case RUN_TIME_SINCE_ENGINE_START:
        case DISTANCE_TRAVELED_WITH_MIL_ON:
        case DIST_TRAV_SINCE_CODES_CLEARED:
        case TIME_RUN_WITH_MIL_ON:
        case TIME_SINCE_CODES_CLEARED:
        case ENGINE_REFERENCE_TORQUE:
            return calculator_1F;

        case FUEL_RAIL_PRESSURE:
            return calculator_22;

        case FUEL_RAIL_GUAGE_PRESSURE:
        case FUEL_RAIL_ABS_PRESSURE:
            return calculator_23;

        case EVAP_SYSTEM_VAPOR_PRESSURE:
            return calculator_32;
     
        case CATALYST_TEMP_BANK_1_SENSOR_1:
        case CATALYST_TEMP_BANK_2_SENSOR_1:
        case CATALYST_TEMP_BANK_1_SENSOR_2:
        case CATALYST_TEMP_BANK_2_SENSOR_2:
            return calculator_3C;

        case CONTROL_MODULE_VOLTAGE:
            return calculator_42;

        case ABS_LOAD_VALUE:
            return calculator_43;

        case FUEL_AIR_COMMANDED_EQUIV_RATIO:
            return calculator_44;

        case MAX_VALUES_EQUIV_V_I_PRESSURE:
            return calculator_4F;

        case MAX_MAF_RATE:
            return calculator_50;

        case ABS_EVAP_SYS_VAPOR_PRESSURE:
            return calculator_53;

        case SHORT_TERM_SEC_OXY_SENS_TRIM_1_3:
        case LONG_TERM_SEC_OXY_SENS_TRIM_1_3:
        case SHORT_TERM_SEC_OXY_SENS_TRIM_2_4:
        case LONG_TERM_SEC_OXY_SENS_TRIM_2_4:
            return calculator_55;

        case FUEL_INJECTION_TIMING:
            return calculator_5D;

        case ENGINE_FUEL_RATE:
            return calculator_5E;

        default:
            return nullptr;    
        
    }
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
    return (uint32_t)processPID(SERVICE_01, SUPPORTED_PIDS_1_20, 1, 4);
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
    return (uint32_t)processPID(SERVICE_01, MONITOR_STATUS_SINCE_DTC_CLEARED, 1, 4);
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
    return (uint16_t)processPID(SERVICE_01, FREEZE_DTC, 1, 2);
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
    return (uint16_t)processPID(SERVICE_01, FUEL_SYSTEM_STATUS, 1, 2);
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
    return processPID(SERVICE_01, ENGINE_LOAD, 1, 1, 100.0 / 255.0);
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
    return processPID(SERVICE_01, ENGINE_COOLANT_TEMP, 1, 1, 1, -40.0);
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
    return processPID(SERVICE_01, SHORT_TERM_FUEL_TRIM_BANK_1, 1, 1, 100.0 / 128.0, -100.0);
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
    return processPID(SERVICE_01, LONG_TERM_FUEL_TRIM_BANK_1, 1, 1, 100.0 / 128.0, -100.0);
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
    return processPID(SERVICE_01, SHORT_TERM_FUEL_TRIM_BANK_2, 1, 1, 100.0 / 128.0, -100.0);
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
    return processPID(SERVICE_01, LONG_TERM_FUEL_TRIM_BANK_2, 1, 1, 100.0 / 128.0, -100.0);
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
    return processPID(SERVICE_01, FUEL_PRESSURE, 1, 1, 3.0);
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
    return (uint8_t)processPID(SERVICE_01, INTAKE_MANIFOLD_ABS_PRESSURE, 1, 1);
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
    return processPID(SERVICE_01, ENGINE_RPM, 1, 2, 1.0 / 4.0);
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
    return (int32_t)processPID(SERVICE_01, VEHICLE_SPEED, 1, 1);
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
    return kph() * KPH_MPH_CONVERT;
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
    return processPID(SERVICE_01, TIMING_ADVANCE, 1, 1, 1.0 / 2.0, -64.0);
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
    return processPID(SERVICE_01, INTAKE_AIR_TEMP, 1, 1, 1, -40.0);
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
    return processPID(SERVICE_01, MAF_FLOW_RATE, 1, 2, 1.0 / 100.0);
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
    return processPID(SERVICE_01, THROTTLE_POSITION, 1, 1, 100.0 / 255.0);
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
    return (uint8_t)processPID(SERVICE_01, COMMANDED_SECONDARY_AIR_STATUS, 1, 1);
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
    return (uint8_t)processPID(SERVICE_01, OXYGEN_SENSORS_PRESENT_2_BANKS, 1, 1);
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
    return (uint8_t)processPID(SERVICE_01, OBD_STANDARDS, 1, 1);
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
    return (uint8_t)processPID(SERVICE_01, OXYGEN_SENSORS_PRESENT_4_BANKS, 1, 1);
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
    return (bool)processPID(SERVICE_01, AUX_INPUT_STATUS, 1, 1);
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
    return (uint16_t)processPID(SERVICE_01, RUN_TIME_SINCE_ENGINE_START, 1, 2);
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
    return (uint32_t)processPID(SERVICE_01, SUPPORTED_PIDS_21_40, 1, 4);
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
    return (uint16_t)processPID(SERVICE_01, DISTANCE_TRAVELED_WITH_MIL_ON, 1, 2);
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
    return processPID(SERVICE_01, FUEL_RAIL_PRESSURE, 1, 2, 0.079);
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
    return processPID(SERVICE_01, FUEL_RAIL_GUAGE_PRESSURE, 1, 2, 10.0);
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
    return processPID(SERVICE_01, COMMANDED_EGR, 1, 1, 100.0 / 255.0);
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
    return processPID(SERVICE_01, EGR_ERROR, 1, 1, 100.0 / 128.0, -100);
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
    return processPID(SERVICE_01, COMMANDED_EVAPORATIVE_PURGE, 1, 1, 100.0 / 255.0);
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
    return processPID(SERVICE_01, FUEL_TANK_LEVEL_INPUT, 1, 1, 100.0 / 255.0);
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
    return (uint8_t)processPID(SERVICE_01, WARM_UPS_SINCE_CODES_CLEARED, 1, 1);
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
    return (uint16_t)processPID(SERVICE_01, DIST_TRAV_SINCE_CODES_CLEARED, 1, 2);
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
    return processPID(SERVICE_01, EVAP_SYSTEM_VAPOR_PRESSURE, 1, 2, 1.0 / 4.0);
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
    return (uint8_t)processPID(SERVICE_01, ABS_BAROMETRIC_PRESSURE, 1, 1);
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
    return processPID(SERVICE_01, CATALYST_TEMP_BANK_1_SENSOR_1, 1, 2, 1.0 / 10.0, -40.0);
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
    return processPID(SERVICE_01, CATALYST_TEMP_BANK_2_SENSOR_1, 1, 2, 1.0 / 10.0, -40.0);
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
    return processPID(SERVICE_01, CATALYST_TEMP_BANK_1_SENSOR_2, 1, 2, 1.0 / 10.0, -40.0);
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
    return processPID(SERVICE_01, CATALYST_TEMP_BANK_2_SENSOR_2, 1, 2, 1.0 / 10.0, -40.0);
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
    return (uint32_t)processPID(SERVICE_01, SUPPORTED_PIDS_41_60, 1, 4);
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
    return (uint32_t)processPID(SERVICE_01, MONITOR_STATUS_THIS_DRIVE_CYCLE, 1, 4);
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
    return processPID(SERVICE_01, CONTROL_MODULE_VOLTAGE, 1, 2, 1.0 / 1000.0);
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
    return processPID(SERVICE_01, ABS_LOAD_VALUE, 1, 2, 100.0 / 255.0);
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
    return processPID(SERVICE_01, FUEL_AIR_COMMANDED_EQUIV_RATIO, 1, 2, 2.0 / 65536.0);
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
    return processPID(SERVICE_01, RELATIVE_THROTTLE_POSITION, 1, 1, 100.0 / 255.0);
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
    return processPID(SERVICE_01, AMBIENT_AIR_TEMP, 1, 1, 1, -40);
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
    return processPID(SERVICE_01, ABS_THROTTLE_POSITION_B, 1, 1, 100.0 / 255.0);
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
    return processPID(SERVICE_01, ABS_THROTTLE_POSITION_C, 1, 1, 100.0 / 255.0);
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
    return processPID(SERVICE_01, ABS_THROTTLE_POSITION_D, 1, 1, 100.0 / 255.0);
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
    return processPID(SERVICE_01, ABS_THROTTLE_POSITION_E, 1, 1, 100.0 / 255.0);
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
    return processPID(SERVICE_01, ABS_THROTTLE_POSITION_F, 1, 1, 100.0 / 255.0);
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
    return processPID(SERVICE_01, COMMANDED_THROTTLE_ACTUATOR, 1, 1, 100.0 / 255.0);
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
    return (uint16_t)processPID(SERVICE_01, TIME_RUN_WITH_MIL_ON, 1, 2);
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
    return (uint16_t)processPID(SERVICE_01, TIME_SINCE_CODES_CLEARED, 1, 2);
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
    return processPID(SERVICE_01, MAX_MAF_RATE, 1, 1, 10.0);
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
    return (uint8_t)processPID(SERVICE_01, FUEL_TYPE, 1, 1);
}

/*
 float ELM327::ethanolPercent()

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
float ELM327::ethanolPercent()
{
    return processPID(SERVICE_01, ETHANOL_FUEL_PERCENT, 1, 1, 100.0 / 255.0);
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
    return processPID(SERVICE_01, ABS_EVAP_SYS_VAPOR_PRESSURE, 1, 2, 1.0 / 200.0);
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
    return processPID(SERVICE_01, EVAP_SYS_VAPOR_PRESSURE, 1, 2, 1, -32767);
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
    return processPID(SERVICE_01, FUEL_RAIL_ABS_PRESSURE, 1, 2, 10.0);
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
    return processPID(SERVICE_01, RELATIVE_ACCELERATOR_PEDAL_POS, 1, 1, 100.0 / 255.0);
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
    return processPID(SERVICE_01, HYBRID_BATTERY_REMAINING_LIFE, 1, 1, 100.0 / 255.0);
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
    return processPID(SERVICE_01, ENGINE_OIL_TEMP, 1, 1, 1, -40.0);
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
    return processPID(SERVICE_01, FUEL_INJECTION_TIMING, 1, 2, 1.0 / 128.0, -210.0);
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
    return processPID(SERVICE_01, ENGINE_FUEL_RATE, 1, 2, 1.0 / 20.0);
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
    return (uint8_t)processPID(SERVICE_01, EMISSION_REQUIREMENTS, 1, 1);
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
    return (uint32_t)processPID(SERVICE_01, SUPPORTED_PIDS_61_80, 1, 4);
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
    return processPID(SERVICE_01, DEMANDED_ENGINE_PERCENT_TORQUE, 1, 1, 1, -125.0);
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
    return processPID(SERVICE_01, ACTUAL_ENGINE_TORQUE, 1, 1, 1, -125.0);
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
    return processPID(SERVICE_01, ENGINE_REFERENCE_TORQUE, 1, 2);
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
    return (uint16_t)processPID(SERVICE_01, AUX_INPUT_OUTPUT_SUPPORTED, 1, 2);
}

/*
 void ELM327::sendCommand(const char *cmd)

 Description:
 ------------
  * Sends a command/query for Non-Blocking PID queries

 Inputs:
 -------
  * const char *cmd - Command/query to send to ELM327

 Return:
 -------
  * void
*/
void ELM327::sendCommand(const char *cmd)
{
    // clear payload buffer
    memset(payload, '\0', PAYLOAD_LEN + 1);

    // reset input serial buffer and number of received bytes
    recBytes = 0;
    flushInputBuff();
    connected = false;

    // Reset the receive state ready to start receiving a response message
    nb_rx_state = ELM_GETTING_MSG;

    if (debugMode)
    {
        Serial.print(F("Sending the following command/query: "));
        Serial.println(cmd);
    }

    elm_port->print(cmd);
    elm_port->print('\r');

    // prime the timeout timer
    previousTime = millis();
    currentTime = previousTime;
}

/*
 obd_rx_states ELM327::sendCommand_Blocking(const char* cmd)

 Description:
 ------------
    * Sends a command/query and waits for a respoonse (blocking function)
    Sometimes it's desirable to use a blocking command, e.g when sending an AT command.
    This function removes the need for the caller to set up a loop waiting for the command to finish.
    Caller is free to parse the payload string if they need to use the response.

 Inputs:
 -------
  * const char *cmd - Command/query to send to ELM327

 Return:
 -------
  * int8_t - the ELM_XXX status of getting the OBD response
*/
int8_t ELM327::sendCommand_Blocking(const char *cmd)
{
    sendCommand(cmd);
    uint32_t startTime = millis();
    while (get_response() == ELM_GETTING_MSG) {
        if (millis() - startTime > timeout_ms) break;
    }
    return nb_rx_state;
}

/*
 obd_rx_states ELM327::get_response(void)

 Description:
 ------------
  * Non Blocking (NB) receive OBD scanner response. Must be called repeatedly until
    the status progresses past ELM_GETTING_MSG.

 Inputs:
 -------
  * void

 Return:
 -------
  * int8_t - the ELM_XXX status of getting the OBD response
*/
int8_t ELM327::get_response(void)
{
    // buffer the response of the ELM327 until either the
    // end marker is read or a timeout has occurred
    // last valid idx is PAYLOAD_LEN but want to keep one free for terminating '\0'
    // so limit counter to < PAYLOAD_LEN
    if (!elm_port->available())
    {
        nb_rx_state = ELM_GETTING_MSG;
        if (timeout())
            nb_rx_state = ELM_TIMEOUT;
    }
    else
    {
        char recChar = elm_port->read();

        if (debugMode)
        {
            Serial.print(F("\tReceived char: "));
            // display each received character, make non-printables printable
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
            // convert spaces to underscore, easier to see in debug output
            else if (recChar == ' ')
                Serial.println(F("_"));
            // display regular printable
            else
                Serial.println(recChar);
        }

        // this is the end of the OBD response
        if (recChar == '>')
        {
            if (debugMode)
                Serial.println(F("Delimiter found."));

            nb_rx_state = ELM_MSG_RXD;
        }
        else if (!isalnum(recChar) && (recChar != ':') && (recChar != '.') && (recChar != '\r'))
            // Keep only alphanumeric, decimal, colon, CR. These are needed for response parsing
            // decimal places needed to extract floating point numbers, e.g. battery voltage
            nb_rx_state = ELM_GETTING_MSG; // Discard this character
        else
        {
            if (recBytes < PAYLOAD_LEN)
            {
                payload[recBytes] = recChar;
                recBytes++;
                nb_rx_state = ELM_GETTING_MSG;
            }
            else
                nb_rx_state = ELM_BUFFER_OVERFLOW;
        }
    }

    // Message is still being received (or is timing out), so exit early without doing all the other checks
    if (nb_rx_state == ELM_GETTING_MSG)
        return nb_rx_state;

    // End of response delimiter was found
    if (debugMode && nb_rx_state == ELM_MSG_RXD)
    {
        Serial.print(F("All chars received: "));
        Serial.println(payload);
    }

    if (nb_rx_state == ELM_TIMEOUT)
    {
        if (debugMode)
        {
            Serial.print(F("Timeout detected with overflow of "));
            Serial.print((currentTime - previousTime) - timeout_ms);
            Serial.println(F("ms"));
        }
        return nb_rx_state;
    }

    if (nb_rx_state == ELM_BUFFER_OVERFLOW)
    {
        if (debugMode)
        {
            Serial.print(F("OBD receive buffer overflow (> "));
            Serial.print(PAYLOAD_LEN);
            Serial.println(F(" bytes)"));
        }
        return nb_rx_state;
    }

    // Now we have successfully received OBD response, check if the payload indicates any OBD errors
    if (nextIndex(payload, RESPONSE_UNABLE_TO_CONNECT) >= 0)
    {
        if (debugMode)
            Serial.println(F("ELM responded with error \"UNABLE TO CONNECT\""));

        nb_rx_state = ELM_UNABLE_TO_CONNECT;
        return nb_rx_state;
    }

    connected = true;

    if (nextIndex(payload, RESPONSE_NO_DATA) >= 0)
    {
        if (debugMode)
            Serial.println(F("ELM responded with error \"NO DATA\""));

        nb_rx_state = ELM_NO_DATA;
        return nb_rx_state;
    }

    if (nextIndex(payload, RESPONSE_STOPPED) >= 0)
    {
        if (debugMode)
            Serial.println(F("ELM responded with error \"STOPPED\""));

        nb_rx_state = ELM_STOPPED;
        return nb_rx_state;
    }

    if (nextIndex(payload, RESPONSE_ERROR) >= 0)
    {
        if (debugMode)
            Serial.println(F("ELM responded with \"ERROR\""));

        nb_rx_state = ELM_GENERAL_ERROR;
        return nb_rx_state;
    }

    nb_rx_state = ELM_SUCCESS;
    // Need to process multiline repsonses, remove '\r' from non multiline resp
    if (NULL != strchr(payload, ':')) {
        parseMultiLineResponse();
    } 
    else {
        removeChar(payload, " \r");
    }
    recBytes = strlen(payload); 
    return nb_rx_state;
}

/*
 void ELM327::parseMultilineResponse()
 
 Description:
 ------------
  * Parses a buffered multiline response into a single line with the specified data
  * Modifies the value of payload for further processing and removes the '\r' chars

 Inputs:
 -------
  * void

 Return:
 -------
  * void
*/
void ELM327::parseMultiLineResponse() {
    uint8_t totalBytes = 0;
    uint8_t bytesReceived = 0;
    char newResponse[PAYLOAD_LEN];
    memset(newResponse, 0, PAYLOAD_LEN * sizeof(char)); // Initialize newResponse to empty string
    char line[256] = "";
    char* start = payload;
    char* end = strchr(start, '\r');
  
   do 
    {   //Step 1: Get a line from the response
        memset(line, '\0', 256); 
        if (end != NULL) {
            strncpy(line, start, end - start);
            line[end - start] = '\0';
        } else {
            strncpy(line, start, strlen(start));
            line[strlen(start)] = '\0';
    
            // Exit when there's no more data
            if (strlen(line) == 0) break;
        }
    
        if (debugMode) {
            Serial.print(F("Found line in response: "));
            Serial.println(line);
        }
        // Step 2: Check if this is the first line of the response
        if (0 == totalBytes)
        // Some devices return the response header in the first line instead of the data length, ignore this line
        // Line containing totalBytes indicator is 3 hex chars only, longer first line will be a header.
        { 
            if (strlen(line) > 3) {
                if (debugMode)
                {
                    Serial.print(F("Found header in response line: "));
                    Serial.println(line); 
                }
            }
            else {
                if (strlen(line) > 0) {
                    totalBytes = strtol(line, NULL, 16) * 2;
                    if (debugMode) {
                        Serial.print(F("totalBytes = "));
                        Serial.println(totalBytes);
                    }
                }
            }
        } 
        // Step 3: Process data response lines 
        else { 
            if (strchr(line, ':')) {
                char* dataStart = strchr(line, ':') + 1;
                uint8_t dataLength = strlen(dataStart);
                uint8_t bytesToCopy = (bytesReceived + dataLength > totalBytes) ? (totalBytes - bytesReceived) : dataLength;
                if (bytesReceived + bytesToCopy > PAYLOAD_LEN - 1) {
                    bytesToCopy = (PAYLOAD_LEN - 1) - bytesReceived;
                }
                strncat(newResponse, dataStart, bytesToCopy);
                bytesReceived += bytesToCopy;

                if (debugMode) {
                    Serial.print(F("Response data: "));
                    Serial.println(dataStart);
                }
            }
        }
        if (*(end + 1) == '\0') {  
            start = NULL;  
        } else {
            start = end + 1;
        }
        end = (start != NULL) ? strchr(start, '\r') : NULL;

    } while ((bytesReceived < totalBytes || 0 == totalBytes) && start != NULL);

    // Replace payload with parsed response, null-terminate after totalBytes
    int nullTermPos = (totalBytes < PAYLOAD_LEN - 1) ? totalBytes : PAYLOAD_LEN - 1;
    strncpy(payload, newResponse, nullTermPos);
    payload[nullTermPos] = '\0'; // Ensure null termination
    if (debugMode) 
    {
        Serial.print(F("Parsed multiline response: "));
        Serial.println(payload);
    }
}


/*
 uint64_t ELM327::findResponse(const uint8_t& service, const uint8_t& pid)

 Description:
 ------------
  * Parses the buffered ELM327's response and returns the queried data

 Inputs:
 -------
  * const uint8_t& service - The diagnostic service ID. 01 is "Show current data"
  * const uint8_t& pid     - The Parameter ID (PID) from the service

 Return:
 -------
  * void
*/
uint64_t ELM327::findResponse()
{
    uint8_t firstDatum = 0;
    char header[7] = {'\0'};

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

        if (isMode0x22Query) // mode 0x22 responses always zero-pad the pid to 4 chars, even for a 2-char pid
        {
            header[2] = '0';
            header[3] = '0';
            header[4] = query[2];
            header[5] = query[3];
        }
        else
        {
            header[2] = query[2];
            header[3] = query[3];
        }
    }

    if (debugMode)
    {
        Serial.print(F("Expected response header: "));
        Serial.println(header);
    }

    int8_t firstHeadIndex  = nextIndex(payload, header, 1);
    int8_t secondHeadIndex = nextIndex(payload, header, 2);

    if (firstHeadIndex >= 0)
    {
        if (longQuery | isMode0x22Query)
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

            numPayChars = strlen(payload) - firstDatum;
        }

        response = 0;
        for (uint8_t i = 0; i < numPayChars; i++)
        {
            uint8_t payloadIndex = firstDatum + i;
            uint8_t bitsOffset   = 4 * (numPayChars - i - 1);

            if (debugMode)
            {
                Serial.print(F("\tProcessing hex nibble: "));
                Serial.println(payload[payloadIndex]);
            }
            response = response | ((uint64_t)ctoi(payload[payloadIndex]) << bitsOffset);
        }

        // It is useful to have the response bytes
        // broken-out because some PID algorithms (standard
        // and custom) require special operations for each
        // byte returned

        responseByte_0 =  response        & 0xFF; 
        responseByte_1 = (response >> 8)  & 0xFF;
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

    if (nb_rx_state == ELM_SUCCESS)
        Serial.println(F("ELM_SUCCESS"));
    else if (nb_rx_state == ELM_NO_RESPONSE)
        Serial.println(F("ERROR: ELM_NO_RESPONSE"));
    else if (nb_rx_state == ELM_BUFFER_OVERFLOW)
        Serial.println(F("ERROR: ELM_BUFFER_OVERFLOW"));
    else if (nb_rx_state == ELM_UNABLE_TO_CONNECT)
        Serial.println(F("ERROR: ELM_UNABLE_TO_CONNECT"));
    else if (nb_rx_state == ELM_NO_DATA)
        Serial.println(F("ERROR: ELM_NO_DATA"));
    else if (nb_rx_state == ELM_STOPPED)
        Serial.println(F("ERROR: ELM_STOPPED"));
    else if (nb_rx_state == ELM_TIMEOUT)
        Serial.println(F("ERROR: ELM_TIMEOUT"));
    else if (nb_rx_state == ELM_BUFFER_OVERFLOW)
        Serial.println(F("ERROR: BUFFER OVERFLOW"));
    else if (nb_rx_state == ELM_GENERAL_ERROR)
        Serial.println(F("ERROR: ELM_GENERAL_ERROR"));
    else
        Serial.println(F("No error detected"));

    delay(100);
}

/*
 float ELM327::batteryVoltage()

 Description:
 ------------
  * Get the current vehicle battery voltage in Volts DC

 Inputs:
 -------
  * void

 Return:
 -------
  * float - vehicle battery voltage in VDC
*/
float ELM327::batteryVoltage()
{
    if (nb_query_state == SEND_COMMAND)
    {
        sendCommand(READ_VOLTAGE);
        nb_query_state = WAITING_RESP;
    }
    else if (nb_query_state == WAITING_RESP)
    {
        get_response();
        if (nb_rx_state == ELM_SUCCESS)
        {
            nb_query_state = SEND_COMMAND;         // Reset the query state machine for next command
            payload[strlen(payload) - 1] = '\0';   // Remove the last char ("V") from the payload value

            if (strncmp(payload, "ATRV", 4) == 0)
                return (float)strtod(payload + 4, NULL);
            else
                return (float)strtod(payload, NULL);
        }
        else if (nb_rx_state != ELM_GETTING_MSG)
            nb_query_state = SEND_COMMAND; // Error or timeout, so reset the query state machine for next command
    }
    return 0.0;
}

/*
 int8_t ELM327::get_vin_blocking(char *vin)

 Description:
 ------------
    * Read Vehicle Identification Number (VIN). This is a blocking function.

 Inputs:
 -------
    * char vin[] - pointer to c-string in which to store VIN
        Note: (allocate memory for 18 character c-string in calling function)

 Return:
 -------
  * int8_t - the ELM_XXX status of getting the VIN
*/
int8_t ELM327::get_vin_blocking(char vin[])
{
    char temp[3] = {0};
    char *idx;
    uint8_t vin_counter = 0;
    uint8_t ascii_val;

    if (debugMode)
        Serial.println(F("Getting VIN..."));

    sendCommand("0902"); // VIN is command 0902
    while (get_response() == ELM_GETTING_MSG)
        ;

    // strcpy(payload, "0140:4902013144341:475030305235352:42313233343536");
    if (nb_rx_state == ELM_SUCCESS)
    {
        memset(vin, 0, 18);
        // **** Decoding ****
        if (strstr(payload, "490201"))
        {
            // OBD scanner provides this multiline response:
            // 014                        ==> 0x14 = 20 bytes following
            // 0: 49 02 01 31 44 34       ==> 49 02 = Header. 01 = 1 VIN number in message. 31, 44, 34 = First 3 VIN digits
            // 1: 47 50 30 30 52 35 35    ==> 47->35 next 7 VIN digits
            // 2: 42 31 32 33 34 35 36    ==> 42->36 next 7 VIN digits
            //
            // The resulitng payload buffer is:
            // "0140:4902013144341:475030305235352:42313233343536" ==> VIN="1D4GP00R55B123456" (17-digits)
            idx = strstr(payload, "490201") + 6; // Pointer to first ASCII code digit of first VIN digit
            // Loop over each pair of ASCII code digits. 17 VIN digits + 2 skipped line numbers = 19 loops
            for (int i = 0; i < (19 * 2); i += 2)
            {
                temp[0] = *(idx + i);     // Get first digit of ASCII code
                temp[1] = *(idx + i + 1); // Get second digit of ASCII code
                // No need to add string termination, temp[3] always == 0

                if (strstr(temp, ":"))
                    continue;                                  // Skip the second "1:" and third "2:" line numbers
                
                ascii_val = strtol(temp, 0, 16);               // Convert ASCII code to integer
                snprintf(vin + vin_counter++, sizeof(uint8_t), "%c", ascii_val); // Convert ASCII code integer back to character
                                                               // Serial.printf("Chars %s, ascii_val=%d[dec] 0x%02hhx[hex] ==> VIN=%s\n", temp, ascii_val, ascii_val, vin);
            }
        }
        if (debugMode)
        {
            Serial.print(F("VIN: "));
            Serial.println(vin);
        }
    }
    else
    {
        if (debugMode)
        {
            Serial.println(F("No VIN response"));
            printError();
        }
    }
    return nb_rx_state;
}

/*
 bool ELM327::resetDTC()

 Description:
 ------------
    * Resets the stored DTCs in the ECU. This is a blocking function.
      Note: The SAE spec requires that scan tools verify that a reset
            is intended ("Are you sure?") before sending the mode 04
            reset command to the vehicle. See p.32 of ELM327 datasheet.

 Inputs:
 -------
    * void

 Return:
 -------
  * bool - Indicates the success (or not) of the reset command.
*/
bool ELM327::resetDTC()
{
    if (sendCommand_Blocking("04") == ELM_SUCCESS)
    {
        if (strstr(payload, "44") != NULL)
        {
            if (debugMode)
                Serial.println(F("ELMduino: DTC successfully reset."));

            return true;
        }
    }
    else
    {
        if (debugMode)
            Serial.println(F("ELMduino: Resetting DTC codes failed."));
    }

    return false;
}

/*
 void ELM327::currentDTCCodes(const bool& isBlocking)

 Description:
 ------------
  * Get the list of current DTC codes. This method is blocking by default, but can be run
    in non-blocking mode if desired with optional boolean argument. Typical use involves
    calling the monitorStatus() function first to get the number of DTC current codes stored,
    then calling this function to retrieve those codes. This would  not typically
    be done in NB mode in a loop, but optional NB mode is supported.

  * To check the results of this query, inspect the DTC_Response struct: DTC_Response.codesFound
    will contain the number of codes present and DTC_Response.codes is an array
    of 5 char codes that were retrieved.

 Inputs:
 -------
  * bool isBlocking - optional arg to set (non)blocking mode - defaults to true / blocking mode

 Return:
 -------
  * void
*/
void ELM327::currentDTCCodes(const bool& isBlocking)
{
    char *idx;
    char codeType = '\0';
    char codeNumber[5] = {0};
    char temp[6] = {0};

    if (isBlocking) // In blocking mode, we loop here until get_response() is past ELM_GETTING_MSG state
    {
        sendCommand("03"); // Check DTC is always Service 03 with no PID
        while (get_response() == ELM_GETTING_MSG)
            ;
    }
    else
    {
        if (nb_query_state == SEND_COMMAND)
        {
            sendCommand("03");
            nb_query_state = WAITING_RESP;
        }

        else if (nb_query_state == WAITING_RESP)
            get_response();
    }

    if (nb_rx_state == ELM_SUCCESS)
    {
        nb_query_state = SEND_COMMAND; // Reset the query state machine for next command
        memset(DTC_Response.codes, 0, DTC_CODE_LEN * DTC_MAX_CODES);

        if (strstr(payload, "43") != NULL) // Successful response to Mode 03 request
        {
            // OBD scanner will provide a response that contains one or more lines indicating the codes present.
            // Each response line will start with "43" indicating it is a response to a Mode 03 request.
            // See p. 31 of ELM327 datasheet for details and lookup table of code types.

            uint8_t codesFound = strlen(payload) / 8; // Each code found returns 8 chars starting with "43"
            idx = strstr(payload, "43") + 4;          // Pointer to first DTC code digit (third char in the response)

            if (codesFound > DTC_MAX_CODES) // I don't think the ELM is capable of returning
            {                               // more than 0xF (16) codes, but just in case...
                codesFound = DTC_MAX_CODES;
                Serial.print(F("DTC response truncated at "));
                Serial.print(DTC_MAX_CODES);
                Serial.println(F(" codes."));
            }

            DTC_Response.codesFound = codesFound;

            for (int i = 0; i < codesFound; i++)
            {
                memset(temp, 0, sizeof(temp));
                memset(codeNumber, 0, sizeof(codeNumber));

                codeType = *idx;            // Get first digit of second byte
                codeNumber[0] = *(idx + 1); // Get second digit of second byte
                codeNumber[1] = *(idx + 2); // Get first digit of third byte
                codeNumber[2] = *(idx + 3); // Get second digit of third byte

                switch (codeType) // Set the correct type prefix for the code
                {
                case '0':
                    strcat(temp, "P0");
                    break;

                case '1':
                    strcat(temp, "P1");
                    break;

                case '2':
                    strcat(temp, "P2");
                    break;
                case '3':
                    strcat(temp, "P3");
                    break;

                case '4':
                    strcat(temp, "C0");
                    break;

                case '5':
                    strcat(temp, "C1");
                    break;

                case '6':
                    strcat(temp, "C2");
                    break;

                case '7':
                    strcat(temp, "C3");
                    break;

                case '8':
                    strcat(temp, "B0");
                    break;

                case '9':
                    strcat(temp, "B1");
                    break;

                case 'A':
                    strcat(temp, "B2");
                    break;

                case 'B':
                    strcat(temp, "B3");
                    break;

                case 'C':
                    strcat(temp, "U0");
                    break;

                case 'D':
                    strcat(temp, "U1");
                    break;

                case 'E':
                    strcat(temp, "U2");
                    break;

                case 'F':
                    strcat(temp, "U3");
                    break;

                default:
                    break;
                }

                strcat(temp, codeNumber);            // Append the code number to the prefix
                strcpy(DTC_Response.codes[i], temp); // Add the fully parsed code to the list (array)
                idx = idx + 8;                       // reset idx to start of next code

                if (debugMode)
                {
                    Serial.print(F("ELMduino: Found code: "));
                    Serial.println(temp);
                }
            }
        }
        else
        {
            if (debugMode)
            {
                Serial.println(F("ELMduino: DTC response received with no valid data."));
            }
        }
        return;
    }
    else if (nb_rx_state != ELM_GETTING_MSG)
    {
        nb_query_state = SEND_COMMAND; // Error or timeout, so reset the query state machine for next command

        if (debugMode)
        {
            Serial.println(F("ELMduino: Getting current DTC codes failed."));
            printError();
        }
    }
}

/*
 bool ELM327::isPidSupported(uint8_t pid)

 Description:
 ------------
  * Checks if a particular PID is supported by the connected ECU.

  * This is a convenience method that selects the correct supportedPIDS_xx_xx() query and parses
    the bit-encoded result, returning a simple Boolean value indicating PID support from the ECU.

 Inputs:
 -------
  * uint8_t pid - the PID to check for support.

 Return:
 -------
  * bool - Whether or not the queried PID is supported by the ECU.
*/
bool ELM327::isPidSupported(uint8_t pid)
{
    uint8_t pidInterval = (pid / PID_INTERVAL_OFFSET) * PID_INTERVAL_OFFSET;

    switch (pidInterval)
    {
    case SUPPORTED_PIDS_1_20:
        supportedPIDs_1_20();
        break;

    case SUPPORTED_PIDS_21_40:
        supportedPIDs_21_40();
        pid = (pid - SUPPORTED_PIDS_21_40);
        break;

    case SUPPORTED_PIDS_41_60:
        supportedPIDs_41_60();
        pid = (pid - SUPPORTED_PIDS_41_60);
        break;

    case SUPPORTED_PIDS_61_80:
        supportedPIDs_61_80();
        pid = (pid - SUPPORTED_PIDS_61_80);
        break;

    default:
        break;
    }

    if (nb_rx_state == ELM_SUCCESS)
    {
        return ((response >> (32 - pid)) & 0x1);
    }
    return false;
}

double ELM327::calculator_0C() {
    return (double)((response_A << 8) | response_B)/4;
}

double ELM327::calculator_10() {
    return (double)((response_A << 8) | response_B)/100;
}

double ELM327::calculator_14(){
    return (double)(response_A/200) ;
}

double ELM327::calculator_1F() {
    return (double)((response_A << 8) | response_B);
}

double ELM327::calculator_22() {
    return (double) ((response_A << 8) | response_B) * 0.079;
}

double ELM327::calculator_23() {
    return (double) ((response_A << 8) | response_B) * 10;
}

double ELM327::calculator_32()
{
    return (double) ((int16_t)((response_A << 8) | response_B)) / 4.0;
}

double ELM327::calculator_3C() {
    return (double) (((response_A << 8) | response_B) / 10) - 40;
}

double ELM327::calculator_42() {
    return (double) ((response_A << 8) | response_B) / 1000;
}

double ELM327::calculator_43() {
    return (double) ((response_A << 8) | response_B) * (100.0 / 255.0);
}

double ELM327::calculator_44() {
    return ((double) ((response_A << 8) | response_B) * 2.0) / 65536.0;
}

double ELM327::calculator_4F() {
    return (double) (response_A);
}

double ELM327::calculator_50() {
    return (double) (response_A * 10.0);
}

double ELM327::calculator_53() {
    return (double) ((response_A << 8) | response_B) / 200;
}

double ELM327::calculator_54() {
    return (double) ((int16_t)((response_A << 8) | response_B));
}

double ELM327::calculator_55() {
    return ((double) response_A * (100.0 / 128.0)) - 100.0;
}

//calc 23
double ELM327::calculator_59() {
    return (double) ((response_A << 8) | response_B) * 10;
}

double ELM327::calculator_5D() {
    return (double) (((response_A << 8) | response_B) / 128) - 210;
} 

double ELM327::calculator_5E() {
    return (double) ((response_A << 8) | response_B) / 20;
}

double ELM327::calculator_61() {
    return (double) response_A  - 125;
}