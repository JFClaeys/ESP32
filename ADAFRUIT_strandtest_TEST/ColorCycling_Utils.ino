
void Color_Cycling( uint32_t color ) {
  #ifdef RUN_ON_BOARD 
  colorWipe(&onboard, color);
  #endif

  #ifdef RUN_SINGLE 
  colorWipe(&single, color);
  #endif

  #ifdef RUN_MEDIUM
  //  colorRandomStrip(&stripMedium, color);
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

  #ifdef RUN_DENSE_AS_FASTLED
  colorFastLED( leds, color);
  #endif
}