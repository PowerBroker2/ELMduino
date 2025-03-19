#include "BluetoothSerial.h"
#include "ELMduino.h"

BluetoothSerial SerialBT;
#define ELM_PORT SerialBT
#define DEBUG_PORT Serial

ELM327 myELM327;
uint8_t current_pid = 0;
int nb_query_state = SEND_COMMAND; 

// Define a list of PIDs to test
const uint16_t pidsToTest[] = {
  SUPPORTED_PIDS_1_20,                    // 0x00   
  MONITOR_STATUS_SINCE_DTC_CLEARED,       // 0x01
  FREEZE_DTC,                             // 0x02
  FUEL_SYSTEM_STATUS,                     // 0x03
  ENGINE_LOAD,                            // 0x04
  ENGINE_COOLANT_TEMP,                    // 0x05
  SHORT_TERM_FUEL_TRIM_BANK_1,            // 0x06
  LONG_TERM_FUEL_TRIM_BANK_1,             // 0x07
  SHORT_TERM_FUEL_TRIM_BANK_2,            // 0x08
  LONG_TERM_FUEL_TRIM_BANK_2,             // 0x09
  FUEL_PRESSURE,                          // 0x0A
  INTAKE_MANIFOLD_ABS_PRESSURE,           // 0x0B
  ENGINE_RPM,                             // 0x0C
  VEHICLE_SPEED,                          // 0x0D
  TIMING_ADVANCE,                         // 0x0E
  INTAKE_AIR_TEMP,                        // 0x0F
  MAF_FLOW_RATE,                          // 0x10
  THROTTLE_POSITION,                      // 0x11
  COMMANDED_SECONDARY_AIR_STATUS,         // 0x12
  OXYGEN_SENSORS_PRESENT_2_BANKS,         // 0x13
  OXYGEN_SENSOR_1_A,                      // 0x14
  OXYGEN_SENSOR_2_A,                      // 0x15
  OXYGEN_SENSOR_3_A,                      // 0x16
  OXYGEN_SENSOR_4_A,                      // 0x17
  OXYGEN_SENSOR_5_A,                      // 0x18
  OXYGEN_SENSOR_6_A,                      // 0x19
  OXYGEN_SENSOR_7_A,                      // 0x1A
  OXYGEN_SENSOR_8_A,                      // 0x1B
  OBD_STANDARDS,                          // 0x1C
  OXYGEN_SENSORS_PRESENT_4_BANKS,         // 0x1D
  AUX_INPUT_STATUS,                       // 0x1E
  RUN_TIME_SINCE_ENGINE_START,            // 0x1F
  SUPPORTED_PIDS_21_40,                   // 0x20
  DISTANCE_TRAVELED_WITH_MIL_ON,          // 0x21
  FUEL_RAIL_PRESSURE,                     // 0x22
  FUEL_RAIL_GUAGE_PRESSURE,               // 0x23
  OXYGEN_SENSOR_1_B,                      // 0x24
  OXYGEN_SENSOR_2_B,                      // 0x25
  OXYGEN_SENSOR_3_B,                      // 0x26
  OXYGEN_SENSOR_4_B,                      // 0x27
  OXYGEN_SENSOR_5_B,                      // 0x28
  OXYGEN_SENSOR_6_B,                      // 0x29
  OXYGEN_SENSOR_7_B,                      // 0x2A
  OXYGEN_SENSOR_8_B,                      // 0x2B
  COMMANDED_EGR,                          // 0x2C
  EGR_ERROR,                              // 0x2D
  COMMANDED_EVAPORATIVE_PURGE,            // 0x2E
  FUEL_TANK_LEVEL_INPUT,                  // 0x2F
  WARM_UPS_SINCE_CODES_CLEARED,           // 0x30 - count
  DIST_TRAV_SINCE_CODES_CLEARED,          // 0x31 - km
  EVAP_SYSTEM_VAPOR_PRESSURE,             // 0x32 - Pa
  ABS_BAROMETRIC_PRESSURE,                // 0x33 - kPa
  OXYGEN_SENSOR_1_C,                      // 0x34 - ratio mA
  OXYGEN_SENSOR_2_C,                      // 0x35 - ratio mA
  OXYGEN_SENSOR_3_C,                      // 0x36 - ratio mA
  OXYGEN_SENSOR_4_C,                      // 0x37 - ratio mA
  OXYGEN_SENSOR_5_C,                      // 0x38 - ratio mA
  OXYGEN_SENSOR_6_C,                      // 0x39 - ratio mA
  OXYGEN_SENSOR_7_C,                      // 0x3A - ratio mA
  OXYGEN_SENSOR_8_C,                      // 0x3B - ratio mA
  CATALYST_TEMP_BANK_1_SENSOR_1,          // 0x3C - °C
  CATALYST_TEMP_BANK_2_SENSOR_1,          // 0x3D - °C
  CATALYST_TEMP_BANK_1_SENSOR_2,          // 0x3E - °C
  CATALYST_TEMP_BANK_2_SENSOR_2,          // 0x3F - °C
  SUPPORTED_PIDS_41_60,                   // 0x40 - bit encoded
  MONITOR_STATUS_THIS_DRIVE_CYCLE,        // 0x41 - bit encoded
  CONTROL_MODULE_VOLTAGE,                 // 0x42 - V
  ABS_LOAD_VALUE,                         // 0x43 - %
  FUEL_AIR_COMMANDED_EQUIV_RATIO,         // 0x44 - ratio
  RELATIVE_THROTTLE_POSITION,             // 0x45 - %
  AMBIENT_AIR_TEMP,                       // 0x46 - °C
  ABS_THROTTLE_POSITION_B,                // 0x47 - %
  ABS_THROTTLE_POSITION_C,                // 0x48 - %
  ABS_THROTTLE_POSITION_D,                // 0x49 - %
  ABS_THROTTLE_POSITION_E,                // 0x4A - %
  ABS_THROTTLE_POSITION_F,                // 0x4B - %
  COMMANDED_THROTTLE_ACTUATOR,            // 0x4C - %
  TIME_RUN_WITH_MIL_ON,                   // 0x4D - min
  TIME_SINCE_CODES_CLEARED,               // 0x4E - min
  MAX_VALUES_EQUIV_V_I_PRESSURE,          // 0x4F - ratio V mA kPa
  MAX_MAF_RATE,                           // 0x50 - g/s
  FUEL_TYPE,                              // 0x51
  ETHANOL_FUEL_PERCENT,                   // 0x52
  ABS_EVAP_SYS_VAPOR_PRESSURE,            // 0x53
  EVAP_SYS_VAPOR_PRESSURE,                // 0x54
  SHORT_TERM_SEC_OXY_SENS_TRIM_1_3,       // 0x55
  LONG_TERM_SEC_OXY_SENS_TRIM_1_3,        // 0x56
  SHORT_TERM_SEC_OXY_SENS_TRIM_2_4,       // 0x57
  LONG_TERM_SEC_OXY_SENS_TRIM_2_4,        // 0x58
  FUEL_RAIL_ABS_PRESSURE,                 // 0x59
  RELATIVE_ACCELERATOR_PEDAL_POS,         // 0x5A
  HYBRID_BATTERY_REMAINING_LIFE,          // 0x5B
  ENGINE_OIL_TEMP,                        // 0x5C
  FUEL_INJECTION_TIMING,                  // 0x5D
  ENGINE_FUEL_RATE,                       // 0x5E
  EMISSION_REQUIREMENTS,                  // 0x5F
  SUPPORTED_PIDS_61_80,                   // 0x60
  DEMANDED_ENGINE_PERCENT_TORQUE,         // 0x61
  ACTUAL_ENGINE_TORQUE,                   // 0x62
  ENGINE_REFERENCE_TORQUE,                // 0x63
  ENGINE_PERCENT_TORQUE_DATA             // 0x64
};
const uint8_t responseBytes[0xA9] =
{
    4, 4, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2,
    4, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2,
    4, 4, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 4, 4, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2, 1,
    4, 1, 1, 2, 5, 2, 5, 3, 3, 7, 5, 5, 5, 11, 9, 3, 10, 6, 5, 5, 5, 7, 7, 5, 9, 9, 7, 7, 9, 1, 1, 13,  
    4, 41, 41, 9, 1, 10, 5, 5, 13, 41, 41, 7, 17, 1, 1, 7, 3, 5, 2, 3, 12, 9, 9, 6, 4, 17, 4, 2, 9
};

void setup()
{
    DEBUG_PORT.begin(115200);
    // SerialBT.setPin("1234");
    ELM_PORT.begin("ArduHUD", true);

    if (!ELM_PORT.connect("OBDII"))
    {
        DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 1");
        while (1)
            ;
    }

    if (!myELM327.begin(ELM_PORT, true, 2000))
    {
        DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 2");
        while (1)
            ;
    }

    DEBUG_PORT.println("Connected to ELM327");
}



void loop() {
    if (current_pid > (sizeof(pidsToTest)/sizeof(uint16_t)) - 1)
    {
        current_pid = 0;
    }
    uint16_t pid = pidsToTest[current_pid];
    double result = myELM327.processPID(0x01, pid, 1, responseBytes[current_pid], 1, 0);
    
    if (myELM327.nb_rx_state == ELM_SUCCESS)    // Our response is fully received, let's get our data
    {      
        nb_query_state = SEND_COMMAND;
        DEBUG_PORT.print("Result: ");
        DEBUG_PORT.println(result); 
        current_pid++;                          // Reset the query state for the next command
        delay(500);                             // Wait 0.5 seconds until we query again
    }
    
    else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
    {                                           // If state == ELM_GETTING_MSG, response is not yet complete. Restart the loop.
        nb_query_state = SEND_COMMAND;          // Reset the query state for the next command
        myELM327.printError();
        delay(500);                            // Wait 0.5 seconds until we query again
    }
  }
