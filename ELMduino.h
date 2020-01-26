#pragma once
#include "Arduino.h"




//-------------------------------------------------------------------------------------//
// PIDs
//-------------------------------------------------------------------------------------//
const uint8_t SERVICE_01                       = 1;
	

const uint8_t SUPPORTED_PIDS_1_20              = 0;   // 0x00 - bit encoded
const uint8_t MONITOR_STATUS_SINCE_DTC_CLEARED = 1;   // 0x01 - bit encoded
const uint8_t FREEZE_DTC                       = 2;   // 0x02 -
const uint8_t FUEL_SYSTEM_STATUS               = 3;   // 0x03 - bit encoded
const uint8_t ENGINE_LOAD                      = 4;   // 0x04 - %
const uint8_t ENGINE_COOLANT_TEMP              = 5;   // 0x05 - °C
const uint8_t SHORT_TERM_FUEL_TRIM_BANK_1      = 6;   // 0x06 - %
const uint8_t LONG_TERM_FUEL_TRIM_BANK_1       = 7;   // 0x07 - %
const uint8_t SHORT_TERM_FUEL_TRIM_BANK_2      = 8;   // 0x08 - %
const uint8_t LONG_TERM_FUEL_TRIM_BANK_2       = 9;   // 0x09 - %
const uint8_t FUEL_PRESSURE                    = 10;  // 0x0A - kPa
const uint8_t INTAKE_MANIFOLD_ABS_PRESSURE     = 11;  // 0x0B - kPa
const uint8_t ENGINE_RPM                       = 12;  // 0x0C - rpm
const uint8_t VEHICLE_SPEED                    = 13;  // 0x0D - km/h
const uint8_t TIMING_ADVANCE                   = 14;  // 0x0E - ° before TDC
const uint8_t INTAKE_AIR_TEMP                  = 15;  // 0x0F - °C
const uint8_t MAF_FLOW_RATE                    = 16;  // 0x10 - g/s
const uint8_t THROTTLE_POSITION                = 17;  // 0x11 - %
const uint8_t COMMANDED_SECONDARY_AIR_STATUS   = 18;  // 0x12 - bit encoded
const uint8_t OXYGEN_SENSORS_PRESENT_2_BANKS   = 19;  // 0x13 - bit encoded
const uint8_t OXYGEN_SENSOR_1_A                = 20;  // 0x14 - V %
const uint8_t OXYGEN_SENSOR_2_A                = 21;  // 0x15 - V %
const uint8_t OXYGEN_SENSOR_3_A                = 22;  // 0x16 - V %
const uint8_t OXYGEN_SENSOR_4_A                = 23;  // 0x17 - V %
const uint8_t OXYGEN_SENSOR_5_A                = 24;  // 0x18 - V %
const uint8_t OXYGEN_SENSOR_6_A                = 25;  // 0x19 - V %
const uint8_t OXYGEN_SENSOR_7_A                = 26;  // 0x1A - V %
const uint8_t OXYGEN_SENSOR_8_A                = 27;  // 0x1B - V %
const uint8_t OBD_STANDARDS                    = 28;  // 0x1C - bit encoded
const uint8_t OXYGEN_SENSORS_PRESENT_4_BANKS   = 29;  // 0x1D - bit encoded
const uint8_t AUX_INPUT_STATUS                 = 30;  // 0x1E - bit encoded
const uint8_t RUN_TIME_SINCE_ENGINE_START      = 31;  // 0x1F - sec

const uint8_t SUPPORTED_PIDS_21_40             = 32;  // 0x20 - bit encoded
const uint8_t DISTANCE_TRAVELED_WITH_MIL_ON    = 33;  // 0x21 - km
const uint8_t FUEL_RAIL_PRESSURE               = 34;  // 0x22 - kPa
const uint8_t FUEL_RAIL_GUAGE_PRESSURE         = 35;  // 0x23 - kPa
const uint8_t OXYGEN_SENSOR_1_B                = 36;  // 0x24 - ratio V
const uint8_t OXYGEN_SENSOR_2_B                = 37;  // 0x25 - ratio V
const uint8_t OXYGEN_SENSOR_3_B                = 38;  // 0x26 - ratio V
const uint8_t OXYGEN_SENSOR_4_B                = 39;  // 0x27 - ratio V
const uint8_t OXYGEN_SENSOR_5_B                = 40;  // 0x28 - ratio V
const uint8_t OXYGEN_SENSOR_6_B                = 41;  // 0x29 - ratio V
const uint8_t OXYGEN_SENSOR_7_B                = 42;  // 0x2A - ratio V
const uint8_t OXYGEN_SENSOR_8_B                = 43;  // 0x2B - ratio V
const uint8_t COMMANDED_EGR                    = 44;  // 0x2C - %
const uint8_t EGR_ERROR                        = 45;  // 0x2D - %
const uint8_t COMMANDED_EVAPORATIVE_PURGE      = 46;  // 0x2E - %
const uint8_t FUEL_TANK_LEVEL_INPUT            = 47;  // 0x2F - %
const uint8_t WARM_UPS_SINCE_CODES_CLEARED     = 48;  // 0x30 - count
const uint8_t DIST_TRAV_SINCE_CODES_CLEARED    = 49;  // 0x31 - km
const uint8_t EVAP_SYSTEM_VAPOR_PRESSURE       = 50;  // 0x32 - Pa
const uint8_t ABS_BAROMETRIC_PRESSURE          = 51;  // 0x33 - kPa
const uint8_t OXYGEN_SENSOR_1_C                = 52;  // 0x34 - ratio mA
const uint8_t OXYGEN_SENSOR_2_C                = 53;  // 0x35 - ratio mA
const uint8_t OXYGEN_SENSOR_3_C                = 54;  // 0x36 - ratio mA
const uint8_t OXYGEN_SENSOR_4_C                = 55;  // 0x37 - ratio mA
const uint8_t OXYGEN_SENSOR_5_C                = 56;  // 0x38 - ratio mA
const uint8_t OXYGEN_SENSOR_6_C                = 57;  // 0x39 - ratio mA
const uint8_t OXYGEN_SENSOR_7_C                = 58;  // 0x3A - ratio mA
const uint8_t OXYGEN_SENSOR_8_C                = 59;  // 0x3B - ratio mA
const uint8_t CATALYST_TEMP_BANK_1_SENSOR_1    = 60;  // 0x3C - °C
const uint8_t CATALYST_TEMP_BANK_2_SENSOR_1    = 61;  // 0x3D - °C
const uint8_t CATALYST_TEMP_BANK_1_SENSOR_2    = 62;  // 0x3E - °C
const uint8_t CATALYST_TEMP_BANK_2_SENSOR_2    = 63;  // 0x3F - °C

const uint8_t SUPPORTED_PIDS_41_60             = 64;  // 0x40 - bit encoded
const uint8_t MONITOR_STATUS_THIS_DRIVE_CYCLE  = 65;  // 0x41 - bit encoded
const uint8_t CONTROL_MODULE_VOLTAGE           = 66;  // 0x42 - V
const uint8_t ABS_LOAD_VALUE                   = 67;  // 0x43 - %
const uint8_t FUEL_AIR_COMMANDED_EQUIV_RATIO   = 68;  // 0x44 - ratio
const uint8_t RELATIVE_THROTTLE_POSITION       = 69;  // 0x45 - %
const uint8_t AMBIENT_AIR_TEMP                 = 70;  // 0x46 - °C
const uint8_t ABS_THROTTLE_POSITION_B          = 71;  // 0x47 - %
const uint8_t ABS_THROTTLE_POSITION_C          = 72;  // 0x48 - %
const uint8_t ACCELERATOR_PEDAL_POSITION_D     = 73;  // 0x49 - %
const uint8_t ACCELERATOR_PEDAL_POSITION_E     = 74;  // 0x4A - %
const uint8_t ACCELERATOR_PEDAL_POSITION_F     = 75;  // 0x4B - %
const uint8_t COMMANDED_THROTTLE_ACTUATOR      = 76;  // 0x4C - %
const uint8_t TIME_RUN_WITH_MIL_ON             = 77;  // 0x4D - min
const uint8_t TIME_SINCE_CODES_CLEARED         = 78;  // 0x4E - min
const uint8_t MAX_VALUES_EQUIV_V_I_PRESSURE    = 79;  // 0x4F - ratio V mA kPa
const uint8_t MAX_MAF_RATE                     = 80;  // 0x50 - g/s
const uint8_t FUEL_TYPE                        = 81;  // 0x51 - ref table
const uint8_t ETHONOL_FUEL_PERCENT             = 82;  // 0x52 - %
const uint8_t ABS_EVAP_SYS_VAPOR_PRESSURE      = 83;  // 0x53 - kPa
const uint8_t EVAP_SYS_VAPOR_PRESSURE          = 84;  // 0x54 - Pa
const uint8_t SHORT_TERM_SEC_OXY_SENS_TRIM_1_3 = 85;  // 0x55 - %
const uint8_t LONG_TERM_SEC_OXY_SENS_TRIM_1_3  = 86;  // 0x56 - %
const uint8_t SHORT_TERM_SEC_OXY_SENS_TRIM_2_4 = 87;  // 0x57 - %
const uint8_t LONG_TERM_SEC_OXY_SENS_TRIM_2_4  = 88;  // 0x58 - %
const uint8_t FUEL_RAIL_ABS_PRESSURE           = 89;  // 0x59 - kPa
const uint8_t RELATIVE_ACCELERATOR_PEDAL_POS   = 90;  // 0x5A - %
const uint8_t HYBRID_BATTERY_REMAINING_LIFE    = 91;  // 0x5B - %
const uint8_t ENGINE_OIL_TEMP                  = 92;  // 0x5C - °C
const uint8_t FUEL_INJECTION_TIMING            = 93;  // 0x5D - °
const uint8_t ENGINE_FUEL_RATE                 = 94;  // 0x5E - L/h
const uint8_t EMISSION_REQUIREMENTS            = 95;  // 0x5F - bit encoded

const uint8_t SUPPORTED_PIDS_61_80             = 96;  // 0x60 - bit encoded
const uint8_t DEMANDED_ENGINE_PERCENT_TORQUE   = 97;  // 0x61 - %
const uint8_t ACTUAL_ENGINE_TORQUE             = 98;  // 0x62 - %
const uint8_t ENGINE_REFERENCE_TORQUE          = 99;  // 0x63 - Nm
const uint8_t ENGINE_PERCENT_TORQUE_DATA       = 100; // 0x64 - %
const uint8_t AUX_INPUT_OUTPUT_SUPPORTED       = 101; // 0x65 - bit encoded




//-------------------------------------------------------------------------------------//
// Class constants
//-------------------------------------------------------------------------------------//
const char   CANCEL_OBD[]          = "XXXXXXXXX\r\r\r";
const float  KPH_MPH_CONVERT       = 0.6213711922;
const float  RPM_CONVERT           = 0.25;
const int8_t QUERY_LEN	           = 6;
const int8_t HEADER_LEN            = 4;
const int8_t SERVICE_LEN           = 2;
const int8_t PID_LEN               = 2;
const int8_t PAYLOAD_LEN           = 40;
const int8_t ELM_SUCCESS           = 0;
const int8_t ELM_NO_RESPONSE       = 1;
const int8_t ELM_BUFFER_OVERFLOW   = 2;
const int8_t ELM_GARBAGE           = 3;
const int8_t ELM_UNABLE_TO_CONNECT = 4;
const int8_t ELM_NO_DATA           = 5;
const int8_t ELM_GENERAL_ERROR     = -1;




class ELM327
{
public:
	Stream* elm_port;

	bool connected = false;
	int8_t status = ELM_GENERAL_ERROR;

	


	bool begin(Stream& stream);
	bool initializeELM();
	bool queryPID(uint16_t service, uint16_t pid);
	int8_t sendCommand(const char *cmd);
	bool timeout();
	float rpm();
	int32_t kph();
	float mph();
	



private:
	char payload[PAYLOAD_LEN] = { 0 };
	char query[QUERY_LEN];
	uint8_t hexService[SERVICE_LEN];
	uint8_t hexPid[PID_LEN];
	uint8_t responseHeader[HEADER_LEN];
	uint16_t timeout_ms = 150;
	uint32_t currentTime;
	uint32_t previousTime;




	void upper(char string[], uint8_t buflen);
	void formatQueryArray(uint16_t service, uint16_t pid);
	void formatHeaderArray();
	uint8_t ctoi(uint8_t value);
	void flushInputBuff();
	int findResponse(bool longResponse);
};
