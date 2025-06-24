/*
    ESP-NOW Broadcast Slave
    Lucas Saavedra Vaz - 2024

    This sketch demonstrates how to receive broadcast messages from a master device using the ESP-NOW protocol.

    The master device will broadcast a message every 5 seconds to all devices within the network.

    The slave devices will receive the broadcasted messages. If they are not from a known master, they will be registered as a new master
    using a callback function.
*/

#include "ESP32_NOW.h"
#include "WiFi.h"

#include <esp_mac.h>  // For the MAC2STR and MACSTR macros

#include <vector>
#include "FastLED.h"
#include "ESP32_Broadcasts.h"

/* Definitions */
#define ESPNOW_WIFI_CHANNEL 1

#define LED_TYPE   WS2812B
#define ONBOARD_COLOR_ORDER GRB
#define OUTSIDE_COLOR_ORDER RGB

#define VOLTS          3.3
#define MAX_MA       700
#define BRIGHTNESS 255   /* Control the brightness of your leds */
#define SATURATION 255   /* Control the saturation of your leds */

#define ONBOARD_LED_PIN 7
#define OUTSIDE_DATA_PIN      6
#define ON_BOARD_NUM_LEDS 1

/* variables */
CRGBArray<ON_BOARD_NUM_LEDS> leds;
CRGBArray<ON_BOARD_NUM_LEDS> led_onBoard;
bool message_has_been_received = true;
byte message_count_down = 0;
byte HSVLooping = 0;
int32_t Wifi_channel; // get current channel in order to set it for ESPnow. i womnder why...

/* Classes */

// Creating a new class that inherits from the ESP_NOW_Peer class is required.

class ESP_NOW_Peer_Class : public ESP_NOW_Peer {
public:
  // Constructor of the class
  ESP_NOW_Peer_Class(const uint8_t *mac_addr, uint8_t channel, wifi_interface_t iface, const uint8_t *lmk) : ESP_NOW_Peer(mac_addr, channel, iface, lmk) {}

  // Destructor of the class
  ~ESP_NOW_Peer_Class() {}

  // Function to register the master peer
  bool add_peer() {
    if (!add()) {
      log_e("Failed to register the broadcast peer");
      return false;
    }
    return true;
  }

  // Function to print the received messages from the master
  void onReceive(const uint8_t *data, size_t len, bool broadcast) {
    leds[0] = CRGB(0,0,0);
    FastLED.show();
    TExchangeDataPacket aDatapacket;
    //Serial.printf("Received a message from master " MACSTR " (%s)\n", MAC2STR(addr()), broadcast ? "broadcast" : "unicast");
    //Serial.printf("  Message: %s\n", (char *)data);

    memcpy(&aDatapacket, data, sizeof(aDatapacket));
    HSVLooping = aDatapacket.ColorHueToDisplay;
    message_has_been_received = true;
    message_count_down = 10;
    leds[0] = CRGB(255,0,0);
    FastLED.show();

  }
};

/* Global Variables */

// List of all the masters. It will be populated when a new master is registered
std::vector<ESP_NOW_Peer_Class> masters;

/* Callbacks */

// Callback called when an unknown peer sends a message
void register_new_master(const esp_now_recv_info_t *info, const uint8_t *data, int len, void *arg) {
  if (memcmp(info->des_addr, ESP_NOW.BROADCAST_ADDR, 6) == 0) {
    leds[0] = CRGB(0,0,0);
    FastLED.show();
    //Serial.printf("Unknown peer " MACSTR " sent a broadcast message\n", MAC2STR(info->src_addr));
    //Serial.println("Registering the peer as a master");
    delay(500);
    ESP_NOW_Peer_Class new_master(info->src_addr, Wifi_channel, WIFI_IF_STA, NULL);
    leds[0] = CRGB(0,0,255);
    FastLED.show();
    delay(500);
    masters.push_back(new_master);
    if (!masters.back().add_peer()) {
      //Serial.println("Failed to register the new master");
      return;
    }
  } else {
    leds[0] = CRGB(0,255,0);
    FastLED.show();
    delay(500);
    // The slave will only receive broadcast messages
    log_v("Received a unicast message from " MACSTR, MAC2STR(info->src_addr));
    log_v("Igorning the message");
  }
}

/* Main */

void setup() {
  //Serial.begin(115200);
 // while (!Serial) {
  //  delay(10);
 // }

  FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MAX_MA);
  FastLED.addLeds<LED_TYPE, ONBOARD_LED_PIN, ONBOARD_COLOR_ORDER>(led_onBoard, ON_BOARD_NUM_LEDS);  
  //FastLED.addLeds<LED_TYPE, OUTSIDE_DATA_PIN, OUTSIDE_COLOR_ORDER>(leds, ON_BOARD_NUM_LEDS);
  led_onBoard[0] = CRGB(255,0,0);  // red, we do not have wifi yet
  leds[0] = CRGB(0,0,0);

  FastLED.show();
  // Initialize the Wi-Fi module
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) {
    delay(100);
  }

  led_onBoard[0] = CRGB(0,0,255);  // blue, it seems we do
  FastLED.show();
  Wifi_channel = ESPNOW_WIFI_CHANNEL; 
  //Serial.println("ESP-NOW Example - Broadcast Slave");
  //Serial.println("Wi-Fi parameters:");
  //Serial.println("  Mode: STA");
  //Serial.println("  MAC Address: " + WiFi.macAddress());
  //Serial.printf("  Channel: %d\n", ESPNOW_WIFI_CHANNEL);

  // Initialize the ESP-NOW protocol
  if (!ESP_NOW.begin()) {
    //Serial.println("Failed to initialize ESP-NOW");
    //Serial.println("Reeboting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }
  led_onBoard[0] = CRGB(0,255,0);  // green, it seems esp now has been initialized
  FastLED.show();
  // Register the new peer callback
  ESP_NOW.onNewPeer(register_new_master, NULL);

  //Serial.println("Setup complete. Waiting for a master to broadcast a message...");
}

byte j = 0; 
void loop() {
  
  EVERY_N_MILLIS(50) {
    if (message_count_down != 0) {
      led_onBoard[0] = CRGB(255,255,255); // white, data hase been received
      //message_has_been_received = false;
      message_count_down--;
    } else {
      led_onBoard[0] = CHSV((HSVLooping * 2), SATURATION, BRIGHTNESS); /* The higher the value 4 the less fade there is and vice versa */ 
      HSVLooping++;
    }  
  }

  EVERY_N_MILLIS(10) {
    leds[0] = CHSV((j * 2), SATURATION, BRIGHTNESS); /* The higher the value 4 the less fade there is and vice versa */ 
    j++;
  }

  FastLED.show(); 
}
