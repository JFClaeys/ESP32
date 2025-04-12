#include <WiFi.h>
#include "wifi_credential.h"
#include "ESP32_MCU_Alias.h"
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>

#include "FastLED.h"

#define LED_PIN_ONBOARD   7
#define LED_ONBOARD_COUNT 1
#define FASTLED_LED_TYPE   WS2812B
#define FASTLED_VOLTS      5//3.3
#define FASTLED_MAX_MA     700
#define FASTLED_BRIGHTNESS 128
#define FASTLED_SATURATION 255 
#define FASTLED_STRIP_COLOR_ORDER RGB

#define COLOUR_RED    0xFF0000
#define COLOUR_GREEN  0x00FF00
#define COLOUR_BLUE   0x0000FF 
#define COLOUR_ORANGE 0xFF4400

CRGBArray<LED_ONBOARD_COUNT> ledOnBoard;
unsigned long previousMillis = 0;
unsigned long currentMillis;

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

  FastLED.setMaxPowerInVoltsAndMilliamps(FASTLED_VOLTS, FASTLED_MAX_MA);
  FastLED.addLeds<FASTLED_LED_TYPE, LED_PIN_ONBOARD, FASTLED_STRIP_COLOR_ORDER>(ledOnBoard, LED_ONBOARD_COUNT);
  ledOnBoard[0] = COLOUR_RED;
  FastLED.show();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
 
  isWifiConnected = GetWIFIHasBeenConnected();
  
  if (isWifiConnected) {
    MCUName = getCompleteMCUNameFromChipID(getChipIDFromMacAddress());
    ArduinoOTA.setHostname(MCUName.c_str());
    ArduinoOTA.begin();
    ledOnBoard[0] = COLOUR_BLUE;
    FastLED.show(); 
  } else {
    ledOnBoard[0] = COLOUR_ORANGE;
    FastLED.show();
  }   
}

uint8_t brightness = 128;
int brightnessStep = 1;
void loop() {
  if (isWifiConnected) {
    ArduinoOTA.handle();
  }

  EVERY_N_MILLISECONDS(5) {
    FastLED.setBrightness(brightness);
    FastLED.show();
    brightness += brightnessStep;

    // Inverser la direction du changement d'intensité aux limites
    if (brightness <= 0 || brightness >= 255) {
      brightnessStep = -brightnessStep;
    }
  }
}
