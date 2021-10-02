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
#pragma once

#include <BLEClient.h>
#include <BLEUUID.h>

#include <WString.h>
#include <memory>
#include <string>

// Forward Declare
class BLEAdvertisedDevice;
class EspMQTTClient;

class CoffeeMachine : public BLEClientCallbacks {
public:
  static const BLEUUID kServiceUUID;
  static const BLEUUID kCharacteristicUUID;
  static const BLEUUID kCharacteristic2UUID;
  CoffeeMachine(EspMQTTClient & mqtt_client, BLEAdvertisedDevice & device);

  ~CoffeeMachine();
  
  // Should be called on every iteration of the main event loop.
  void loop();

  // Setups up autodiscovery values in MQTT and subscribes to appropriate values.
  //
  // Should be called once when the MQTT connection is established.
  void setupMqtt();

  // From BLEClientCallbacks
  void onConnect(BLEClient* pclient) override;
  void onDisconnect(BLEClient* pclient) override;
private:
  // Called when the MQTT command state changes for the switch representing the machine.
  void switchCommand(const String & payload);
 
private:
  EspMQTTClient & m_mqtt_client;
  const BLEAddress m_address;
  const std::string m_name;
  std::unique_ptr<BLEClient> m_ble_client;
  BLERemoteService* m_ble_service; // Owned by m_ble_client.
  BLERemoteCharacteristic* m_ble_characteristic; // Owned by m_ble_service (aka. m_ble_client).
  bool m_mqtt_setup;
  bool m_ble_connected;
};
