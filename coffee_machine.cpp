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

#include <EspMQTTClient.h>

#include <sstream>

CoffeeMachine::CoffeeMachine(EspMQTTClient & mqtt_client, BLEAdvertisedDevice & device)
  : m_mqtt_client(mqtt_client)
  , m_address(device.getAddress())
  , m_name(device.haveName() ? device.getName() : "unknown")
  , m_mqtt_setup(false)
{
}

void CoffeeMachine::loop()
{
}

void CoffeeMachine::setupMqtt()
{
  if (m_mqtt_setup)
  {
    return;
  }
  // Publish the MQTT auto-discover config for Home Assistant:
  // https://www.home-assistant.io/docs/mqtt/discovery/
  const auto topic_root = std::string("homeassistant/switch/coffeesp32-link/") + m_name;
  const auto config_topic = topic_root + "/config";
  const auto command_topic = topic_root + "/command";
  // JSON object that needs to match config for https://www.home-assistant.io/integrations/switch.mqtt/.
  auto config_oss = std::ostringstream{};
  config_oss << "{"
             << "\"name\": \"Coffee Machine\","
             << "\"command_topic\": \"" << command_topic << "\","
             << "\"icon\": \"mdi:coffee-maker\""
             << "}";
  // TODO: add support for state_topic and availability_topic.
  m_mqtt_client.publish(config_topic.c_str(), config_oss.str().c_str());

  m_mqtt_client.subscribe(command_topic.c_str(), [this] (const String & payload)  {
    switchCommand(payload);
  });
  m_mqtt_setup = true;
}

void CoffeeMachine::switchCommand(const String & payload)
{
  // TODO implement the BLE commands.
  Serial.printf("Got command %s for %s\n", payload, m_name.c_str());
}
