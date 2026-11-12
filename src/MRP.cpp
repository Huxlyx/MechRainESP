#include "MRP.h"
#include "unit_type.h"

WiFiClient client;

byte* dataHeader = new byte[3];
byte* dataBuffer = new byte[65535];

/* header (3) + largest fixed-size payload: 4 byte float + 4 byte channel field */
static const size_t TX_BUFFER_SIZE = 16;
static byte txBuffer[TX_BUFFER_SIZE];

typedef union
{
	float val;
	uint8_t bytes[4];
} FLOATUNION_t;

FLOATUNION_t floatVal;

static void writeHeader(byte* buf, const uint8_t id, const uint16_t length) {
	buf[0] = id;
	buf[1] = length >> 8;
	buf[2] = length;
}

void sendHeader(const uint8_t id, const uint16_t length) {
	writeHeader(dataHeader, id, length);
	client.write(dataHeader, 3);
}

void sendPacket(const uint8_t id, const uint8_t* payload, const uint16_t length) {
	if (length > TX_BUFFER_SIZE - 3) {
		Serial.printf("Payload too large for tx buffer: %d\n", length);
		return;
	}
	writeHeader(txBuffer, id, length);
	memcpy(txBuffer + 3, payload, length);
	/* single write, so header and payload leave in one TCP segment */
	client.write(txBuffer, length + 3);
}

void sendUInt8Packet(const uint8_t id, const uint8_t val) {
	sendPacket(id, &val, 1);
}

void sendUInt16Packet(const uint8_t id, const uint16_t val) {
	const uint8_t payload[2] = { (uint8_t) (val >> 8), (uint8_t) val };
	sendPacket(id, payload, 2);
}

void sendFloatPacket(const uint8_t id, const float val) {
	floatVal.val = val;
	sendPacket(id, floatVal.bytes, 4);
}

/*
 * Channeled replies append a channel field (CHANNEL_ID, 2 byte length = 1, channel)
 * after the value, so the server can tell which channel a reply belongs to.
 */
static uint16_t appendChannel(uint8_t* payload, uint16_t offset, const uint8_t channel) {
	payload[offset++] = CHANNEL_ID;
	payload[offset++] = 0x00;
	payload[offset++] = 0x01;
	payload[offset++] = channel;
	return offset;
}

void sendChanneledUInt8Packet(const uint8_t id, const uint8_t val, const uint8_t channel) {
	uint8_t payload[5];
	payload[0] = val;
	sendPacket(id, payload, appendChannel(payload, 1, channel));
}

void sendChanneledUInt16Packet(const uint8_t id, const uint16_t val, const uint8_t channel) {
	uint8_t payload[6];
	payload[0] = val >> 8;
	payload[1] = val;
	sendPacket(id, payload, appendChannel(payload, 2, channel));
}

bool readHeader() {
	/* readBytes waits up to the stream timeout, read() would return whatever has arrived so far */
	return client.readBytes(dataHeader, 3) == 3;
}

int readByte() {
	uint8_t val;
	return client.readBytes(&val, 1) == 1 ? val : -1;
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
	/* room for header and the terminator getBytes writes */
	if (len > 65535 - 4) {
		len = 65535 - 4;
	}
	writeHeader(dataBuffer, msgId, len);
	/* last byte returned is 0 terminator, therefore need + 1 to get whole string
	(which we know the length of, so no terminator required) */
	msg.getBytes(dataBuffer + 3, len + 1);
	/* single write, so header and text leave together */
	client.write(dataBuffer, len + 3);
}

int readData(int expected) {
	return client.readBytes(dataBuffer, expected);
}
