#include <Arduino.h>
#include <Wifi.h>
#include <Wire.h>
#include <Adafruit_HTU21DF.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <Preferences.h>
#include "Discovery.h"
#include "MRP.h"
#include "unit_type.h"

Adafruit_HTU21DF sht21 = Adafruit_HTU21DF();

#define THIS_DEVICE_ID 0

/* add build timestamp (auto-updated at each build) */
#define BUILD_TIMESTAMP __DATE__ " " __TIME__

const int AirValue = 3500;
const int WaterValue = 1450;

const char* WIFI_SSID = "Vodafone-F028";
const char* WIFI_PW = "$wRGA*4jqUDNgNb$";

const int CO2_PWM_IN	= 17;
const int PPM_RANGE 	= 5000;

const int IN_CHANNEL_0 	= 36;
const int IN_CHANNEL_1 	= 39;
const int IN_CHANNEL_2 	= 34;
const int IN_CHANNEL_3	= 35;
const int IN_CHANNEL_4 	= 32;
const int IN_CHANNEL_5 	= 33;
const int IN_CHANNEL_6 	= 25;
const int IN_CHANNEL_7 	= 26;

const int OUT_CHANNEL_0	= 27;
const int OUT_CHANNEL_1	= 14;
const int OUT_CHANNEL_2	= 12;
const int OUT_CHANNEL_3	= 13;

unsigned long lastDiscovery = 0;

const uint16_t DEFAULT_UDP_BROADCAST_DELAY = 5000;
const uint16_t DEFAULT_CONNECTION_DELAY = 1000;

Preferences preferences;
uint16_t udpBroadcastDelay;
uint16_t connectionDelay;
uint8_t deviceId;

uint8_t inPinMask;
uint8_t outPinMask;

bool sht21Available;

const char* PREF_KEY_UDP_BROADCAST_DELAY = "UdpBCDelay";
const char* PREF_KEY_CONNECTION_DELAY = "ConDelay";
const char* PREF_KEY_DEVICE_ID = "DeviceId";
const char* PREF_KEY_IN_PIN_MASK = "InChMsk";
const char* PREF_KEY_OUT_PIN_MASK = "OutChMsk";

bool checkConnection();
int mapOutputChannel(uint8_t channel);
int mapInputChannel(uint8_t channel);
void sendMsg(String msg, uint8_t msgId);
void sendMoisturePercent(uint8_t channel);
void sendMoistureAbs(uint8_t channel);
void handleMeasurementRequest();
void handleDeviceSettingRequest();
void handleDeviceSettingChange();
void handleToggleOutPin();
uint16_t readCO2PWM();

void setup()
{
	pinMode(OUT_CHANNEL_0, OUTPUT);
	pinMode(OUT_CHANNEL_1, OUTPUT);
	pinMode(OUT_CHANNEL_2, OUTPUT);
	pinMode(OUT_CHANNEL_3, OUTPUT);

	pinMode(IN_CHANNEL_0, INPUT);
	pinMode(IN_CHANNEL_1, INPUT);
	pinMode(IN_CHANNEL_2, INPUT);
	pinMode(IN_CHANNEL_3, INPUT);
	pinMode(IN_CHANNEL_4, INPUT);
	pinMode(IN_CHANNEL_5, INPUT);
	pinMode(IN_CHANNEL_6, INPUT);
	pinMode(IN_CHANNEL_7, INPUT);

	pinMode(CO2_PWM_IN, INPUT);
	sht21Available = sht21.begin();

	Serial.begin(115200);
	delay(10);


	preferences.begin("MechRain", false);
	udpBroadcastDelay = preferences.getUShort(PREF_KEY_UDP_BROADCAST_DELAY, DEFAULT_UDP_BROADCAST_DELAY);
	connectionDelay = preferences.getUShort(PREF_KEY_CONNECTION_DELAY, DEFAULT_CONNECTION_DELAY);
	deviceId = preferences.getUChar(PREF_KEY_DEVICE_ID, THIS_DEVICE_ID);
	inPinMask = preferences.getUChar(PREF_KEY_IN_PIN_MASK, 0);
	outPinMask = preferences.getUChar(PREF_KEY_OUT_PIN_MASK, 0);

	Serial.printf("UDP broadcast delay: %d Connection delay: %d\n", udpBroadcastDelay, connectionDelay);
	Serial.printf("Device id: %d\n", deviceId);
	Serial.printf("inPinMask: %d outPinMask: %d\n", inPinMask, outPinMask);

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
    
	/* send device id */
	sendHeader(DEVICE_ID, 1);
	sendByte(deviceId);

	/* send build id */
	String buildId = String(BUILD_TIMESTAMP);
	sendMsg(buildId, BUILD_ID);

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
				case DEVICE_SETTING_CHANGE:
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

int mapOutputChannel(uint8_t channel) {
	switch(channel) {
		case 0:
			return OUT_CHANNEL_0;
		case 1:
			return OUT_CHANNEL_1;
		case 2:
			return OUT_CHANNEL_2;
		case 3:
			return OUT_CHANNEL_3;
	}
	return -1;
}

int mapInputChannel(uint8_t channel) {
	switch(channel) {
		case 0:
			return IN_CHANNEL_0;
		case 1:
			return IN_CHANNEL_1;
		case 2:
			return IN_CHANNEL_2;
		case 3:
			return IN_CHANNEL_3;
		case 4:
			return IN_CHANNEL_4;
		case 5:
			return IN_CHANNEL_5;
		case 6:
			return IN_CHANNEL_6;
		case 7:
			return IN_CHANNEL_7;
	}
	return -1;
}

void toggleRelais(int channel, int duration) {
	int outChannel = mapOutputChannel(channel);
	if (outChannel == -1) {
		sendMsg("Channel not available", ERROR);
		return;
	}

	if (channel & outPinMask == 0) {
		sendMsg("Output pin not enabled", ERROR);
		return;
	}

	digitalWrite(outChannel, HIGH);
	delay(duration);
	digitalWrite(outChannel, LOW);
}

void sendMoisturePercent(uint8_t channel) {
	int inChannel = mapInputChannel(channel);
	if (inChannel == -1) {
		sendMsg("Channel not available", ERROR);
		return;
	}

	if (channel & inPinMask == 0) {
		sendMsg("Input pin not enabled", ERROR);
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

	int anChannel = mapInputChannel(channel);
	if (anChannel == -1) {
		sendMsg("Channel not available!", ERROR);
	}

	uint16_t soilMoistureValue = analogRead(anChannel);
	Serial.println(soilMoistureValue);
	sendHeader(SOIL_MOISTURE_ABS, 2);
	sendShort(soilMoistureValue);
}

void sendCo2Ppm() {
	uint16_t ppmVal = readCO2PWM();
	Serial.println(ppmVal);
	sendHeader(CO2_PPM, 2);
	sendShort(ppmVal);
}

void handleMeasurementRequest() {

    readHeader();

	uint16_t co2ppm;

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
			Serial.println("    TEMPERATURE");
			if ( ! sht21Available) {
				sendMsg("Sht21 not available", ERROR);
			} else {
				float shtTemp = sht21.readTemperature();
				Serial.println(shtTemp);
				sendHeader(TEMPERATURE, 4);
				sendFloat(shtTemp);
			}
			break;
		case HUMIDITY:
			Serial.println("    HUMIDITY");
			if ( ! sht21Available) {
				sendMsg("Sht21 not available", ERROR);
			} else {
				float shtHumid = sht21.readHumidity();
				Serial.println(shtHumid);
				sendHeader(HUMIDITY, 4);
				sendFloat(shtHumid);
			}
			break;
		case CO2_PPM:
			Serial.println("    CO2_PPM");
			co2ppm = readCO2PWM();
			Serial.println(co2ppm);
			sendHeader(CO2_PPM, 2);
			sendShort(co2ppm);
			break;
		case LIGHT:
		case DISTANCE_MM:
		case DISTANCE_ABS:
		case UP_TIME:
		case IMAGE:
		default:
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
		case DEVICE_ID:
			Serial.println("Device id request");
			sendHeader(DEVICE_ID, 1);
			sendByte(deviceId);
			break;
		case UDP_BROADCAST_DELAY:
			Serial.println("Broadcast delay request");
			sendHeader(UDP_BROADCAST_DELAY, 2);
			sendShort(udpBroadcastDelay);
			break;
		case CONNECTION_DELAY:
			Serial.println("Connection delay request");
			sendHeader(CONNECTION_DELAY, 2);
			sendShort(connectionDelay);
			break;
		case IN_PIN_MASK:
			Serial.println("Input pin mask request");
			sendHeader(IN_PIN_MASK, 1);
			sendByte(inPinMask);
			break;
		case OUT_PIN_MASK:
			Serial.println("Output pin mask request");
			sendHeader(OUT_PIN_MASK, 1);
			sendByte(outPinMask);
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
		uint8_t byteVal;
		uint16_t shortVal;
		case DEVICE_ID:
			byteVal = lastCommandLength();
			Serial.printf("Set id: %d\n", byteVal);
			deviceId = byteVal;
			byteVal = preferences.putUChar(PREF_KEY_DEVICE_ID, byteVal);
			Serial.printf("Set id result: %d\n", byteVal);
			sendHeader(ACK, 0);
			break;
		case UDP_BROADCAST_DELAY:
			shortVal = lastCommandLength();
			Serial.printf("Set broadcast delay: %d\n", shortVal);
			udpBroadcastDelay = shortVal;
			preferences.putUShort(PREF_KEY_UDP_BROADCAST_DELAY, shortVal);
			sendHeader(ACK, 0);
			break;
		case CONNECTION_DELAY:
			shortVal = lastCommandLength();
			Serial.printf("Set connection delay: %d\n", shortVal);
			connectionDelay = shortVal;
			preferences.putUShort(PREF_KEY_CONNECTION_DELAY, shortVal);
			sendHeader(ACK, 0);
			break;
		case IN_PIN_MASK:
			byteVal = lastCommandLength();
			Serial.printf("Set in pin mask to: %d\n", byteVal);
			inPinMask = byteVal;
			preferences.putUShort(PREF_KEY_IN_PIN_MASK, byteVal);
			sendHeader(ACK, 0);
			break;
		case OUT_PIN_MASK:
			byteVal = lastCommandLength();
			Serial.printf("Set out pin mask to: %d", byteVal);
			outPinMask = byteVal;
			preferences.putUShort(PREF_KEY_OUT_PIN_MASK, byteVal);
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

uint16_t readCO2PWM() {
    unsigned long th;
    uint16_t ppmPwm = 0;
    do {
        th = pulseIn(CO2_PWM_IN, HIGH, 2500000) / 1000;
        float pulsepercent = th / 1004.0;
        ppmPwm = PPM_RANGE * pulsepercent;
		/* wait till we got result */
    } while (th == 0);
    return ppmPwm;
}