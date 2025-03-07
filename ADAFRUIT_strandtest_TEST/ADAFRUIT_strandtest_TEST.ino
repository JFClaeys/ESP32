// A basic everyday NeoPixel strip test program.

// NEOPIXEL BEST PRACTICES for most reliable operation:
// - Add 1000 uF CAPACITOR between NeoPixel strip's + and - connections.
// - MINIMIZE WIRING LENGTH between microcontroller board and first pixel.
// - NeoPixel strip's DATA-IN should pass through a 300-500 OHM RESISTOR.
// - AVOID connecting NeoPixels on a LIVE CIRCUIT. If you must, ALWAYS
//   connect GROUND (-) first, then +, then data.
// - When using a 3.3V microcontroller with a 5V-powered NeoPixel strip,
//   a LOGIC-LEVEL CONVERTER on the data line is STRONGLY RECOMMENDED.
// (Skipping these may work OK on your workbench but can fail in the field)

#include <Adafruit_NeoPixel.h>
#include "FastLED.h"

/*required OTA includes*/
#include <WiFi.h>
#include "wifi_credential.h"
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>
/*required OTA includes*/

#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN_SINGLE 20
#define LED_PIN_DENSE_STRIP 10
#define LED_PIN_MEDIUM_STRIP 1
#define LED_PIN_ONBOARD   7

#define COLOR_STRIP_DELAY 500

/*fastled*/
#define FASTLED_DENSE_LED_TYPE   WS2812B
#define FASTLED_MEDIUM_LED_TYPE   WS2812
#define FASTLED_VOLTS      5//3.3
#define FASTLED_MAX_MA     700
#define FASTLED_BRIGHTNESS 128
#define FASTLED_SATURATION 255 
#define FASTLED_STRIP_COLOR_ORDER GRB

#define RUN_ON_BOARD
#define RUN_SINGLE
//#define RUN_MEDIUM
#define RUN_MEDIUM_RANDOM
//#define RUN_MEDIUM_FASTLED
//#define RUN_DENSE
#define RUN_DENSE_AS_FASTLED


#if defined RUN_MEDIUM && defined RUN_MEDIUM_AS_FASTLED
  /* same as below
  */
  #undef RUN_MEDIUM_AS_FASTLED
#endif  

#if defined RUN_DENSE && defined RUN_DENSE_AS_FASTLED
  /* basically, you cannot run both.  so either one.
     given RUN_DENSE is higher, it would imply an error,
     thus removing the later rather then the former
  */
  #undef RUN_DENSE_AS_FASTLED
#endif  

#define RUN_RED
#define RUN_GREEN
#define RUN_BLUE

#define COLOUR_RED   0xFF0000
#define COLOUR_GREEN 0x00FF00
#define COLOUR_BLUE  0x0000FF // medium seems to be able to light blue when it's at 0F, contralri tothe pothers, whihch do at 08

// How many NeoPixels are attached to the Arduino?
#define LED_SINGLE_COUNT 1
#define LED_ONBOARD_COUNT 1
#define LED_STRIP_COUNT 50
//  GRB(nope)  GBR  RGB  RBG  BGR BRG
// Declare our NeoPixel strip object:
#ifdef RUN_ON_BOARD
Adafruit_NeoPixel onboard(LED_ONBOARD_COUNT, LED_PIN_ONBOARD, NEO_RGB + NEO_KHZ400);
#endif

#ifdef RUN_SINGLE
Adafruit_NeoPixel single(LED_SINGLE_COUNT, LED_PIN_SINGLE, NEO_GRB + NEO_KHZ800);
#endif

#if defined(RUN_MEDIUM) || defined(RUN_MEDIUM_RANDOM)
//#ifdef RUN_MEDIUM
  Adafruit_NeoPixel stripMedium(LED_STRIP_COUNT, LED_PIN_MEDIUM_STRIP, NEO_GRB + NEO_KHZ800);
//#endif

//#ifdef RUN_MEDIUM_RANDOM
//  Adafruit_NeoPixel stripMedium(LED_STRIP_COUNT, LED_PIN_MEDIUM_STRIP, NEO_GRB + NEO_KHZ800);
#endif

#ifdef RUN_MEDIUM_AS_FASTLED
  CRGBArray<LED_STRIP_COUNT> ledsMedium;
   //CRGB ledsMedium[LED_STRIP_COUNT];
#endif

#ifdef RUN_DENSE
Adafruit_NeoPixel stripDense(LED_STRIP_COUNT, LED_PIN_DENSE_STRIP, NEO_GRB + NEO_KHZ400);
#endif

#ifdef RUN_DENSE_AS_FASTLED
  CRGBArray<LED_STRIP_COUNT> leds;
  // CRGB leds[LED_STRIP_COUNT];  
#endif

// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:c:\Users\Admin\Documents\Arduino\libraries\FastLED\src\FastLED.h
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

void SetupThis( Adafruit_NeoPixel *astrip ){
  astrip->begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  astrip->clear();
  astrip->show();            // Turn OFF all pixels ASAP
  astrip->setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
}


// setup() function -- runs once at startup --------------------------------
uint8_t ledStripIntensity = 0;
unsigned long previousMillis = 0;
unsigned long currentMillis;
const char* ssid = HOME_WIFI_SSID;
const char* password = HOME_WIFI_PASSWORD;
uint8_t colorLoop = 0;

void setup() {
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

  #ifdef RUN_ON_BOARD   
  SetupThis(&onboard);  
  #endif

  #ifdef RUN_SINGLE     
  SetupThis(&single);   
  #endif

  #if defined(RUN_MEDIUM) || defined(RUN_MEDIUM_RANDOM)
  SetupThis(&stripMedium);    
  //#endif
  //
  //#ifdef RUN_MEDIUM_RANDOM
  //SetupThis(&stripMedium);    
  #endif

  #ifdef RUN_MEDIUM_AS_FASTLED
  FastLED.setMaxPowerInVoltsAndMilliamps(FASTLED_VOLTS, FASTLED_MAX_MA);
  FastLED.addLeds<FASTLED_MEDIUM_LED_TYPE, LED_PIN_MEDIUM_STRIP, FASTLED_STRIP_COLOR_ORDER>(ledsMedium, LED_STRIP_COUNT);  
  #endif

  #ifdef RUN_DENSE
  SetupThis(&stripDense);  
  #endif

  #ifdef RUN_DENSE_AS_FASTLED
  FastLED.setMaxPowerInVoltsAndMilliamps(FASTLED_VOLTS, FASTLED_MAX_MA);
  FastLED.addLeds<FASTLED_DENSE_LED_TYPE, LED_PIN_DENSE_STRIP, FASTLED_STRIP_COLOR_ORDER>(leds, LED_STRIP_COUNT);  
  #endif


  /* required OTA setup code*/
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    //Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();    
  }

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("ESP32_Lolin_C3_Pico_A");
  ArduinoOTA.begin();
 /* required OTA setup code*/

}

// loop() function -- runs repeatedly as long as board is on ---------------

void loop() {
  ArduinoOTA.handle();

  // Fill along the length of the strip in various colors...
  currentMillis = millis();
  if (currentMillis - previousMillis >= COLOR_STRIP_DELAY) {
    previousMillis = currentMillis;
  
    switch (colorLoop) {
      case 0 : 
        Color_Cycling(COLOUR_RED);
        break;
      case 1 :
        Color_Cycling(COLOUR_GREEN);
        break;
      case 2 :
        Color_Cycling(COLOUR_BLUE);
        break;
    }
    colorLoop++;
    if (colorLoop > 2) {
     colorLoop = 0;
    }

    ledStripIntensity = ledStripIntensity + 10;
    if (ledStripIntensity > 255) {
      ledStripIntensity = 10;
    }
  }
}

const int COLOR_COUNT = 5;
enum colors { RED = 0, GREEN, BLUE, YELLOW, PURPLE };

uint32_t getRandomColor(Adafruit_NeoPixel *astrip)
{    
    switch(random(COLOR_COUNT))
    {
        case RED:    { return astrip->Color(255, 0, 0);   }
        case GREEN:  { return astrip->Color(0, 255, 0);   }
        case BLUE:   { return astrip->Color(0, 0, 255);   }
        case YELLOW: { return astrip->Color(255, 255, 0); }
        case PURPLE: { return astrip->Color(255, 0, 255); }
        
        // should never happen
        default:{ return astrip->Color(255, 255, 255); }
    }  
}


void colorWipe(Adafruit_NeoPixel *astrip, uint32_t color) {
  for(int i=0;  i < astrip->numPixels(); i++) { // For each pixel in strip...
    astrip->setPixelColor(i, color);            //  Set pixel's color (in RAM)
  }
  astrip->setBrightness(ledStripIntensity);
  astrip->show();                             //  Update strip to match  
}

void colorRandomStrip( Adafruit_NeoPixel *astrip, uint32_t color) {
  for(int i=0;  i < astrip->numPixels(); i++) { // For each pixel in strip...
    astrip->setPixelColor(i, getRandomColor(astrip));            //  Set pixel's color (in RAM)
  }
  astrip->setBrightness(ledStripIntensity);
  astrip->show();                             //  Update strip to match    
}

#if defined RUN_DENSE_AS_FASTLED || defined RUN_MEDIUM_AS_FASTLED
void colorFastLED(CRGB aled[], uint32_t color ) {
  fill_solid( aled, LED_STRIP_COUNT, color);
  FastLED.setBrightness(ledStripIntensity);
  FastLED.show();  
}
#endif

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(Adafruit_NeoPixel *astrip, uint32_t color, int wait) {
  for(int a=0; a<10; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      astrip->clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c < astrip->numPixels(); c += 3) {
        astrip->setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      astrip->show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
    }
  }
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(Adafruit_NeoPixel *astrip, int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    // strip.rainbow() can take a single argument (first pixel hue) or
    // optionally a few extras: number of rainbow repetitions (default 1),
    // saturation and value (brightness) (both 0-255, similar to the
    // ColorHSV() function, default 255), and a true/false flag for whether
    // to apply gamma correction to provide 'truer' colors (default true).
    astrip->rainbow(firstPixelHue);
    // Above line is equivalent to:
    // strip.rainbow(firstPixelHue, 1, 255, 255, true);
    astrip->show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(Adafruit_NeoPixel *astrip, int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<30; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      astrip->clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for(int c=b; c < astrip->numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / astrip->numPixels();
        uint32_t color = astrip->gamma32(astrip->ColorHSV(hue)); // hue -> RGB
        astrip->setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      astrip->show();                // Update strip with new contents
      delay(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}
