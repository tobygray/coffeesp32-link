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
  //client.subscribe("mytopic/test", [] (const String &payload)  {
  //  Serial.println(payload);
  //});

  //client.publish("mytopic/test", "This is a message");
  m_mqtt_setup = true;
}
