

//This routine creates a random number from 0 to 255 and uses it 
//to determine the sign of the brightness change for each color.
//The sign also reverses when the brigtness approaches the lower 
//or upper limits.
//The magnitude of the change = +-(MULT). 
uint32_t randomizedColor()
{
  static int8_t signBlue=1,signRed=1,signGreen=1;
  int8_t MULT = 10;
  static int32_t Red=200,Blue=200,Green=200;
  uint8_t randNum = random(255);
  
  if (randNum > 1 && randNum < 3) signBlue *= -1;
  if (randNum > 84 && randNum < 86) signRed *= -1;
  if (randNum > 169 && randNum <171) signGreen *= -1;
  
  if (Blue >= 240) signBlue = -1;
  if (Blue <= 10) signBlue = 1;
  if (randNum <= 84) Blue += (signBlue*MULT);
  if(Blue < 0) Blue = 10;
  if(Blue > 255) Blue = 240;
  
    
  if (Red >= 240) signRed = -1;
  if (Red <= 10) signRed = 1;
  if (randNum > 84 && randNum <= 169) Red += (signRed*MULT);
   if(Red < 0) Red = 10;
  if(Red > 255) Red = 240;
  
  if (Green >= 240) signGreen = -1;
  if (Green <= 10) signGreen = 1;
  if (randNum > 169) Green += (signGreen*MULT);
  if(Green < 0) Green = 10;
  if(Green > 255) Green = 240;
  
  return (Red<<16 | Green<<8 | Blue);
}


// Generate a random color with the given intensity (0-100) and applying color balance
uint32_t randomColorWithBalance(ColorBalance balance, int intensity) {
  // Scale intensity from 0-100 to 0-255
  uint8_t scaledIntensity = map(intensity, 0, 100, 0, 255);// no need, internally controled
  
  // Generate random base color values
  uint8_t r = random(256);
  uint8_t g = random(256);
  uint8_t b = random(256);
  
  // Apply color balance by multiplying each component
  r = ((uint16_t)r * balance.red) / 255;
  g = ((uint16_t)g * balance.green) / 255;
  b = ((uint16_t)b * balance.blue) / 255;
  
  // Apply intensity scaling to all components
  r = ((uint16_t)r * scaledIntensity) / 255;
  g = ((uint16_t)g * scaledIntensity) / 255;
  b = ((uint16_t)b * scaledIntensity) / 255;
  
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
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
    //uint32_t c = random(2147483647);
    //astrip->setPixelColor(i, c);

    //astrip->setPixelColor(i, getRandomColor(astrip));            //  Set pixel's color (in RAM)

    //astrip->setPixelColor(i, randomizedColor());
    ColorBalance abal;
    //abal.
    astrip->setPixelColor(i, randomColorWithBalance(abal , 100));

  }
  astrip->setBrightness(ledStripIntensity);
  astrip->show();                             //  Update strip to match    
}

/*************************************************************************/

void Color_Cycling( uint32_t color ) {
  #ifdef RUN_ON_BOARD 
  colorWipe(&onboard, color);
  #endif

  #ifdef RUN_SINGLE 
  colorWipe(&single, color);
  #endif

  #ifdef RUN_MEDIUM
  colorWipe(&stripMedium, color);
  #endif
  
  #ifdef RUN_MEDIUM_RANDOM
  colorRandomStrip(&stripMedium, color); 
  #endif
  
  #ifdef RUN_MEDIUM_AS_FASTLED
  colorFastLED(ledsMedium, color);
  #endif

  #ifdef RUN_DENSE
  colorWipe(&stripDense, color);
  #endif

  #ifdef RUN_DENSE_RANDOM
  colorRandomStrip(&stripDense, color); 
  #endif

  #ifdef RUN_DENSE_AS_FASTLED
  colorFastLED( leds, color);
  #endif
}