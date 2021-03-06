#include <ESP8266WiFi.h>
#include "ELMduino.h"


const char* ssid = "WiFi_OBDII";
const char* password = "your-password";


//IP Adress of your ELM327 Dongle
IPAddress server(192, 168, 0, 10);
WiFiClient client;
ELM327 myELM327;


uint32_t rpm = 0;


void setup()
{
  Serial.begin(115200);

  // Connecting to ELM327 WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid);
  // WiFi.begin(ssid, password); //Use this line if your ELM327 has a password protected WiFi

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected to Wifi");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (client.connect(server, 35000))
    Serial.println("connected");
  else
  {
    Serial.println("connection failed");
    ESP.reset();
  }

  myELM327.begin(client, true, 2000);

  Serial.println("Connected to ELM327");
  Serial.println("Ensure your serial monitor line ending is set to 'Carriage Return'");
  Serial.println("Type and send commands/queries to your ELM327 through the serial monitor");
  Serial.println();
}


void loop()
{
  if(Serial.available())
  {
    char c = Serial.read();

    Serial.write(c);
    client.write(c);
  }

  if(client.available())
  {
    char c = client.read();

    if(c == '>')
      Serial.println();

    Serial.write(c);
  }
}
