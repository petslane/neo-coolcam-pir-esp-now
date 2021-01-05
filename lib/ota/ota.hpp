
#pragma once

#include "Arduino.h"

#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

class OTA {
private:
    typedef std::function<void(void)> onCallback;
    std::vector<onCallback> onStartCallbacks;
    std::vector<onCallback> onEndCallbacks;
public:
    void setup() {
        ArduinoOTA.setHostname("pir");

        ArduinoOTA.onStart([&]() {
            Serial.println("Start");
            std::for_each(onStartCallbacks.begin(), onStartCallbacks.end(), [](onCallback cb) { cb(); });
        });
        ArduinoOTA.onEnd([&]() {
            Serial.println("\nEnd");
            std::for_each(onEndCallbacks.begin(), onEndCallbacks.end(), [](onCallback cb) { cb(); });
            delay(1000);
            ESP.restart();
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");
        });

        ArduinoOTA.begin();
    }

    void loop() {
        ArduinoOTA.handle();
    }

    void onStart(onCallback cb) {
        onStartCallbacks.push_back(cb);
    }
    void onEnd(onCallback cb) {
        onEndCallbacks.push_back(cb);
    }
};
