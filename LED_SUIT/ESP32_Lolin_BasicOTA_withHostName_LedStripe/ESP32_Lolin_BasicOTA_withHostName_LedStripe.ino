#include <WiFi.h>
#include "wifi_credential.h"
#include "ESP32_MCU_Alias.h"
#include <ESPmDNS.h>
#include <NetworkUdp.h>

#include <OneButton.h>
#include <ArduinoOTA.h>

#include "FastLED.h"
#include <Adafruit_NeoPixel.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define LED_PIN_ONBOARD      7
#define LED_PIN_STRIP        1
#define BUTTON_ONOFF_PIN     0
#define BUTTON_INTENSITY_PIN 3
#define MOSFET_CTRL_PIN     21
#define SCL_PIN             10  // mpu6050
#define SDA_PIN              8  // mpu6050
// pin 3, {9, 10,} 18, 19

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

#define DELAY_STRIP_BRIGHTNESS 10
#define DELAY_STATUS_LED 5
#define DELAY_STRIPE_LEDS 20
#define DELAY_MPU6050_READING 200
#define DELAY_BUTTON_READING 1
#define STATUS_LED_STEP 1

#define COLOUR_RED    0xFF0000
#define COLOUR_GREEN  0x00FF00
#define COLOUR_BLUE   0x0000FF 
#define COLOUR_ORANGE 0xFF4400
#define COLOUR_WHITE  0xFFFFFF
#define COLOUR_BLACK  0x000000

#define CLICK_MS_DURATION 200
#define DBLCLICK_MS_DURATION 800

enum LedsActivityStatus {
  LEDS_CYCLING,            // system is normal
  LEDS_NO_CYCLING,         // system is silent.
  LEDS_TO_BE_TURNED_OFF,   // button pressed asked to turn off leds
  LEDS_TURNED_OFF,         // system has turned leds off, i.e: set them black and no mouyre cycling
  LEDS_TO_BE_TURNED_ON,    // button pressed asked to turn on leds
  LEDS_TURNED_ON,          // system has turned leds on, i.e: resumed cycling
  LEDS_PREPARE_FOR_SLEEP,  // future case
};

Adafruit_MPU6050 mpu;      // connected to pins 10 and 8 (SCL / SDA) . no interrupt pin (yet)

Adafruit_NeoPixel strip(LED_STRIP_COUNT, LED_PIN_STRIP, NEO_GRB + NEO_KHZ800);
CRGBArray<LED_ONBOARD_COUNT> ledOnBoard;

int  pixelCycle = 0;  // Pattern Pixel Cycle

const char* ssid = HOME_WIFI_SSID;
const char* password = HOME_WIFI_PASSWORD;
bool isWifiConnected = false;
bool isMPUConnected = false;

uint8_t brightnessStatus = FASTLED_BRIGHTNESS; 
uint8_t brightnessStrip = FASTLED_BRIGHTNESS; 
bool brightnessStripUpdating = true;
char brightnessStepStatus = STATUS_LED_STEP; // these two willevolve between +1 and -1. a byte cannot go negative.
char brightnessStepStrip = STATUS_LED_STEP;  // thus use a char instead

float accelMagnitude = 0;
float smoothedAccel = 0;
CRGB finalColourOfSingleLED;

bool ButtonSaidGoToSleep = false;
bool SleepModeHasBeenActivated = false;

LedsActivityStatus ledSystemStat = LEDS_CYCLING;

const float ALPHA_SMOOTHING_FACTOR = 0.1;
const float ACCEL_MINIMUM = 9.0;
const float ACCEL_MAXIMUM = 15.0;

OneButton button_intensityOnOff;
OneButton button_cyclingOnOff;

//-----------------------------------------------------------//

void onSingleOnOffPressed() {
  switch (ledSystemStat) {
    case LEDS_CYCLING:
      ledSystemStat = LEDS_TO_BE_TURNED_OFF;
      break;
    case LEDS_NO_CYCLING:
      ledSystemStat = LEDS_TO_BE_TURNED_ON;
      break;
  } 
}

//-----------------------------------------------------------//

void onSingleIntensityPressed() {
  brightnessStripUpdating = !brightnessStripUpdating;
}

//-----------------------------------------------------------//

void onSleepModeRequested(void *oneButton){
  if (!SleepModeHasBeenActivated) {
    SleepModeHasBeenActivated = true;  // so... won't be back here...
    // Configuration du réveil par bouton (EXT0)
    // LOW = se réveille quand le bouton est pressé (connecté à GND)
    // errRc = esp_deep_sleep_enable_gpio_wakeup(1ull << WAKEUP_PIN, ESP_GPIO_WAKEUP_GPIO_HIGH);  // $$    <---  precise pinr
    // errRc = esp_deep_sleep_enable_gpio_wakeup(1ull | 0b11111, ESP_GPIO_WAKEUP_GPIO_HIGH);  // all RTC pins can wake 0 to 5
    esp_deep_sleep_enable_gpio_wakeup((1ULL << BUTTON_INTENSITY_PIN), ESP_GPIO_WAKEUP_GPIO_LOW);
    strip.setBrightness(0);        // button pressed asked to turn off leds
    strip.show();
    ledOnBoard[0] = COLOUR_BLACK;
    FastLED.show();
    digitalWrite(MOSFET_CTRL_PIN, LOW);  // eteindre le courant de la led strip
    esp_deep_sleep_start();
  }  
}
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
  if (ledSystemStat != LEDS_CYCLING) {
    return;
  }

  for(uint16_t i=0; i < LED_STRIP_COUNT; i++) {
    strip.setPixelColor(i, Wheel((i + pixelCycle) & MAX_COLOUR)); //  Update delay time
  }
  strip.setBrightness(brightnessStrip);
  strip.show();                             //  Update strip to match
  pixelCycle++;                             //  Advance current cycle
  if(pixelCycle >= MAX_COLOUR+1)
    pixelCycle = 0;                         //  Loop the cycle back to the begining
  //brightnessStrip = GetUpdatedBrightness(brightnessStrip, brightnessStepStrip);
}

//-----------------------------------------------------------//

void setup() {
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  String MCUName;

  /* Given that we can come back to sleep, we need to initialize the vars here, and not in declaration*/
  isWifiConnected = false;
  isMPUConnected = false;
  pixelCycle = 0;

  ButtonSaidGoToSleep = false;
  SleepModeHasBeenActivated = false;
  ledSystemStat = LEDS_CYCLING;
  brightnessStripUpdating = true;

  pinMode(MOSFET_CTRL_PIN, OUTPUT);
  digitalWrite(MOSFET_CTRL_PIN, HIGH);  //open up the power

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
  finalColourOfSingleLED = ledOnBoard[0];
  FastLED.show();

  button_cyclingOnOff.setup(BUTTON_ONOFF_PIN, INPUT_PULLUP, true);
  button_cyclingOnOff.attachClick(onSingleOnOffPressed);
  button_cyclingOnOff.setClickMs(CLICK_MS_DURATION);

  button_intensityOnOff.setup(BUTTON_INTENSITY_PIN, INPUT_PULLUP, true);
  button_intensityOnOff.attachClick(onSingleIntensityPressed);
  button_intensityOnOff.setClickMs(CLICK_MS_DURATION);
  button_intensityOnOff.setLongPressIntervalMs(DBLCLICK_MS_DURATION);
  button_intensityOnOff.attachLongPressStop(onSleepModeRequested, &button_intensityOnOff);
}

//-----------------------------------------------------------//

void loop() {
  if (isWifiConnected) {
    ArduinoOTA.handle();
  }

  EVERY_N_MILLISECONDS(DELAY_BUTTON_READING) {
    button_cyclingOnOff.tick();
    button_intensityOnOff.tick();
  }

  EVERY_N_MILLISECONDS(DELAY_BUTTON_READING) {
    switch (ledSystemStat) {
      case LEDS_CYCLING: break;        // normal
      case LEDS_NO_CYCLING: break;     // system is silent.
      case LEDS_TO_BE_TURNED_OFF:
        strip.setBrightness(0);        // button pressed asked to turn off leds
        strip.show();
        ledSystemStat = LEDS_TURNED_OFF;
        break;
      case LEDS_TURNED_OFF:            // system has turned leds off, i.e: set them black and no more cycling
        ledOnBoard[0] = COLOUR_WHITE;
        FastLED.show();
        ledSystemStat = LEDS_NO_CYCLING;
        break;
      case LEDS_TO_BE_TURNED_ON:       // button pressed asked to turn on leds
        strip.setBrightness(brightnessStrip);
        ledSystemStat = LEDS_TURNED_ON;
        break;
      case LEDS_TURNED_ON:             // system has turned leds on, i.e: resumed cycling
        ledOnBoard[0] = finalColourOfSingleLED;
        FastLED.setBrightness(brightnessStatus);
        FastLED.show();
        ledSystemStat = LEDS_CYCLING;
        break;
      case LEDS_PREPARE_FOR_SLEEP: break;  // future case
    }
  }

  EVERY_N_MILLISECONDS(DELAY_MPU6050_READING) {
    if (isMPUConnected) {
      if (ledSystemStat != LEDS_CYCLING){
        return;
      }

      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);
      // Calculate acceleration magnitude
      accelMagnitude = sqrt(a.acceleration.x * a.acceleration.x +
                            a.acceleration.y * a.acceleration.y +
                            a.acceleration.z * a.acceleration.z);

      // Apply smoothing filter
      smoothedAccel = ALPHA_SMOOTHING_FACTOR * accelMagnitude + (1 - ALPHA_SMOOTHING_FACTOR) * smoothedAccel;
  
      // Map acceleration to LED intensity (0-255)
      //brightnessStrip = map(constrain(smoothedAccel * 10, ACCEL_MINIMUM * 10, ACCEL_MAXIMUM * 10),
      //                    ACCEL_MINIMUM * 10, 
      //                    ACCEL_MAXIMUM * 10, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
      // on first attempts, it seems the brightness was going toward zero, when motionless.
      // so it appears the system could use a minimum, all time. TBD
    }
  }

  EVERY_N_MILLISECONDS(DELAY_STATUS_LED) {
    if (ledSystemStat == LEDS_CYCLING) {
      brightnessStatus = GetUpdatedBrightness(brightnessStatus, brightnessStepStatus);
      FastLED.setBrightness(brightnessStatus);
      FastLED.show();
    }  
  }

  EVERY_N_MILLISECONDS(DELAY_STRIP_BRIGHTNESS){
    if (ledSystemStat == LEDS_CYCLING) {
      if (brightnessStripUpdating) {
        brightnessStrip = GetUpdatedBrightness(brightnessStrip, brightnessStepStrip);
      } else {
        brightnessStrip = MAX_BRIGHTNESS;
      }  
    }
  }

  EVERY_N_MILLISECONDS(DELAY_STRIPE_LEDS) {
    rainbow();
  }
}
