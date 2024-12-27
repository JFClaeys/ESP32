#include "FastLED.h"

#define LED_TYPE   WS2812B
#define ONBOARD_COLOR_ORDER GRB
#define OUTSIDE_COLOR_ORDER RGB

#define VOLTS          3.3
#define MAX_MA       700
#define BRIGHTNESS 255   /* Control the brightness of your leds */
#define SATURATION 255   /* Control the saturation of your leds */

#define ONBOARD_LED_PIN 7
#define OUTSIDE_DATA_PIN      10
#define ON_BOARD_NUM_LEDS 1

CRGBArray<ON_BOARD_NUM_LEDS> leds;
CRGBArray<ON_BOARD_NUM_LEDS> led_onBoard;

void setup() {
  delay(1000); // safety startup delay
  FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MAX_MA);
  FastLED.addLeds<LED_TYPE, ONBOARD_LED_PIN, ONBOARD_COLOR_ORDER>(led_onBoard, ON_BOARD_NUM_LEDS);  
  FastLED.addLeds<LED_TYPE, OUTSIDE_DATA_PIN, OUTSIDE_COLOR_ORDER>(leds, ON_BOARD_NUM_LEDS);
  
}
byte j = 0; 
void loop() {

  EVERY_N_MILLIS(10) {
    leds[0] = CHSV((j * 2), SATURATION, BRIGHTNESS); /* The higher the value 4 the less fade there is and vice versa */ 
    led_onBoard[0] = CHSV((j * 2), SATURATION, BRIGHTNESS); /* The higher the value 4 the less fade there is and vice versa */ 
    j++;
  }
FastLED.show();  
}
