
#include "Arduino.h"

#include "common.hpp"
#include "internal_state.hpp"
#include "logger.hpp"
#include "ota.hpp"
#include "com.hpp"
#include "wireless.hpp"

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include <ESP8266WiFi.h>

InternalState internalState;
OTA ota;
Com com;
Wireless wireless;

bool debug = false;

bool mcuHandshaked = false;
uint8 mcuReadyState = 0;
static unsigned long lastMcuReadyStateSend = 0;

ConfigMessage confirming_state;

struct TuyaMessage {
    uint16_t header;  // Fixed: 0x55AA
    uint8_t version;
    uint8_t command;
    uint16_t length;
    uint8_t value[];  // Variable length. May contain simple value, or DataPoint struct, depending on command.
    uint8_t checksum;  // Sum of all previous bytes, modulo 256
};

void setup() {
    internalState.setup();
    com.setup();
    wireless.setup(internalState.getBootState());

    ota.setup();
    ota.onEnd([&] () {
        internalState.setBootState(BootState::BOOT_STATE_NORMAL);
    });

    com.onCommand([&] (Command command, char * message, uint16 len) {
        if (command == Command::HANDSHAKE) {
            logger.log("Handshake ACK received");

            mcuHandshaked = true;
        }

        if (command == Command::CONFIG) {
            logger.log("Config ACK received");

            mcuReadyState += 1;
            lastMcuReadyStateSend = 0;
            return;
        }

        if (command == Command::DATA) {
            if (len >= 5) {
                if (message[0] == 0x65 && message[1] == 0x01) {
                    if (message[4] == 0x01) {
                        wireless.sendEvent(Event::MOTION_DETECTED);
                    } else if (message[4] == 0x00) {
                        wireless.sendEvent(Event::MOTION_CLEARED);
                    }
                } else if (message[0] == 0x66 && message[1] == 0x04) {
                    if (message[4] == 0x00) {
                        wireless.sendEvent(Event::BATTERY_HIGH);
                    } else if (message[4] == 0x01) {
                        wireless.sendEvent(Event::BATTERY_MEDIUM);
                    } else if (message[4] == 0x02) {
                        wireless.sendEvent(Event::BATTERY_LOW);
                    }
                }
            }
        }

        if (command == Command::RESET) {
            if (internalState.getBootState() == BootState::BOOT_STATE_NORMAL) {
                internalState.setBootState(BootState::BOOT_STATE_STA);
                ESP.restart();
            } else if (internalState.getBootState() == BootState::BOOT_STATE_STA) {
                internalState.setBootState(BootState::BOOT_STATE_AP);
                ESP.restart();
            } else if (internalState.getBootState() == BootState::BOOT_STATE_AP) {
                internalState.setBootState(BootState::BOOT_STATE_NORMAL);
                ESP.restart();
            }
        }
    });
}

void startHandshake() {
    if (mcuHandshaked) {
        return;
    }

    static unsigned long lastHandshakeSent = 0;

    if (millis() < lastHandshakeSent || millis() - lastHandshakeSent < 1000) {
        return;
    }

    logger.log("Handshake send");
    sendCommand(Command::HANDSHAKE, "", 0);
    lastHandshakeSent = millis();
}


long last = 0;
void loop() {
    ota.loop();
    com.loop();
    wireless.loop();

    static unsigned long connected = 0;
    if (!connected && wireless.getReadyState() > 0) {
        connected = millis();
    }

    if (!mcuHandshaked && (internalState.getBootState() == BootState::BOOT_STATE_NORMAL || connected && millis() - connected > 5000)) {
        startHandshake();
    }
    if (mcuHandshaked && mcuReadyState != wireless.getReadyState() && (lastMcuReadyStateSend == 0 || millis() - lastMcuReadyStateSend > 500)) {
        lastMcuReadyStateSend = millis();
        char data[2];
        data[0] = 0x00;
        data[1] = 0x00;
        if (internalState.getBootState() == BootState::BOOT_STATE_NORMAL) {
            if (mcuReadyState == 0) {
                data[0] = ConfigMessage::WIFI_CONNECTED;
            }
            if (mcuReadyState == 1) {
                data[0] = ConfigMessage::GOT_IP;
            }
            if (mcuReadyState == 2) {
                data[0] = ConfigMessage::MQTT_CONNECTED;
            }
            sendCommand(Command::CONFIG, data, 1);
            logger.log("send config normal " + String(mcuReadyState));
        } else if (internalState.getBootState() == BootState::BOOT_STATE_STA) {
            data[0] = ConfigMessage::STA;
            sendCommand(Command::CONFIG, data, 1);
            logger.log("send config sta");
        } else if (internalState.getBootState() == BootState::BOOT_STATE_AP) {
            data[0] = ConfigMessage::AP;
            sendCommand(Command::CONFIG, data, 1);
            logger.log("send config ap");
        }
    }

    if (millis() - last < 5000) {
        return;
    }
    last = millis();


//    Serial.println("Connected: " + String(WiFi.localIP().toString()));
//    esp_now_send(remoteMac, (u8 *) "ping", 4); // NULL means send to all peers
}