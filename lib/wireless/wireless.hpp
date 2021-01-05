
#pragma once

#include "Arduino.h"
#include "internal_state.hpp"
#include "logger.hpp"

extern "C" {
#include "user_interface.h"
#include <espnow.h>
}

#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s

// Replace with your network credentials
const char* ssid = STRINGIFY(WIFI_SSID);
const char* password = STRINGIFY(WIFI_PASS);

uint8_t mac[6] = {NOW_GW_MAC};

bool messageDelivered = false;

class Wireless {
private:
    BootState state;

    uint8 readyState = 0;

    bool isHandshakeDone = false;

    std::vector<Event> events;

    bool sendingMessage = false;

    void setupSTA() {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        while (WiFi.waitForConnectResult() != WL_CONNECTED) {
            ESP.restart();
        }

//        readyState += 1; // wifi connected
//        readyState += 1; // Got IP
        readyState += 1; // MQTT connected
        logger.log("Connected: " + String(WiFi.localIP().toString()));
    }

    void setupNormal() {
        WiFi.mode(WIFI_AP);

        if (esp_now_init() != 0) {
            ESP.restart();
        }

        esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

        // sent now cb
        esp_now_register_send_cb([&](uint8_t *mac, uint8_t sendStatus) {
            messageDelivered = true;
        });

        readyState += 1; // wifi connected
        readyState += 1; // Got IP
        readyState += 1; // MQTT connected
    }
    void setupAP() {
        setupSTA();
    }

public:
    void setup(BootState state) {
        this->state = state;

        if (state == BootState::BOOT_STATE_STA) {
            setupSTA();
        } else if (state == BootState::BOOT_STATE_NORMAL) {
            setupNormal();
        } else if (state == BootState::BOOT_STATE_AP) {
            setupAP();
        }
    }

    uint8 getReadyState() {
        return readyState;
    }

    void sendEvent(Event e) {
        events.push_back(e);
    }

    void loop() {
        if (messageDelivered) {
            messageDelivered = false;
            sendingMessage = false;

            events.erase(events.begin());

            char data[1];
            data[0] = 0x00;
            sendCommand(Command::DATA, data, 1); // ACK only if message sent out
        }
        if (events.size() > 0 && !sendingMessage) {
            Event e = events.at(0);
            if (state == BootState::BOOT_STATE_NORMAL) {
                sendingMessage = true;
                String message;
                if (e == MOTION_DETECTED) {
                    message = "motion:1";
                } else if (e == MOTION_CLEARED) {
                    message = "motion:0";
                } else if (e == BATTERY_LOW) {
                    message = "battery:low";
                } else if (e == BATTERY_MEDIUM) {
                    message = "battery:medium";
                } else if (e == BATTERY_HIGH) {
                    message = "battery:high";
                } else {
                    return;
                }
                esp_now_send((u8 *) mac, (u8 *) message.c_str(), message.length());
            } else {
                if (e == MOTION_DETECTED) {
                    logger.log("DATA EVENT - ALERT DETECTED");
                } else if (e == MOTION_CLEARED) {
                    logger.log("DATA EVENT - ALERT CLEARED");
                } else if (e == BATTERY_LOW) {
                    logger.log("DATA EVENT - BATTERY LOW");
                } else if (e == BATTERY_MEDIUM) {
                    logger.log("DATA EVENT - BATTERY MEDIUM");
                } else if (e == BATTERY_HIGH) {
                    logger.log("DATA EVENT - BATTERY HIGH");
                }
                delay(100);
                char data[1];
                data[0] = 0x00;
                sendCommand(Command::DATA, data, 1); // ACK only if message sent out
            }
        }
    }
};