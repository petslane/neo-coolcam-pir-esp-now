
#pragma once

#include "Arduino.h"
#include <EEPROM.h>

enum BootState: uint8 {
    BOOT_STATE_NORMAL = 0x00,
    BOOT_STATE_AP = 0x01,
    BOOT_STATE_STA = 0x02,
};

struct InternalStateConfig {
    uint8 checksum;
    BootState bootState;
};

class InternalState {
private:
    InternalStateConfig config;

    void save() {
        EEPROM.begin(512);

        EEPROM.write(0, getChecksum());
        EEPROM.write(1, config.bootState);

        EEPROM.commit();
        EEPROM.end();
    }

    void load() {
        EEPROM.begin(512);

        config.checksum = (uint8) EEPROM.read(0);
        config.bootState = (BootState) EEPROM.read(1);

        uint8 checksum = getChecksum();

        if (checksum != config.checksum) {
            config.bootState = BootState::BOOT_STATE_NORMAL;
        }

        EEPROM.end();
    }

    uint8 getChecksum() {
        uint8 checksum = 0x00;

        checksum += (uint8) config.bootState;

        return checksum;
    }

public:
    void setup() {
        load();
    }

    BootState getBootState() {
        return config.bootState;
    }

    void setBootState(BootState state) {
        config.bootState = state;

        save();
    }
};
