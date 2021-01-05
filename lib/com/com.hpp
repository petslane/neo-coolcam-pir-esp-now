
#pragma once

#include "Arduino.h"
#include "common.hpp"
#include "logger.hpp"

#define BUFFER_SIZE 1000
char buffer[BUFFER_SIZE];
uint8 bufferPos = 0;

char const hex_chars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

class Com {
private:
    typedef std::function<void(Command, char *, uint16)> onCallback;
    std::vector<onCallback> onCommandCallbacks;

    unsigned long lastBufferChar = 0;

private:

    void sendBuffer() {
        if (bufferPos == 0) {
            return;
        }

        String string = "Data: ";
        for (int i = 0; i < bufferPos; i++) {
            char const byte = buffer[i];

            if (byte > 32 && byte < 126) {
                string += (String) byte;
            } else {
                string += (String) hex_chars[(byte & 0xF0) >> 4];
                string += (String) hex_chars[(byte & 0x0F) >> 0];
            }
            string += " ";
        }

//    esp_now_send(remoteMac, (u8 *) string.c_str(), string.length());
        bufferPos = 0;
    }

    bool hasCompleteMessage() {
        uint16 len = 0;
        if (bufferPos >= 6) {
            len = (buffer[4] << 8) + buffer[5];
        }
        return bufferPos >= 7 + len;
    }

    void clearCompleteMessage() {
        if (!hasCompleteMessage()) {
            return;
        }

        uint16 len = (buffer[4] << 8) + buffer[5];

        memmove(&buffer[0], &buffer[len + 7], BUFFER_SIZE - (len + 7));

        bufferPos -= len + 7;
    }
    void processIncomingMessage() {
        if (!hasCompleteMessage()) {
            return;
        }

        uint8 command = buffer[3];
        uint16 len = (buffer[4] << 8) + buffer[5];

        logger.log("Received command: " + String((int) command));
        char message[len + 1];
        if (len > 0) {
            memcpy(&message[0], &buffer[6], len);
            message[len] = '\0';

            logger.log("Received message: " + String(message));

            if (command == Command::DATA) {
                String string = "";
                for (int i = 0; i < len; i++) {
                    char const byte = message[i];

                    string += (String) hex_chars[(byte & 0xF0) >> 4];
                    string += (String) hex_chars[(byte & 0x0F) >> 0];

                    string += " ";
                }
                logger.log("Received message HEX: " + String(string));
            }
        }

        std::for_each(onCommandCallbacks.begin(), onCommandCallbacks.end(), [&](onCallback cb) {
            cb((Command) command, message, len);
        });

        clearCompleteMessage();
    }

    void serialReceive() {
        while (Serial.available() > 0) {
            // read the incoming byte:
            char incomingByte = Serial.read();

            buffer[bufferPos] = (char) incomingByte;
            bufferPos++;
            lastBufferChar = millis();

            if (bufferPos == 1 && buffer[0] != (char) 0x55) {
                bufferPos = 0;
            }
            if (bufferPos == 2 && buffer[1] != (char) 0xAA) {
                bufferPos = 0;
            }

            processIncomingMessage();
            break;
        }

        if (millis() - lastBufferChar > 500 && bufferPos > 0) {
            logger.log("buffer reset as position " + String(bufferPos));

            String string = "Data: ";
            for (int i = 0; i < bufferPos; i++) {
                char const byte = buffer[i];

                if (byte > 32 && byte < 126) {
                    string += (String) byte;
                } else {
                    string += (String) hex_chars[(byte & 0xF0) >> 4];
                    string += (String) hex_chars[(byte & 0x0F) >> 0];
                }
                string += " ";
                if (i % 20 == 0 && i > 0) {
                    logger.log(string);
                    string = "continues: ";
                }
            }
            logger.log(string);

            bufferPos = 0;
        }
    }

public:
    void setup() {
        Serial.begin(9600);
    }

    void loop() {
        serialReceive();
    }

    void onCommand(onCallback cb) {
        onCommandCallbacks.push_back(cb);
    }
};