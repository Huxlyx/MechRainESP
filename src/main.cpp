#include <Arduino.h>
#include <Wifi.h>

const int AirValue = 3500;
const int WaterValue = 1450;

const char *WIFI_SSID = "Welcome to City 17 legacy";
const char *WIFI_PW = "DeineMutter123";

// const char* HORST = "192.168.0.60";
const char* HORST = "192.168.0.67"; // <- Raspi
const int   PORT   = 7777;

void setup()
{
  Serial.begin(115200);
  delay(10);

  WiFi.disconnect();
  delay(1000);
  WiFi.setHostname("MechRain");
  WiFi.mode(WIFI_STA);
}

void checkConnection()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("(Re)connecting WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PW);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi Connected");
  }
}

void loop()
{
  WiFiClient client;

  checkConnection();
  Serial.println("Connecting to Server");
  if (!client.connect(HORST, PORT))
  {
    Serial.println("Connection to Server failed");
    return;
  }

  while (client.connected())
  {
    char buffer[7];
    int soilMoistureValue = analogRead(35);
    Serial.println(soilMoistureValue);
    int soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
    Serial.print(soilmoisturepercent);
    Serial.println("%");

    client.write("M_HumPercent;");
    itoa(soilmoisturepercent, buffer, 10);
    client.write(buffer);
    client.write("#");

    client.write("M_HumExact;");
    itoa(soilMoistureValue, buffer, 10);
    client.write(buffer);
    client.write("#");

    delay(30000);
  }
}