
#pragma once

#include "Arduino.h"
#include <WiFiUdp.h>
#include <Syslog.h>

WiFiUDP udpClient;
IPAddress logIP = IPAddress(192,168,0,29);
Syslog syslog(udpClient, logIP, 514, "pir", "pir", LOG_KERN);

class Logger {
public:
    void log(const String &message) {
        syslog.log(LOG_INFO, message);
    }
};

Logger logger;
