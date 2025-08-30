#include <Arduino.h>
#include <Wifi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include "Discovery.h"
#include "unit_type.h"

#define THIS_DEVICE_ID 1

const int AirValue = 3500;
const int WaterValue = 1450;

const char* WIFI_SSID = "Welcome to City 17 legacy";
const char* WIFI_PW = "DeineMutter123";

const int IN_CHANNEL_0 = 34; 
const int IN_CHANNEL_1 = 35; 
const int IN_CHANNEL_2 = 32; 

const int OUT_CHANNEL_0 = 26;
const int OUT_CHANNEL_1 = 25;
const int OUT_CHANNEL_2 = 33;

WiFiClient client;
unsigned long lastDiscovery = 0;

uint8_t* dataHeader = new uint8_t[3];
uint8_t* dataBuffer = new uint8_t[65535];

const byte HANDSHAKE[] = {
    DEVICE_ID, 0x00, 0x01, THIS_DEVICE_ID /* Send Device ID */
};

bool checkConnection()
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

	if (client.connected()) {
		return true;
	}

	if (millis() - lastDiscovery > 5000) {
		sendDiscovery();
		lastDiscovery = millis();
	}
			
	if (parseServerInfo()) {
		Serial.printf("Server found: %s:%d\n", serverInfo.ip.toString().c_str(), serverInfo.port);
			
		/* Establish connection with received server info */
		if (client.connect(serverInfo.ip, serverInfo.port)) {
			Serial.println("Connection established");
			return true;
		} else {
			Serial.println("Connection failed");
		}
	}
	return false;
}

int mapRelaisChannel(uint8_t channel) {
	switch(channel) {
		case 0:
			return OUT_CHANNEL_0;
		case 1:
			return OUT_CHANNEL_1;
		case 2:
			return OUT_CHANNEL_2;
	}
	return -1;
}

int mapMoistureChannel(uint8_t channel) {
	switch(channel) {
		case 0:
			return IN_CHANNEL_0;
		case 1:
			return IN_CHANNEL_1;
		case 2:
			return IN_CHANNEL_2;
	}
	return -1;
}

void sendMsg(String msg, uint8_t msgId) {
	Serial.println(msg);
	uint16_t len = msg.length();
	dataHeader[0] = msgId;
	dataHeader[1] = len << 8;
	dataHeader[2] = len;
	/* last byte returned is 0 terminator, therefore need + 1 to get whole string 
	(which we know the length of, so no terminator required) */
	msg.getBytes(dataBuffer, len + 1);
	client.write(dataHeader, 3);
	client.write(dataBuffer, len);
}

void toggleRelais(int channel, int duration) {
	int outChannel = mapRelaisChannel(channel);
	if (outChannel == -1) {
		sendMsg("Channel not available", ERROR);
		return;
	}

	digitalWrite(outChannel, HIGH);
	delay(duration);
	digitalWrite(outChannel, LOW);
}

void sendMoisturePercent(uint8_t channel) {
	int inChannel = mapMoistureChannel(channel);
	if (inChannel == -1) {
		sendMsg("Channel not available", ERROR);
		return;
	}

	int soilMoistureValue = analogRead(inChannel);
	int soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
	Serial.print(soilmoisturepercent);
	Serial.println("%");

	dataHeader[0] = SOIL_MOISTURE_PERCENT;
	dataHeader[1] = 0;
	dataHeader[2] = 1;
	client.write(dataHeader, 3);
	dataBuffer[0] = soilmoisturepercent;
	client.write(dataBuffer, 1);
}

void sendMoistureAbs(uint8_t channel) {

	int anChannel = mapMoistureChannel(channel);
	if (anChannel == -1) {
		sendMsg("Channel not available!", ERROR);
	}

	int soilMoistureValue = analogRead(anChannel);
	Serial.println(soilMoistureValue);
	dataHeader[0] = SOIL_MOISTURE_ABS;
	dataHeader[1] = 0;
	dataHeader[2] = 2;
	client.write(dataHeader, 3);
	dataBuffer[0] = soilMoistureValue >> 8;
	dataBuffer[1] = soilMoistureValue;
	client.write(dataBuffer, 2);
}

void handleMeasurementRequest() {

    client.read(dataHeader, 3);

	switch (dataHeader[0]){
		case SOIL_MOISTURE_PERCENT:
			Serial.println("    SOIL_MOISTURE_PERCENT");
    		client.read(dataHeader, 3);
			if (dataHeader[0] == CHANNEL_ID) {
				sendMoisturePercent(client.read());
			} else {
				Serial.println("    HELP! (1)");
			}
			break;
		case SOIL_MOISTURE_ABS:
			Serial.println("    SOIL_MOISTURE_ABS");
    		client.read(dataHeader, 3);
			if (dataHeader[0] == CHANNEL_ID) {
				sendMoistureAbs(client.read());
			} else {
				Serial.println("    HELP! (1)");
			}
			break;
		case TEMPERATURE:
		case HUMIDITY:
		case LIGHT:
		case DISTANCE_MM:
		case DISTANCE_ABS:
		case UP_TIME:
		case IMAGE:
			sendMsg("    Not available yet", ERROR);
			break;
	}
}

/**
 * Toggle out pin for Relais,
 * duration limited to 65_535 ms max.
 * 1 byte channel, 2 byte duration
 */
void handleToggleOutPin() {
    client.read(dataHeader, 3);

	uint8_t channel = dataHeader[0];
	uint16_t duration = dataHeader[1] << 8 | dataHeader[2];

	String str1 = "Toggling channel ";
	String str2 = " for ";

	String msg1 = str1 + channel;
	String msg2 = str2 + duration;
	sendMsg(msg1 + msg2, STATUS_MSG);

	toggleRelais(channel, duration);
}

void setup()
{
	pinMode(OUT_CHANNEL_0, OUTPUT);
	pinMode(OUT_CHANNEL_1, OUTPUT);
	pinMode(OUT_CHANNEL_2, OUTPUT);

	pinMode(IN_CHANNEL_0, INPUT);
	pinMode(IN_CHANNEL_1, INPUT);
	pinMode(IN_CHANNEL_2, INPUT);

	Serial.begin(115200);
	delay(10);

	WiFi.disconnect();
	delay(1000);
	WiFi.setHostname("MechRain");
	WiFi.mode(WIFI_STA);
	udp.begin(UDP_PORT);
}

void loop()
{
	if ( ! checkConnection()) {
		delay(1000);
		Serial.println("No connection to server");
		return;
	}
    
	client.write(HANDSHAKE, 4);
	bool keepGoing = true;

	while (client.connected() && keepGoing)
	{
		if (client.available())
        {
			uint8_t retryIndex = 0;
			uint16_t payload;
			while (client.read(dataHeader, 3) < 0 && retryIndex++ < 100)
			{
				//... read until we actually receive data - bug?
				Serial.println("retry");
			}
			Serial.println("Got Command");
            switch (dataHeader[0])
            {
				case MEASUREMENT_REQ:
					Serial.println("  Measurement request");
					handleMeasurementRequest();
					break;
				case RESET:
					keepGoing = false;
					sendMsg("Resetting", STATUS_MSG);
					lastDiscovery = 0;
					client.stop();
					break;
				case TOGGLE_OUT_PIN:
					Serial.println("  Toggle pin");
					/* expect 3 byte payload */
					payload = dataHeader[1] << 8 | dataHeader[2];
					if (payload != 3) {
						String str1 = "Expected 3 byte payload but got ";
						String errString = str1 + payload;
						sendMsg(errString, ERROR);
					} else {
						handleToggleOutPin();
					}
					break;
				default:
					String str1 = "Unknown command";
					String msg1 = str1 + dataHeader[0];
					sendMsg(msg1, ERROR);
					break;
			}
		}
	}
	Serial.println("resetting");
}