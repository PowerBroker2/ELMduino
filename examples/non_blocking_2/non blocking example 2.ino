#include <ELMduino.h>
#include "BluetoothSerial.h"

ELM327 myELM327;
BluetoothSerial SerialBT;

char txt[80] = "";

// #define USE_MAC_NAME // Comment this line out to specify OBDII scanner bluetooth mac address. Uncomment to specify bluetooth name
#if defined USE_MAC_NAME
char bt_name[] = "OBDII";
#else
char bluetooth_mac_str[] = "00:1d:a5:22:5f:f6";  // VGate iCar Pro black and gold
uint8_t bt_mac[6] = {0x00};
#endif
char pin[] = "1234";

typedef enum { IDLE,
               COOLANT_TEMP,
               OIL_TEMP,
               ENG_RPM,
               BATT_VOLT } obd_pid_states;

obd_pid_states obd_state = IDLE;
float coolant_temperature = 0.0;
float battery_voltage = 0.0;
float eng_rpm = 0.0;
uint32_t slow_data_last_time = millis();
uint32_t fast_data_last_time = millis();

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting ESP32 BT"));

  // Start ESP32 bluetooth
  SerialBT.begin("ELM_NonBlock", true);

  Serial.println(F("BT Started"));
  Serial.println(F("Trying to connect OBDII scanner"));

  bool bt_connected = false;
// Attempt to connect ESP32 to bluetooth OBDII scanner
// SerialBT.setPin(pin);
#if defined USE_MAC_NAME
  Serial.printf("%s = %s\n", "BT Name", bt_name);
  bt_connected = SerialBT.connect(bt_name);
#else
  // Convert bluetooth MAC address string into integer array[6]
  sscanf(bluetooth_mac_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
         bt_mac, bt_mac + 1, bt_mac + 2,
         bt_mac + 3, bt_mac + 4, bt_mac + 5);
  Serial.printf("%s = %s\n", "MAC address", bluetooth_mac_str);
  bt_connected = SerialBT.connect(bt_mac);
#endif

  if (!bt_connected) {
    Serial.println(F("Couldn't connect to OBD scanner - Phase 1"));
    while (1);
  }

  Serial.println(F("Phase 1 ok. Trying to start ELM327"));
  bt_connected = myELM327.begin(SerialBT, true, 2000);

  if (!bt_connected) {
    Serial.println("Couldn't connect to OBD scanner - Phase 2");
    while (1);
  }

  Serial.println(F("Phase 2 ok"));
}

void loop() {
  switch (obd_state) {
    case IDLE:
      // Schedule PIDs to be read at different rates (slow PIDs and fast PIDs)
      if (millis() > slow_data_last_time + 5000) {
        slow_data_last_time = millis();
        obd_state = BATT_VOLT;
        Serial.printf("IDLE, query SLOW PIDs at %.3f s\n", slow_data_last_time / 1000.0);
      } else if (millis() > fast_data_last_time + 1000) {
        fast_data_last_time = millis();
        obd_state = COOLANT_TEMP;  // Specify the first slow PID here
        Serial.printf("IDLE, query FAST PIDs at %.3f s\n", fast_data_last_time / 1000.0);
      } else {
        // Serial.printf("[%.3f s] IDLE\n", millis() / 1000.0);
      }
      break;

    case COOLANT_TEMP:
      coolant_temperature = myELM327.engineCoolantTemp();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        Serial.printf("Coolant temp = %.0f\n", coolant_temperature);
        obd_state = OIL_TEMP;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
        obd_state = OIL_TEMP;
      }
      break;

    case OIL_TEMP:
      // Not yet implemented
      obd_state = ENG_RPM;
      break;

    case ENG_RPM:
      eng_rpm = myELM327.rpm();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        Serial.printf("Engine RPM = %.0f\n", eng_rpm);
        obd_state = IDLE;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
        obd_state = IDLE;
      }
      break;

    case BATT_VOLT:
      battery_voltage = myELM327.batteryVoltage();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        Serial.printf("Battery voltage = %.1f\n", battery_voltage);
        obd_state = IDLE;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
        obd_state = IDLE;
      }
      break;
  }
}
