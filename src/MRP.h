#ifndef MRP_H
#define MRP_H

#include <Arduino.h>
#include <WiFiClient.h>

extern WiFiClient client;
extern byte* dataBuffer;
void sendHeader(const uint8_t id, const uint16_t length);
void sendMessage(String msg, uint8_t msgId);
void sendByte(uint8_t val);
void sendShort(uint16_t val);
void sendFloat(float val);
bool readHeader();
int readData(int expected);
uint8_t lastCommandId();
uint16_t lastCommandLength();

#endif  // MRP_H