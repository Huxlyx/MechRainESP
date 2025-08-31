#include <Arduino.h>
#include <Wifi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <Preferences.h>
#include "Discovery.h"
#include "MRP.h"
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

unsigned long lastDiscovery = 0;

const uint16_t DEFAULT_UDP_BROADCAST_DELAY = 5000;
const uint16_t DEFAULT_CONNECTION_DELAY = 1000;

Preferences preferences;
uint16_t udpBroadcastDelay;
uint16_t connectionDelay;

const char* PREF_KEY_UDP_BROADCAST_DELAY = "UdpBCDelay";
const char* PREF_KEY_CONNECTION_DELAY = "ConDelay";

const byte HANDSHAKE[] = {
    DEVICE_ID, 0x00, 0x01, THIS_DEVICE_ID /* Send Device ID */
};

bool checkConnection();
int mapRelaisChannel(uint8_t channel);
int mapMoistureChannel(uint8_t channel);
void sendMsg(String msg, uint8_t msgId);
void sendMoisturePercent(uint8_t channel);
void sendMoistureAbs(uint8_t channel);
void handleMeasurementRequest();
void handleDeviceSettingRequest();
void handleDeviceSettingChange();
void handleToggleOutPin();

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

	preferences.begin("MechRain", false);
	udpBroadcastDelay = preferences.getUShort(PREF_KEY_UDP_BROADCAST_DELAY, DEFAULT_UDP_BROADCAST_DELAY);
	connectionDelay = preferences.getUShort(PREF_KEY_CONNECTION_DELAY, DEFAULT_CONNECTION_DELAY);

	Serial.printf("UDP broadcast delay: %d Connection delay: %d\n", udpBroadcastDelay, connectionDelay);

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
			while (readHeader() < 0 && retryIndex++ < 100)
			{
				//... read until we actually receive data - bug?
				Serial.println("retry");
			}
			Serial.println("Got Command");
            switch (lastCommandId())
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
					payload = lastCommandLength();
					if (payload != 3) {
						String str1 = "Expected 3 byte payload but got ";
						String errString = str1 + payload;
						sendMsg(errString, ERROR);
					} else {
						handleToggleOutPin();
					}
					break;
				case DEVICE_SETTING_REQ:
					handleDeviceSettingRequest();
					break;
				case DEVICE_SETTING_SET:
					handleDeviceSettingChange();
					break;
				default:
					String str1 = "Unknown command";
					String msg1 = str1 + lastCommandId();
					sendMsg(msg1, ERROR);
					break;
			}
		}
	}
	Serial.println("resetting");
}

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

	sendHeader(SOIL_MOISTURE_PERCENT, 1);
	sendByte(soilmoisturepercent);
}

void sendMoistureAbs(uint8_t channel) {

	int anChannel = mapMoistureChannel(channel);
	if (anChannel == -1) {
		sendMsg("Channel not available!", ERROR);
	}

	uint16_t soilMoistureValue = analogRead(anChannel);
	Serial.println(soilMoistureValue);
	sendHeader(SOIL_MOISTURE_ABS, 2);
	sendShort(soilMoistureValue);
}

void handleMeasurementRequest() {

    readHeader();

	switch (lastCommandId()){
		case SOIL_MOISTURE_PERCENT:
			Serial.println("    SOIL_MOISTURE_PERCENT");
    		readHeader();
			if (lastCommandId() == CHANNEL_ID) {
				sendMoisturePercent(client.read());
			} else {
				Serial.println("    HELP! (1)");
			}
			break;
		case SOIL_MOISTURE_ABS:
			Serial.println("    SOIL_MOISTURE_ABS");
    		readHeader();
			if (lastCommandId() == CHANNEL_ID) {
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

void handleDeviceSettingRequest() {
	if ( ! readHeader()) {
		sendMsg("Error reading header", ERROR);
		return;
	}
	switch (lastCommandId()) {
		case UDP_BROADCAST_DELAY:
			Serial.println("Broadcast delay request");
			sendHeader(UDP_BROADCAST_DELAY, 2);
			sendShort(udpBroadcastDelay);
			break;
		case CONNECTION_DELAY:
			Serial.println("Connection delay request");
			sendHeader(CONNECTION_DELAY, 2);
			sendShort(udpBroadcastDelay);
			break;
		default:
			String str1 = "Unknown request";
			String msg1 = str1 + lastCommandId();
			Serial.println(msg1);
			sendMsg(msg1, ERROR);
	}
}

void handleDeviceSettingChange() {
	if ( ! readHeader()) {
		sendMsg("Error reading header", ERROR);
		return;
	}
	switch (lastCommandId()) {
		uint16_t newDelay;
		case UDP_BROADCAST_DELAY:
			newDelay = lastCommandLength();
			Serial.printf("Set broadcast delay: %d", newDelay);
			udpBroadcastDelay = newDelay;
			preferences.putUShort(PREF_KEY_UDP_BROADCAST_DELAY, newDelay);
			sendHeader(ACK, 0);
			break;
		case CONNECTION_DELAY:
			newDelay = lastCommandLength();
			Serial.printf("Set connection delay: %d", newDelay);
			connectionDelay = newDelay;
			preferences.putUShort(PREF_KEY_CONNECTION_DELAY, newDelay);
			sendHeader(ACK, 0);
			break;
		default:
			String str1 = "Unknown request";
			String msg1 = str1 + lastCommandId();
			Serial.println(msg1);
			sendMsg(msg1, ERROR);
	}
}

/**
 * Toggle out pin for Relais,
 * duration limited to 65_535 ms max.
 * 1 byte channel, 2 byte duration
 */
void handleToggleOutPin() {
	readData(3);

	uint8_t channel = dataBuffer[0];
	uint16_t duration = dataBuffer[1] << 8 | dataBuffer[2];

	String str1 = "Toggling channel ";
	String str2 = " for ";

	String msg1 = str1 + channel;
	String msg2 = str2 + duration;
	sendMsg(msg1 + msg2, STATUS_MSG);

	toggleRelais(channel, duration);
}
