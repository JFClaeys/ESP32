/*
    ESP-NOW Broadcast Master
    Lucas Saavedra Vaz - 2024

    This sketch demonstrates how to broadcast messages to all devices within the ESP-NOW network.
    This example is intended to be used with the ESP-NOW Broadcast Slave example.

    The master device will broadcast a message every 5 seconds to all devices within the network.
    This will be done using by registering a peer object with the broadcast address.

    The slave devices will receive the broadcasted messages and print them to the Serial Monitor.
*/

#include "ESP32_NOW.h"
#include "WiFi.h"

#include <esp_mac.h>  // For the MAC2STR and MACSTR macros

/* Definitions */

#define ESPNOW_WIFI_CHANNEL 6
#define LED_PIN_OUT 22 
#define TIME_LAPSE_BETWEEN_BROADCAST 1500
#define TIME_LAPSE_LED_CUTOFF 150

/* Classes */

// Creating a new class that inherits from the ESP_NOW_Peer class is required.

class ESP_NOW_Broadcast_Peer : public ESP_NOW_Peer {
public:
  // Constructor of the class using the broadcast address
  ESP_NOW_Broadcast_Peer(uint8_t channel, wifi_interface_t iface, const uint8_t *lmk) : ESP_NOW_Peer(ESP_NOW.BROADCAST_ADDR, channel, iface, lmk) {}

  // Destructor of the class
  ~ESP_NOW_Broadcast_Peer() {
    remove();
  }

  // Function to properly initialize the ESP-NOW and register the broadcast peer
  bool begin() {
    if (!ESP_NOW.begin() || !add()) {
      log_e("Failed to initialize ESP-NOW or register the broadcast peer");
      return false;
    }
    return true;
  }

  // Function to send a message to all devices within the network
  bool send_message(const uint8_t *data, size_t len) {
    digitalWrite(LED_PIN_OUT, HIGH);
    if (!send(data, len)) {
      log_e("Failed to broadcast message");
      digitalWrite(LED_PIN_OUT, LOW);
      return false;
    }
    return true;    
  }
};

/* Global Variables */
unsigned long previousMillis = 0;
unsigned long currentMillis;
unsigned long NextWaitToBroadcast = TIME_LAPSE_BETWEEN_BROADCAST;
uint32_t msg_count = 0;

// Create a broadcast peer object
ESP_NOW_Broadcast_Peer broadcast_peer(ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, NULL);

/* Main */

void setup() {
  pinMode(LED_PIN_OUT, OUTPUT);
  digitalWrite(LED_PIN_OUT, HIGH);

  // Initialize the Wi-Fi module
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) {
    delay(100);
  }

   // Register the broadcast peer
  if (!broadcast_peer.begin()) {
    delay(5000);
    ESP.restart();
  }

  digitalWrite(LED_PIN_OUT, LOW);

}

void loop() {
  currentMillis = millis();

  if (currentMillis - previousMillis >= NextWaitToBroadcast) {
    previousMillis = currentMillis;
    NextWaitToBroadcast = random(15,40) * 100;
    // Broadcast a message to all devices within the network
    char data[32];
    snprintf(data, sizeof(data), "Hello, World! #%lu", msg_count++);

    if (!broadcast_peer.send_message((uint8_t *)data, sizeof(data))) { 
      
    }

  } else {
    if (currentMillis - previousMillis >= TIME_LAPSE_LED_CUTOFF) {
      digitalWrite(LED_PIN_OUT, LOW);  
    }  
  } 
  
}
