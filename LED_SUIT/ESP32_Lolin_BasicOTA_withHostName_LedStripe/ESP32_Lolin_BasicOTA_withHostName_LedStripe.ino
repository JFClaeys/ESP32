#include <WiFi.h>
#include "wifi_credential.h"
#include "ESP32_MCU_Alias.h"
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>

#include "FastLED.h"
#include <Adafruit_NeoPixel.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define LED_PIN_ONBOARD     7
#define LED_PIN_STRIP      20
#define LED_ONBOARD_COUNT   1
#define LED_STRIP_COUNT    25
#define FASTLED_LED_TYPE   WS2812B
#define FASTLED_VOLTS      5//3.3
#define FASTLED_MAX_MA     700
#define FASTLED_BRIGHTNESS 128
#define MAX_COLOUR 255
#define MIN_BRIGHTNESS 20
#define MAX_BRIGHTNESS 255
#define FASTLED_STRIP_COLOR_ORDER RGB
#define DELAY_STATUS_LED 5
#define DELAY_STRIPE_LEDS 75
#define DELAY_MPU6050_READING 200
#define STATUS_LED_STEP 1

#define COLOUR_RED    0xFF0000
#define COLOUR_GREEN  0x00FF00
#define COLOUR_BLUE   0x0000FF 
#define COLOUR_ORANGE 0xFF4400

Adafruit_MPU6050 mpu;

Adafruit_NeoPixel strip(LED_STRIP_COUNT, LED_PIN_STRIP, NEO_GRB + NEO_KHZ800);
CRGBArray<LED_ONBOARD_COUNT> ledOnBoard;

int  pixelCycle = 0;  // Pattern Pixel Cycle

const char* ssid = HOME_WIFI_SSID;
const char* password = HOME_WIFI_PASSWORD;
bool isWifiConnected = false;
bool isMPUConnected = false;
uint8_t brightnessStatus = FASTLED_BRIGHTNESS; 
uint8_t brightnessStrip = FASTLED_BRIGHTNESS; 
char brightnessStepStatus = STATUS_LED_STEP; // these two willevolve between +1 and -1. a byte cannot go negative.
char brightnessStepStrip = STATUS_LED_STEP;  // thus use a char instead
float accelMagnitude = 0;
float smoothedAccel = 0;

const float ALPHA_SMOOTHING_FACTOR = 0.1;
const float ACCEL_MINIMUM = 9.0;
const float ACCEL_MAXIMUM = 15.0;

//-----------------------------------------------------------//

bool GetWIFIHasBeenConnected() {
  const uint16_t MAX_DELAY_CONNECT = 6000;
  unsigned long startTime = millis();

  while ((WiFi.status() != WL_CONNECTED) && (millis() - startTime < MAX_DELAY_CONNECT)) {
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }  
    delay(10);
  }
  return WiFi.status() == WL_CONNECTED;
}

//-----------------------------------------------------------//

bool GetMPUHasBeenConnected() {
  return mpu.begin();
}

//-----------------------------------------------------------//

uint32_t Wheel(byte WheelPos) {
  WheelPos = MAX_COLOUR - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(MAX_COLOUR - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, MAX_COLOUR - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, MAX_COLOUR - WheelPos * 3, 0);
}

//-----------------------------------------------------------//

uint8_t GetUpdatedBrightness( uint8_t inputBrightness, char &Step  ) {
   inputBrightness += Step;

  // Inverser la direction du changement d'intensité aux limites
  if ((inputBrightness <= 0) || (inputBrightness >= random(230, MAX_BRIGHTNESS))) {
    Step = -Step;
  }
  return inputBrightness;
}

//-----------------------------------------------------------//

void rainbow() {  
  for(uint16_t i=0; i < LED_STRIP_COUNT; i++) {
    strip.setPixelColor(i, Wheel((i + pixelCycle) & MAX_COLOUR)); //  Update delay time  
  }
  strip.setBrightness(brightnessStrip);
  strip.show();                             //  Update strip to match
  pixelCycle++;                             //  Advance current cycle
  if(pixelCycle >= MAX_COLOUR+1)
    pixelCycle = 0;                         //  Loop the cycle back to the begining
  brightnessStrip = GetUpdatedBrightness(brightnessStrip, brightnessStepStrip);
}

//-----------------------------------------------------------//

void setup() {
  String MCUName;

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.clear();
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(MAX_BRIGHTNESS); // Set BRIGHTNESS to about 1/5 (max = 255)

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
  }

  isMPUConnected = GetMPUHasBeenConnected();
  if (isMPUConnected) {
    //initialization of accelerometer data.  calibration?
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  }

  if (isWifiConnected && isMPUConnected) {  //both are connected
    ledOnBoard[0] = COLOUR_GREEN;
  } else {
    if (isWifiConnected && !isMPUConnected) { // only wifi is connected
      ledOnBoard[0] = COLOUR_BLUE;
    } else {
      if (!isWifiConnected && isMPUConnected) {  // only mpu6050 is conmected
         ledOnBoard[0] = COLOUR_ORANGE;
      }
    } // there are no other condition, 
  }   //other than all disconnected, which is alread defined as RED, above
  FastLED.show(); 
}

//-----------------------------------------------------------//

void loop() {
  if (isWifiConnected) {
    ArduinoOTA.handle();
  }

  EVERY_N_MILLISECONDS(DELAY_MPU6050_READING) {
    if (isMPUConnected) {
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);
      // Calculate acceleration magnitude
      accelMagnitude = sqrt(a.acceleration.x * a.acceleration.x + 
                            a.acceleration.y * a.acceleration.y + 
                            a.acceleration.z * a.acceleration.z);

      // Apply smoothing filter
      smoothedAccel = ALPHA_SMOOTHING_FACTOR * accelMagnitude + (1 - ALPHA_SMOOTHING_FACTOR) * smoothedAccel;
  
      // Map acceleration to LED intensity (0-255)
      brightnessStrip = map(constrain(smoothedAccel * 10, ACCEL_MINIMUM * 10, ACCEL_MAXIMUM * 10), 
                          ACCEL_MINIMUM * 10, 
                          ACCEL_MAXIMUM * 10, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
      // on first attempts, it seems the brightness was going toward zero, when motionless.
      // so it appears the system could use a minimum, all time. TBD                    
    }
  }

  EVERY_N_MILLISECONDS(DELAY_STATUS_LED) {
    FastLED.setBrightness(brightnessStatus);
    FastLED.show();
    brightnessStatus = GetUpdatedBrightness(brightnessStatus, brightnessStepStatus);
  }

  EVERY_N_MILLISECONDS(DELAY_STRIPE_LEDS) {
    rainbow();
  }  
}
