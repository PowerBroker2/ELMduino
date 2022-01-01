#include <ELMduino.h>
#include <M5Unified.h>

#include "BluetoothSerial.h"

ELM327 myELM327;
BluetoothSerial SerialBT;

char txt[80] = "";

// #define USE_MAC_NAME // Comment this line out to specify OBDII scanner bluetooth mac address. Uncomment to specify bluetooth name
#if defined USE_MAC_NAME
char bt_name[] = "OBDLink LX";
// char bt_name[] = "OBDII";
// char bt_name[] = "V-LINK";
#else
// char bluetooth_mac_str[] = "00:1D:A5:68:98:8C"; // EM327 mini clone Blue
// char bluetooth_mac_str[] = "00:1D:A5:22:69:9F"; // VGate iCar Pro black and gold
char bluetooth_mac_str[] = "00:1d:a5:22:5f:f6";  // VGate iCar Pro black and gold
// char bluetooth_mac_str[] = "00:04:3E:53:5E:0D";
// char bluetooth_mac_str[] = "00:04:3E:53:5E:0D"; // OBDLink LX green
uint8_t bt_mac[6] = {0x00};
#endif
char pin[] = "1234";

typedef enum { IDLE,
               BARO_PRESS,
               COOLANT_TEMP,
               OIL_TEMP,
               MANIFOLD_PRESS,
               ENG_RPM,
               BATT_VOLT } obd_pid_states;

obd_pid_states obd_state = IDLE;
float coolant_temperature = 0.0;
float battery_voltage = 0.0;
float eng_rpm = 0.0;
uint32_t slow_data_last_time = millis();
uint32_t fast_data_last_time = millis();

void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;  // default=115200. if "Serial" is not needed, set it to 0.
  cfg.clear_display = true;      // default=true. clear the screen when begin.
  cfg.output_power = false;      // default=true. use external port 5V output.
  cfg.internal_imu = false;      // default=true. use internal IMU.
  cfg.internal_rtc = true;       // default=true. use internal RTC.
  cfg.external_imu = false;      // default=false. use Unit Accel & Gyro.
  cfg.external_rtc = false;      // default=false. use Unit RTC.
  cfg.led_brightness = 0;        // default= 0. Green LED brightness (0=off / 255=max) (※ not NeoPixel)

  M5.begin(cfg);
  M5.Lcd.setBrightness((70 * 255) / 100);  // Set LCD brightness to 70% of maximum

  M5.Lcd.setFont(&fonts::FreeSans12pt7b);
  M5.Lcd.setTextDatum(top_left);
  M5.Lcd.setTextPadding(0);
  M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);

  uint16_t xx = 0;
  uint16_t yy = 10;
  M5.Lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
  sprintf(txt, "%s", "Starting ESP32 BT");
  Serial.println(txt);
  M5.Lcd.drawString(txt, xx, yy);

  // Start ESP32 bluetooth
  SerialBT.begin("ELM_NonBlock", true);

  M5.Lcd.setTextPadding(M5.Lcd.textWidth(txt) + 10);
  M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  sprintf(txt, "%s", "BT Started");
  Serial.println(txt);
  M5.Lcd.drawString(txt, xx, yy);

  yy += M5.Lcd.fontHeight();
  M5.Lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
  M5.Lcd.setFont(&fonts::FreeSans9pt7b);
  sprintf(txt, "%s", "Trying to connect OBDII scanner");
  Serial.println(txt);
  M5.Lcd.drawString(txt, xx, yy);

  yy += M5.Lcd.fontHeight();
  M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  bool bt_connected = false;
// Attempt to connect ESP32 to bluetooth OBDII scanner
// SerialBT.setPin(pin);
#if defined USE_MAC_NAME
  sprintf(txt, "%s = %s", "BT Name", bt_name);
  Serial.println(txt);
  M5.Lcd.drawString(txt, xx, yy);
  bt_connected = SerialBT.connect(bt_name);
#else
  // Convert bluetooth MAC address string into integer array[6]
  sscanf(bluetooth_mac_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
         bt_mac, bt_mac + 1, bt_mac + 2,
         bt_mac + 3, bt_mac + 4, bt_mac + 5);
  sprintf(txt, "%s = %s", "MAC address", bluetooth_mac_str);
  Serial.println(txt);
  M5.Lcd.drawString(txt, xx, yy);
  bt_connected = SerialBT.connect(bt_mac);
#endif

  yy += M5.Lcd.fontHeight();

  if (!bt_connected) {
    sprintf(txt, "%s", "Can't connect OBD - Phase 1");
    Serial.println(txt);
    M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
    M5.Lcd.drawString(txt, xx, yy);
    while (1)
      ;
  }

  M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  sprintf(txt, "%s", "Phase 1 ok");
  Serial.println(txt);
  M5.Lcd.drawString(txt, xx, yy);

  yy += M5.Lcd.fontHeight();
  M5.Lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
  sprintf(txt, "%s", "Trying to start ELM327");
  Serial.println(txt);
  M5.Lcd.drawString(txt, xx, yy);

  bt_connected = myELM327.begin(SerialBT, true, 2000);

  yy += M5.Lcd.fontHeight();

  if (!bt_connected) {
    sprintf(txt, "%s", "Can't connect OBD - Phase 2");
    Serial.println(txt);
    M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
    M5.Lcd.drawString(txt, xx, yy);

    while (1)
      ;
  }

  M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  sprintf(txt, "%s", "Phase 2 ok");
  Serial.println(txt);
  M5.Lcd.drawString(txt, xx, yy);

  delay(2000);

  M5.Lcd.clear();

  // Label for Coolant Temperature
  xx = 0;
  M5.Lcd.setFont(&fonts::FreeSans12pt7b);
  M5.Lcd.setTextDatum(top_left);
  M5.Lcd.setTextPadding(100);
  M5.Lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
  M5.Lcd.drawString("Coolant Temp:", xx, 10);

  // Label for RPM
  M5.Lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
  M5.Lcd.drawString("RPM:", xx, 50);

  // Label for battery voltage
  M5.Lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
  M5.Lcd.drawString("Battery:", xx, 90);

}

void loop() {
  uint16_t xx = 180;
  uint16_t yy = 10;

  switch (obd_state) {
    case IDLE:
      if (millis() > slow_data_last_time + 5000) {
        slow_data_last_time = millis();
        obd_state = BATT_VOLT;
        Serial.printf("IDLE, query SLOW PIDs at %.3f s\n", slow_data_last_time / 1000.0);
      } else if (millis() > fast_data_last_time + 1000) {
        fast_data_last_time = millis();
        obd_state = COOLANT_TEMP;
        Serial.printf("IDLE, query FAST PIDs at %.3f s\n", fast_data_last_time / 1000.0);
      } else {
        // Serial.printf("[%.3f s] IDLE\n", millis() / 1000.0);
      }
      break;

    case BARO_PRESS:
      break;

    case COOLANT_TEMP:
      coolant_temperature = myELM327.engineCoolantTemp();
      yy = 10;
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        // Display coolant temperature
        M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
        sprintf(txt, "%.0f deg", coolant_temperature);
        M5.Lcd.drawString(txt, xx, yy);
        Serial.printf("Coolant temp = %s\n", txt);
        obd_state = ENG_RPM;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
        M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
        sprintf(txt, "%s", "N/A deg");
        M5.Lcd.drawString(txt, xx, yy);
        obd_state = ENG_RPM;
      }
      break;

    case OIL_TEMP:
      break;

    case MANIFOLD_PRESS:
      break;

    case ENG_RPM:
      eng_rpm = myELM327.rpm();
      yy = 50;
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        // Display coolant temperature
        M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
        sprintf(txt, "%.0f RPM", eng_rpm);
        M5.Lcd.drawString(txt, xx, yy);
        Serial.printf("Engine RPM = %s\n", txt);
        obd_state = IDLE;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
        M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
        sprintf(txt, "%s", "N/A RPM");
        M5.Lcd.drawString(txt, xx, yy);
        obd_state = IDLE;
      }
      break;

    case BATT_VOLT:
      battery_voltage = myELM327.batteryVoltage();
      yy = 90;
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        // Display vehicle battery voltage
        M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
        sprintf(txt, "%.1f VDC", battery_voltage);
        M5.Lcd.drawString(txt, xx, yy);
        Serial.printf("Battery voltage = %s\n", txt);

        obd_state = IDLE;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
        M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
        sprintf(txt, "%s", "N/A VDC");
        M5.Lcd.drawString(txt, xx, yy);
        obd_state = IDLE;
      }
      break;
  }
}
