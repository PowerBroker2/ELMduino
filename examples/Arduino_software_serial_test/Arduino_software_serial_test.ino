#include <SoftwareSerial.h>
#include "ELMduino.h"


SoftwareSerial mySerial(2, 3); // RX, TX
#define ELM_PORT mySerial


ELM327 myELM327;


uint32_t rpm = 0;


void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(115200);
  ELM_PORT.begin(115200);

  Serial.println("Attempting to connect to ELM327...");

  if (!myELM327.begin(ELM_PORT))
  {
    Serial.println("Couldn't connect to OBD scanner");
    while (1);
  }

  Serial.println("Connected to ELM327");
}


void loop()
{
  float tempRPM = myELM327.rpm();

  if (myELM327.status == ELM_SUCCESS)
  {
    rpm = (uint32_t)tempRPM;
    Serial.print("RPM: "); Serial.println(rpm);
  }
  else
  {
    Serial.print(F("\tERROR: "));
    Serial.println(myELM327.status);
    delay(100);
  }
}

