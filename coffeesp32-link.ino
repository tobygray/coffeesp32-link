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

#define LOG_TAG "coffeesp32-link"

const int LED_PIN = 2;
const int BLE_SCAN_TIME_S = 5;

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

BLEScan* ble_scan;
std::unique_ptr<BLEAdvertisedDevice> ble_device;

class AdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertised_device) override {
      if (!advertised_device.haveServiceUUID() || !advertised_device.getServiceUUID().equals(CoffeeMachine::kServiceUUID))
      {
        // Skip uninteresting devices.
        return;
      }
      if (machine)
      {
        ESP_LOGI(LOG_TAG, "Ignoring another matching device: %s", advertised_device.toString().c_str());
        return;
      }
      ESP_LOGI(LOG_TAG, "Found matching device: %s", advertised_device.toString().c_str());
      BLEDevice::getScan()->stop();
      ble_device = std::unique_ptr<BLEAdvertisedDevice>(new BLEAdvertisedDevice(advertised_device));
    }
};

AdvertisedDeviceCallbacks advertised_device_callback;

void startBLEScan(bool is_continue);
void publishAutoDiscover();

void bleScanCompleteCb(BLEScanResults results)
{
  ESP_LOGI(LOG_TAG, "BLE scan complete. Devices found: %d", results.getCount());
  ble_scan->clearResults();

  if (!ble_device)
  {
    startBLEScan(true);
  }
}

void startBLEScan(bool is_continue)
{
  if (ble_scan->start(BLE_SCAN_TIME_S, bleScanCompleteCb, is_continue))
  {
    ESP_LOGI(LOG_TAG, "BLE scan started");
  }
  else
  {
    ESP_LOGI(LOG_TAG, "BLE scan start failed");
  }
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);

  //esp_log_level_set("*", ESP_LOG_VERBOSE);

  BLEDevice::init("coffeesp32-link");
  ble_scan = BLEDevice::getScan(); //create new scan
  ble_scan->setAdvertisedDeviceCallbacks(&advertised_device_callback);
  ble_scan->setActiveScan(true);
  ble_scan->setInterval(100);
  ble_scan->setWindow(99);
  
  startBLEScan(false);
}

void onConnectionEstablished() {
  ESP_LOGI(LOG_TAG, "MQTT connection established");
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
  if(ble_device && !machine)
  {
    ESP_LOGI(LOG_TAG, "Found matching device, attempting connect");
    machine = std::unique_ptr<CoffeeMachine>(new CoffeeMachine(mqtt_client, *ble_device));
    publishAutoDiscover(); 
  }
  if (machine)
  {
    machine->loop();
  }
}
