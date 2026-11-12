#ifndef MRP_H
#define MRP_H

#include <Arduino.h>
#include <WiFiClient.h>

extern WiFiClient client;
extern byte* dataBuffer;
void sendHeader(const uint8_t id, const uint16_t length);
void sendPacket(const uint8_t id, const uint8_t* payload, const uint16_t length);
void sendUInt8Packet(const uint8_t id, const uint8_t val);
void sendUInt16Packet(const uint8_t id, const uint16_t val);
void sendFloatPacket(const uint8_t id, const float val);
void sendChanneledUInt8Packet(const uint8_t id, const uint8_t val, const uint8_t channel);
void sendChanneledUInt16Packet(const uint8_t id, const uint16_t val, const uint8_t channel);
void sendMsg(String msg, uint8_t msgId);
bool readHeader();
int readByte();
int readData(int expected);
uint8_t lastCommandId();
uint16_t lastCommandLength();

#endif  // MRP_H