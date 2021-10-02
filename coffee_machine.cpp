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

#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>

#include <sstream>

#define LOG_TAG "CoffeeMachine"

// This seems to be a generic UUID used by RN4020 BLE chip: http://ww1.microchip.com/downloads/en/devicedoc/70005191b.pdf
// So it seems it's not been changed for the coffee machines.
const BLEUUID CoffeeMachine::kServiceUUID = BLEUUID("00035b03-58e6-07dd-021a-08123a000300");
// The main control (maybe all?) is done via this INDICATE/READ/WRITE characteristic.
const BLEUUID CoffeeMachine::kCharacteristicUUID = BLEUUID("00035b03-58e6-07dd-021a-08123a000301");
// There's a mysterious single byte READ/WRITE charateristic that doesn't seem to do much.
const BLEUUID CoffeeMachine::kCharacteristic2UUID = BLEUUID("00035b03-58e6-07dd-021a-08123a0003ff");

namespace {
  
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    ESP_LOGI(LOG_TAG, "Notify callback for characteristic %s, length %d", pBLERemoteCharacteristic->getUUID().toString().c_str(), length);
    // TODO: Dump Data
}
void add_checksum(std::vector<unsigned char> & payload)
{
  unsigned int checksum = 7439;
  for(auto i = 0; i < payload.size(); i++)
  {
    checksum = ((checksum << 8 | ((checksum >> 8) & 0xFF)) & 0xFFFF) ^ (payload[i] & 0xFF);
    checksum ^= (checksum & 0xFF) >> 4;
    checksum ^= (checksum << 12) & 0xFFFF;
    checksum ^= ((checksum & 0xFF) << 5) & 0xFFFF;
  }
  payload.push_back((checksum >> 8) & 0xFF);
  payload.push_back(checksum & 0xFF);
}
} // anonymous namespace

CoffeeMachine::CoffeeMachine(EspMQTTClient & mqtt_client, BLEAdvertisedDevice & device)
  : m_mqtt_client(mqtt_client)
  , m_address(device.getAddress())
  , m_name(device.haveName() ? device.getName() : "unknown")
  , m_ble_client(BLEDevice::createClient())
  , m_ble_service()
  , m_ble_characteristic()
  , m_mqtt_setup(false)
  , m_ble_connected(false)
{
   m_ble_client->setClientCallbacks(this);
   ESP_LOGI(LOG_TAG, "Connecting..");
   m_ble_client->connect(&device);
   ESP_LOGI(LOG_TAG, "Getting service...");
   m_ble_service = m_ble_client->getService(kServiceUUID);
   if (!m_ble_service)
   {
    ESP_LOGI(LOG_TAG, "Failed to find remote service");
    m_ble_client->disconnect();
    return;
   }
   ESP_LOGI(LOG_TAG, "Getting characteristic...");
   m_ble_characteristic = m_ble_service->getCharacteristic(kCharacteristicUUID);
   if (!m_ble_characteristic)
   {
    ESP_LOGI(LOG_TAG, "Failed to find remote characteristic");
    m_ble_client->disconnect();
    return;
   }

   m_ble_characteristic->registerForNotify(notifyCallback, false);
   ESP_LOGI(LOG_TAG, "Constructed coffee machine");
}

CoffeeMachine::~CoffeeMachine()
{
}

void CoffeeMachine::loop()
{
  if (m_ble_characteristic)
  {
    auto data = m_ble_characteristic->readValue();
    auto all_null = true;
    for (int i = 0; i < data.length(); ++i)
    {
      if (data[i] != 0)
      {
        all_null = false;
        break;
      }
    }
    if (all_null)
    {
      return;
    }
    ESP_LOGI(LOG_TAG, "Data is of length: %d", data.length());
    // TODO dump data
  }
}

void CoffeeMachine::setupMqtt()
{
  if (m_mqtt_setup)
  {
    return;
  }
  ESP_LOGI(LOG_TAG, "Starting MQTT setup");
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
  ESP_LOGI(LOG_TAG, "Finished MQTT setup");
  m_mqtt_setup = true;
}

void CoffeeMachine::switchCommand(const String & payload)
{
  if (payload.equals("ON"))
  {
    ESP_LOGI(LOG_TAG, "Attempting to turn device on");
    auto payload = std::vector<unsigned char>{0x0d, 0x07, 0x84, 0x0f, 0x02, 0x01};
    add_checksum(payload);
    ESP_LOGI(LOG_TAG, "Sending %d bytes", payload.size());
    m_ble_characteristic->writeValue(reinterpret_cast<uint8_t*>(payload.data()), payload.size(), true);
  }
  else if (payload.equals("OFF"))
  {  
   // TODO implement
   ESP_LOGI(LOG_TAG, "Asked to turn off %s, but not implemented", m_name.c_str()); 
  }
  else
  {
    ESP_LOGI(LOG_TAG, "Got unknown command %s for %s", payload, m_name.c_str());
  }
}

void CoffeeMachine::onConnect(BLEClient* pclient)
{
  ESP_LOGI(LOG_TAG, "Connected to BLE client");
  m_ble_connected = true;
}

void CoffeeMachine::onDisconnect(BLEClient* pclient)
{
  ESP_LOGI(LOG_TAG, "Disconnected from BLE client");
  m_ble_connected = false;  
}
