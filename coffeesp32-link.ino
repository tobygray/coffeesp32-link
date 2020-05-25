/* Copyright (C) 2020, Toby Gray <toby.gray@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */ 
#include "coffee_machine.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <EspMQTTClient.h>

#include <memory>

// secrets.h needs to hold the following information:
// 
// const char* wifi_ssid = "NetworkName";
// const char* wifi_password = "PasswordForWifi";
// const char* mqtt_server = "mqtt-server-hostname-or-ip";
// const char* mqtt_username = "username"; // Can be NULL.
// const char* mqtt_password = "PasswordForMQTT"; // Can be NULL.
// const int machine_pin = 1234;
#include "secrets.h"

// This seems to be a generic UUID used by RN4020 BLE chip: http://ww1.microchip.com/downloads/en/devicedoc/70005191b.pdf
// So it seems it's not been changed for the coffee machines.
const auto service_uuid = BLEUUID("00035b03-58e6-07dd-021a-08123a000300");

const int LED_PIN = 2;
const int BLE_SCAN_TIME_S = 15;

EspMQTTClient mqtt_client(
  wifi_ssid,
  wifi_password,
  mqtt_server,
  mqtt_username,
  mqtt_password,
  "coffeesp32-link"
);

// Single coffee machine of interest, because who would want to control two of these things in the same house?
std::unique_ptr<CoffeeMachine> machine;

class AdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertised_device) override {
      if (!advertised_device.haveServiceUUID() || !advertised_device.getServiceUUID().equals(service_uuid))
      {
        // Skip uninteresting devices.
        return;
      }
      if (machine)
      {
        Serial.printf("Ignoring another matching device: %s\n", advertised_device.toString().c_str());
        return;
      }
      Serial.printf("Found matching device: %s \n", advertised_device.toString().c_str());
      machine = std::unique_ptr<CoffeeMachine>(new CoffeeMachine(mqtt_client, advertised_device));
    }
};

BLEScan* ble_scan;
AdvertisedDeviceCallbacks advertised_device_callback;

void startBLEScan(bool is_continue);
void publishAutoDiscover();

void bleScanCompleteCb(BLEScanResults results)
{
  Serial.print("BLE scan complete. Devices found: ");
  Serial.println(results.getCount());
  ble_scan->clearResults();

  if (!machine)
  {
    startBLEScan(true);
  }
  else
  {
    Serial.println("Found matching device, stopping BLE scan");
    publishAutoDiscover();
  }
}

void startBLEScan(bool is_continue)
{
  if (ble_scan->start(BLE_SCAN_TIME_S, bleScanCompleteCb, is_continue))
  {
    Serial.println("BLE scan started");
  }
  else
  {
    Serial.println("BLE scan start failed");
  }
}

void setup()
{  
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  BLEDevice::init("coffeesp32-link");
  ble_scan = BLEDevice::getScan(); //create new scan
  ble_scan->setAdvertisedDeviceCallbacks(&advertised_device_callback);
  ble_scan->setActiveScan(true);
  ble_scan->setInterval(100);
  ble_scan->setWindow(99);
  
  startBLEScan(false);
}

void onConnectionEstablished() {
  Serial.println("MQTT connection established");
  publishAutoDiscover();
}

void publishAutoDiscover()
{
  if (!machine)
  {
    // No matching BLE device found yet.
    return;
  }
  if (!mqtt_client.isConnected())
  {
    // Not connected to MQTT yet, so can't publish anything.
    return;
  }
  machine->setupMqtt();
}

void loop()
{
  mqtt_client.loop();
  if (machine)
  {
    machine->loop();
  }
}
