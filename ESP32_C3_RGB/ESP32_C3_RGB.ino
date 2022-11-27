#include <Adafruit_NeoPixel.h>
#include <OneButton.h>
#include <RainbowDef.h>

#define LONG_PRESS_NEXT_PATTERN 1000  // 1 second long press will increment pattern pointer while not lit
#define CLICK_MS_DURATION 120
#define PIN 7
#define BUTTON 9
#define START_SPEED 20
#define LED_0 0
#define LED_BLUE 3
#define LED_GREEN 2
#define LED_RED 1



//forward declarations
void onPressedForNextPattern();
void onSinglePressed();
void onDoubleClick();

uint16_t AngleCycling = 0;  // current angle to use in the rainbow array 
byte speed = START_SPEED;  // actually... milliseconds
int  millisWait;  //how many millisecnds before next step
unsigned long CurrentTime = 0;
unsigned long LoopStartTime = 0;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, PIN, NEO_GRB + NEO_KHZ800);

class Button{
private:
  OneButton button;
public:
  explicit Button(uint8_t pin):button(pin) {
    button.setClickTicks(CLICK_MS_DURATION);
    button.attachClick([](void *scope) { ((Button *) scope)->Clicked();}, this);
    button.attachDoubleClick([](void *scope) { ((Button *) scope)->DoubleClicked();}, this);
    button.attachLongPressStart([](void *scope) { ((Button *) scope)->LongPressed();}, this);
  }

  void Clicked() {
    onSinglePressed();
  }

  void DoubleClicked() {
    onDoubleClick();
  }

  void LongPressed() {
    onPressedForNextPattern();
  }

  void read() {
    button.tick();
  }
};

Button button(BUTTON);

void onPressedForNextPattern() {
  delay(100);
  pixels.clear();
  pixels.setPixelColor(LED_0, pixels.Color(0, 0, 255)); // Moderately bright green color.
  pixels.show();
  delay(100);      
  speed = START_SPEED;
}

void onSinglePressed() {
  if (speed > 0) {
      delay(100);
      pixels.clear();
      pixels.setPixelColor(LED_0, pixels.Color(255, 0, 0)); // Moderately bright green color.
      pixels.show();
      delay(100);      
    speed--;
  } 
}

void onDoubleClick() {  // test to see double clicking behaviour
  delay(100);
  pixels.clear();
  pixels.setPixelColor(LED_0, pixels.Color(0, 255, 0)); // Moderately bright green color.
  pixels.show();
  delay(100);      
  speed++;
}


void sineLED(byte LED, int angle)
{
  uint8_t red = pgm_read_byte(lights +  ((angle + 120) % CIRCLE_ANGLES));
  uint8_t green = pgm_read_byte(lights + angle);
  uint8_t blue = pgm_read_byte(lights + ((angle + 240) % CIRCLE_ANGLES));
  pixels.setPixelColor(LED, pixels.Color(red, green, blue)); // Moderately bright green color.
  
  analogWrite(LED_RED, red);
  analogWrite(LED_GREEN, green);
  analogWrite(LED_BLUE, blue);  
}

void setup() {
  
  // put your setup code here, to run once:
  pixels.begin(); // This initializes the NeoPixel library.
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN,OUTPUT);
  pinMode(LED_RED,OUTPUT);

  millisWait = START_SPEED;
  LoopStartTime = millis();  
}

void loop() {

  button.read();

  CurrentTime = millis();
  if ((CurrentTime - LoopStartTime) >=  millisWait) {
    /* Ready to move to next wait sequence */
  // put your main code here, to run repeatedly:
    if ((AngleCycling % 2) == 0) {
       sineLED(LED_0, AngleCycling);
       pixels.show(); // This sends the updated pixel color to the hardware.       
    }

    if (AngleCycling < CIRCLE_ANGLES) {
      AngleCycling++;
    } else {
      AngleCycling = 0;    
    }    
    LoopStartTime = millis();    
  }
}
