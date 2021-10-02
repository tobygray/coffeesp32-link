# coffeesp32-link
ESP32 Bridge to Control DeLonghi Coffee Machines.

Initially developed for a DeLonghi Dinamica plus.

![Delonghi Dinamica plus](delonghi.jpg)

Requires [Arduino IDE](https://www.arduino.cc/) with ESP32 board supported, added via preferences as an additional board URL: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

Uses these libraries:
* https://github.com/plapointe6/EspMQTTClient v1.8.0
* https://pubsubclient.knolleary.net/ v2.8.0

Having BLE and MQTT inside the app makes it quite large, try one of the no-OTA flash schemes (such as a 2MB APP one).

Useful links:
* https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/BLEDevice.cpp
* https://github.com/manekinekko/cafy/blob/main/src/commands.ts
