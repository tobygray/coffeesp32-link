#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "EspMQTTClient.h"

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

class AdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertised_device) override {
      if (!advertised_device.haveServiceUUID() || !advertised_device.getServiceUUID().equals(service_uuid))
      {
        // Skip uninteresting devices.
        return;
      }
      Serial.printf("Advertised Device: %s \n", advertised_device.toString().c_str());
    }
};

BLEScan* ble_scan;
AdvertisedDeviceCallbacks advertised_device_callback;


EspMQTTClient client(
  wifi_ssid,
  wifi_password,
  mqtt_server,
  mqtt_username,
  mqtt_password,
  "coffeesp32-link"
);

void startBLEScan(bool is_continue);

void bleScanCompleteCb(BLEScanResults results)
{
  Serial.print("BLE scan complete. Devices found: ");
  Serial.println(results.getCount());
  ble_scan->clearResults();

  // Flash the LED to show that all is well.
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);

  startBLEScan(true);
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

  //client.subscribe("mytopic/test", [] (const String &payload)  {
  //  Serial.println(payload);
  //});

  //client.publish("mytopic/test", "This is a message");
}

void loop()
{
  client.loop();
}
