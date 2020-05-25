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

#include <BLEAdvertisedDevice.h>

#include <string>

// Forward Declare
class EspMQTTClient;

class CoffeeMachine {
public:
  CoffeeMachine(EspMQTTClient & mqtt_client, BLEAdvertisedDevice & device);
  // Should be called on every iteration of the main event loop.
  void loop();

  // Setups up autodiscovery values in MQTT and subscribes to appropriate values.
  //
  // Should be called once when the MQTT connection is established.
  void setupMqtt();
 
private:
  EspMQTTClient & m_mqtt_client;
  const BLEAddress m_address;
  const std::string m_name;
  bool m_mqtt_setup;
};
