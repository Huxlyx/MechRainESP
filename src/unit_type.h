#include <Arduino.h>

const uint8_t DEVICE_ID             = 0x01;
const uint8_t MEASUREMENT_REQ       = 0x04;
const uint8_t MEASUREMENT_RESP      = 0x05;
const uint8_t TOGGLE_OUT_PIN        = 0x06;
const uint8_t DEVICE_SETTING_REQ    = 0x0C;
const uint8_t DEVICE_SETTING_SET    = 0x0D;
const uint8_t ACK                   = 0x0E;
const uint8_t RESET                 = 0x0F;

const uint8_t CHANNEL_ID    = 0xA0;
const uint8_t DURATION_MS   = 0xA1;

const uint8_t SOIL_MOISTURE_PERCENT = 0xB0;
const uint8_t SOIL_MOISTURE_ABS     = 0xB1;
const uint8_t TEMPERATURE           = 0xB2;
const uint8_t HUMIDITY              = 0xB3;
const uint8_t LIGHT                 = 0xB4;
const uint8_t DISTANCE_MM           = 0xB5;
const uint8_t DISTANCE_ABS          = 0xB6;
const uint8_t UP_TIME               = 0xBA;
const uint8_t IMAGE                 = 0xBE;

const uint8_t START_SEGMENT = 0xC0;
const uint8_t SEGMENT       = 0xC1;
const uint8_t END_SEGMENT   = 0xC2;

const uint8_t UDP_BROADCAST_DELAY   = 0xD0;
const uint8_t CONNECTION_DELAY      = 0xD1;

const uint8_t STATUS_MSG    = 0xF0;
const uint8_t ERROR         = 0xFF;