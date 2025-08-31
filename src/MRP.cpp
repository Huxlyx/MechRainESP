#include "MRP.h"

WiFiClient client;

byte* dataHeader = new byte[3];
byte* dataBuffer = new byte[65535];

void sendHeader(const uint8_t id, const uint16_t length) {
	dataHeader[0] = id;
	dataHeader[1] = length << 8;
	dataHeader[2] = length;
	client.write(dataHeader, 3);
}

int readHeader() {
    return client.read(dataHeader, 3);
}

uint8_t lastCommandId() {
    return dataHeader[0];
}

uint16_t lastCommandLength() {
    return dataHeader[1]<< 8 | dataHeader[2];
}

void sendMsg(String msg, uint8_t msgId) {
	Serial.println(msg);
	uint16_t len = msg.length();
	sendHeader(msgId, len);
	/* last byte returned is 0 terminator, therefore need + 1 to get whole string 
	(which we know the length of, so no terminator required) */
	msg.getBytes(dataBuffer, len + 1);
	client.write(dataBuffer, len);
}

void sendByte(uint8_t val) {
    client.write(val);
}

void sendShort(uint16_t val)
{
    dataBuffer[0] = val >> 8;
    dataBuffer[1] = val;
	client.write(dataBuffer, 2);
}

int readData(int expected) {
    return client.read(dataBuffer, expected);
}