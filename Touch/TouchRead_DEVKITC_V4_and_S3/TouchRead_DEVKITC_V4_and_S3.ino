// ESP32 Touch Test
// Just test touch pin - Touch0 is T0 which is on GPIO 4.


#if SOC_TOUCH_SENSOR_VERSION == 1 // ESP32 in general
  #define THRESHOLD 40  // change to your sensibility liking
  #define LED_PIN_HIGH HIGH
  #define LED_PIN_LOW LOW
  #define LED_PIN 25   //not onboard led
  #define OUTPUT_MASK_LEN 4  
  #define NUMBER_TOUCH_PINS 10 // 10 available touch pins
  const uint8_t LIST_OF_TOUCHPINS[NUMBER_TOUCH_PINS] =  { T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 };    
#else
  #if SOC_TOUCH_SENSOR_VERSION == 2  // specific to EPS32 S2 and S3
    #define THRESHOLD 25000  // change to your sensibility liking
    /* it sems that XIAO ESp32 S3 user led is lit when digitalWrite is LOW, and off when it is HIGH. */
    #define LED_PIN_HIGH LOW
    #define LED_PIN_LOW HIGH     
    #define LED_PIN 21  // onboard user led pin
    #define OUTPUT_MASK_LEN 8  // the touchread value is WAY bigger in ESP32 s2-s3 
    #define NUMBER_TOUCH_PINS 9  // and there is only 9 touch pins
    const uint8_t LIST_OF_TOUCHPINS[NUMBER_TOUCH_PINS] =  { T1, T2, T3, T4, T5, T6, T7, T8, T9 };     
   #endif
#endif

bool ThresholdIsreached( int value ) {
  /* Given that the esp32 S2-S3 are touched whrn value goes down, 
     and ESP32 dev board are doing the opposite, we must have a function that handles both.*/

  #if SOC_TOUCH_SENSOR_VERSION == 1 
    return ((value <= THRESHOLD) && (value > 0));  // a non responsive PIN will output 0 (zero), so filter it out
  #elif SOC_TOUCH_SENSOR_VERSION == 2
    return (value > THRESHOLD);
  #endif
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  delay(1000);  // give me time to bring up serial monitor
  Serial.println("ESP32 Touch Test");
 
  digitalWrite(LED_PIN, LED_PIN_HIGH);
  delay(500);
  digitalWrite(LED_PIN, LED_PIN_LOW);
}

void loop() {
  touch_value_t touchedArray[NUMBER_TOUCH_PINS]; //local array to receive data from pins
  bool IsToBeLit = false;  //will we lit the led?

  for (uint8_t i = 0; i < NUMBER_TOUCH_PINS; i++) {
     touchedArray[i] = touchRead(LIST_OF_TOUCHPINS[i]);  //collect a touchpin value
     /* Verify that it's threshold crossed.  
        If so, the IsToBeLit will become positive and will remain so until end of loop */
     IsToBeLit = IsToBeLit || ThresholdIsreached(touchedArray[i]); 
     Serial.printf( "%*d", OUTPUT_MASK_LEN, touchedArray[i] ); // output value, formated as to be readable
  }

   if (IsToBeLit) {
    digitalWrite(LED_PIN, LED_PIN_HIGH);
    Serial.println(" Lit"  );  
  } else {
    digitalWrite(LED_PIN, LED_PIN_LOW);
    Serial.println(" Off"  );  
  }  
  delay(100);
 }
