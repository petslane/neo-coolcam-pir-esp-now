# Neo Coolcam PIR

This is Work In Progress firmware of Neo Coolcam PIR (WiFi version) that uses ESP-NOW messages instead of WiFi to send events.

ESP-NOW messages can be received with [petslane/esp-now-gateway](https://github.com/petslane/esp-now-gateway) that publishes received events in MQTT.

## Flash 

This project uses [PlatformIO](https://platformio.org/). Change `build_flags` in `platformio.ini` before uploading.

First upload must be done using serial programmer, but next uploads can be done using OTA over WiFi if PIR is set to AP/STA mode with button next to battery. After OTA upload, PIR restarts in normal mode - ESP-NOW mode.

## Battery life

This firmware was created to:
- remove need for PIR sensor to know anything about WiFi network or MQTT server
- have better battery life by keeping ESP8266 run time shorter using ESP-NOW communication instead of connecting to WiFi network and MQTT server to send messages
- have full control of device and not relay on external services

Original manual stated that battery lasts for “2500 times trigger”, this probably means 1 trigger time is 1 event for movement detected and another event about minute later for movement stop. So total of 5000 events/ESP8266 wakeups.

Using this firmware and placing PIR somewhere with lot of movement (in average 284 events or 142 new movement detections per day) I managed to get total of 24428 events within 3 months until battery died. It's about 5 times more events/movements that original firmware promised. Original firmware would have probably lasted less than a month in similar environment with constant movements.
