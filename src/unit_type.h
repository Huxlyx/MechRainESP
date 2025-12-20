#include <Arduino.h>

const uint8_t UNKNOWN               = 0x00;

const uint8_t DEVICE_ID             = 0x01;
const uint8_t MEASUREMENT_REQ       = 0x04;
const uint8_t MEASUREMENT_RESP      = 0x05;
const uint8_t TOGGLE_OUT_PIN        = 0x06;
const uint8_t DEVICE_SETTING_REQ    = 0x0C;
const uint8_t DEVICE_SETTING_CHANGE = 0x0D;
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
const uint8_t CO2_PPM               = 0xB7;
const uint8_t UP_TIME               = 0xBA;
const uint8_t IMAGE                 = 0xBE;

const uint8_t START_SEGMENT = 0xC0;
const uint8_t SEGMENT       = 0xC1;
const uint8_t END_SEGMENT   = 0xC2;

const uint8_t UDP_BROADCAST_DELAY   = 0xD0;
const uint8_t CONNECTION_DELAY      = 0xD1;
const uint8_t IN_PIN_MASK           = 0xD2;
const uint8_t OUT_PIN_MASK          = 0xD3;
const uint8_t NUM_PIXELS            = 0xD4;

const uint8_t RESET_LED             = 0xE0;
const uint8_t SET_LED_MODE_1        = 0xE1;
const uint8_t SET_LED_MODE_2        = 0xE2;
const uint8_t SET_LED_MODE_3        = 0xE3;
const uint8_t SET_LED_MODE_4        = 0xE4;
const uint8_t SET_LED_MODE_5        = 0xE5;
const uint8_t SET_LED_ALL_RGB       = 0xEA;
const uint8_t SET_LED_SINGLE_RGB    = 0xEB;

const uint8_t STATUS_MSG    = 0xF0;
const uint8_t BUILD_ID      = 0xF1;
const uint8_t HEARTBEAT     = 0xF2;
const uint8_t ERROR         = 0xFF;

/* I/O channel mask*/

const uint8_t IN_CHANNEL_0_EN = 0x01;
const uint8_t IN_CHANNEL_1_EN = 0x02;
const uint8_t IN_CHANNEL_2_EN = 0x04;
const uint8_t IN_CHANNEL_3_EN = 0x08;
const uint8_t IN_CHANNEL_4_EN = 0x10;
const uint8_t IN_CHANNEL_5_EN = 0x20;
const uint8_t IN_CHANNEL_6_EN = 0x40;
const uint8_t IN_CHANNEL_7_EN = 0x80;

const uint8_t OUT_CHANNEL_0_EN = 0x01;
const uint8_t OUT_CHANNEL_1_EN = 0x02;
const uint8_t OUT_CHANNEL_2_EN = 0x04;
const uint8_t OUT_CHANNEL_3_EN = 0x08;