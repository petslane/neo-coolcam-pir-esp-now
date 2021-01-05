
#pragma once

#include "Arduino.h"

enum Command: uint8 {
    HANDSHAKE = 0x01,
    CONFIG = 0x02,
    RESET = 0x03,
    DATA = 0x05,
};

enum Event: uint8 {
    MOTION_DETECTED,
    MOTION_CLEARED,
    BATTERY_LOW,
    BATTERY_MEDIUM,
    BATTERY_HIGH,
};

enum ConfigMessage: uint8 {
    AP = 0x00,
    STA = 0x01,
    WIFI_CONNECTED = 0x02,
    GOT_IP = 0x03,
    MQTT_CONNECTED = 0x04,
};

struct DataPoint {
    uint8_t id;
    uint8_t type;
    uint16_t length;
    uint8_t value[];   // Variable length
};

void sendCommand(Command command, char * message, uint16 len) {
    uint8 data[10];
    data[0] = 0x55;
    data[1] = 0xAA;
    data[2] = 0x00;
    data[3] = command;
    data[4] = len >> 8;
    data[5] = len & 0xFF;
    Serial.write(data, 6);
    if (len > 0) {
        Serial.write(message, len);
    }
    uint8 checksum = 0;
    for (int i = 0; i < 6; i++) {
        checksum += (uint8) data[i];
    }
    for (int i = 0; i < len; i++) {
        checksum += (uint8) message[i];
    }
    Serial.write(checksum);
    Serial.println();
}
