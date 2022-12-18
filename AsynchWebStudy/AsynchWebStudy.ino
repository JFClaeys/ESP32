#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <wifi_credential.h>
#include <Adafruit_NeoPixel.h>
#include "web_page.h"
#include "RainbowDef.h"

#define BLUE_LED 8
#define GREEN_LED 10
#define RED_LED 6
#define NEORGB_LED 7

#define LOOP_MS 3      // how long in ms  before iteration of colour

const char* ssid = HOME_WIFI_SSID;
const char* password = HOME_WIFI_PASSWORD;

const char* input_parameter1 = "output";
const char* input_parameter2 = "state";

uint8_t iWait = 0;          // current cycle before next color cycle
uint16_t AngleCycling = 0;  // current angle to use in the rainbow array 
bool runNeo = true;

// Creating a AsyncWebServer object 
AsyncWebServer server(80);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, NEORGB_LED, NEO_GRB + NEO_KHZ800);


// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    buttons += "<h4>Output - BLUE LED</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + pintoid(BLUE_LED) + "\" " + outputState(BLUE_LED) + "><span class=\"slider\"></span></label>"+"\n";

    buttons += "<h4>Output - GREEN LED</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + pintoid(GREEN_LED) + "\" " + outputState(GREEN_LED) + "><span class=\"slider\"></span></label>"+"\n";

    buttons += "<h4>Output - RED LED</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + pintoid(RED_LED) + "\" " + outputState(RED_LED) + "><span class=\"slider\"></span></label>"+"\n";

    buttons += "<h4>Output - RGB NEO</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleAnimation(this)\" id=\"" + pintoid(0) + "\" " + outputBooleanState(0) + "><span class=\"slider\"></span></label>"+"\n";

    return buttons;
  }
  return String();
}

String pintoid( int input ) {
  return String( input );
}   

String outputState(int output){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
}

String outputBooleanState(int output){
  if (runNeo) {
    return "checked";
  } else {
    return "";
  }
}

void sineLED(byte LED, int angle)
{
  uint8_t red = pgm_read_byte(lights +  ((angle + 120) % CIRCLE_ANGLES));
  uint8_t green = pgm_read_byte(lights + angle);
  uint8_t blue = pgm_read_byte(lights + ((angle + 240) % CIRCLE_ANGLES));
  pixels.setPixelColor(LED, pixels.Color(red, green, blue)); // Moderately bright green color.  
  pixels.show();
}

void processLoopContent() {
  if (!runNeo) {
    return;
  }

  if (iWait < LOOP_MS) {
    delay(1);
    iWait++;
  } else {
    iWait = 0;

    if ((AngleCycling % 5) == 0) {
       sineLED(0, AngleCycling);
    }

    //going further on the cycle or resttting it
    if (AngleCycling < CIRCLE_ANGLES) {
      AngleCycling++;
    } else {
      AngleCycling = 0;  
    }      
  }  
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  pixels.begin();  

  pinMode(BLUE_LED,OUTPUT);    digitalWrite(BLUE_LED, HIGH);
  pinMode(RED_LED, OUTPUT);    digitalWrite(RED_LED, HIGH);
  pinMode(GREEN_LED, OUTPUT);  digitalWrite(GREEN_LED, HIGH);
    
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", 
            HTTP_GET, 
            [](AsyncWebServerRequest *request){request->send_P(200, "text/html", index_html, processor);}
           );

  server.on("/toggle",
              HTTP_GET,
              [](AsyncWebServerRequest *request){
                if (runNeo) {
                  pixels.setPixelColor(0, pixels.Color(0, 0, 0)); // Moderately bright green color.  
                  pixels.show();
                }
                runNeo = !runNeo;                                
                Serial.println("Toggle runNeo");
                request->send_P(200, "text/pain", "OK");
              }
            );

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", 
            HTTP_GET, 
            [] (AsyncWebServerRequest *request) {
                String inputMessage1;
                String inputMessage2;
                // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
                if (request->hasParam(input_parameter1) && request->hasParam(input_parameter2)) {
                  inputMessage1 = request->getParam(input_parameter1)->value();
                  inputMessage2 = request->getParam(input_parameter2)->value();
                  digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
                }
                else {
                  inputMessage1 = "No message sent";
                  inputMessage2 = "No message sent";
                }
                Serial.print("GPIO: ");
                Serial.print(inputMessage1);
                Serial.print(" - Set to: ");
                Serial.println(inputMessage2);
                request->send(200, "text/plain", "OK");
              }
          );

  // Start server
  server.begin();

}

void loop() {
  // put your main code here, to run repeatedly:
  processLoopContent();
}
