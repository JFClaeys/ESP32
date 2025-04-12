// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <WiFi.h>
#include "wifi_credential.h"
#include "ESP32_MCU_Alias.h"
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>


const char* ssid = HOME_WIFI_SSID;
const char* password = HOME_WIFI_PASSWORD;
bool isWifiConnected = false;

bool GetWIFIHasBeenConnected() {
  const uint16_t MAX_DELAY_CONNECT = 5000;
  unsigned long startTime = millis();

  while ((WiFi.status() != WL_CONNECTED) && (millis() - startTime < MAX_DELAY_CONNECT)) {
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
    delay(10);
  }
  return WiFi.status() == WL_CONNECTED;
}

void setup() {
  String MCUName;


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
 
  isWifiConnected = GetWIFIHasBeenConnected();
  
  if (isWifiConnected) {
    MCUName = getCompleteMCUNameFromChipID(getChipIDFromMacAddress());
    ArduinoOTA.setHostname(MCUName.c_str());
    ArduinoOTA.begin();
  }   
}

void loop() {
  if (isWifiConnected) {
    ArduinoOTA.handle();
  }
}
