#include "BluetoothSerial.h"

#define ledPIN_1 19
#define ledPIN_2 18


BluetoothSerial SerialBT;
char incomingChar;

void ToggleStatus( bool fine ) {
  if (!fine) {    
    digitalWrite(ledPIN_1, HIGH);
    delay(2000);
    digitalWrite(ledPIN_1, LOW);     
  } else {
     for (byte i = 0; i < 4; i++) {
        digitalWrite(ledPIN_1, HIGH);
        delay(200);
        digitalWrite(ledPIN_1, LOW); 
        delay(200);
     }
  }
}

void TogglePINs( uint PIN_1, uint PIN_2, String LEDNames){
  String s2;

  if ((digitalRead(PIN_1) == HIGH) || (digitalRead(PIN_2) == HIGH)) { 
     digitalWrite(PIN_1, LOW); 
     digitalWrite(PIN_2, LOW);      
     s2 = "OFF";
  } else { 
    digitalWrite(PIN_1, HIGH); 
    digitalWrite(PIN_2, HIGH);      
    s2 = "ON";
  };
  SerialBT.println("LEDs " + LEDNames + " turned " + s2);
}

void TogglePIN_Led( uint PIN, String LED_Name ) {  
  String s2;
  if (digitalRead(PIN) == HIGH) { 
     digitalWrite(PIN, LOW); 
     s2 = "OFF";
  } else { 
    digitalWrite(PIN, HIGH); 
    s2 = "ON";
  };
  SerialBT.println("LED " + LED_Name + " turned " + s2);
}

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  i (event == ESP_SPP_SRV_OPEN_EVT) {
    SerialBT.println("Welcome");
  }
 
  if (event == ESP_SPP_CLOSE_EVT ) {
    digitalWrite(ledPIN_1, LOW);
    digitalWrite(ledPIN_2, LOW);
  }
}


void setup() {
  pinMode(ledPIN_1, OUTPUT);
  pinMode(ledPIN_2, OUTPUT);
  SerialBT.register_callback(callback);
  ToggleStatus(SerialBT.begin("ESP32Test"));
}

void loop() {
    if (SerialBT.available()) {
      incomingChar = SerialBT.read();
      
      switch (incomingChar) {
        case 48 :
          TogglePIN_Led(ledPIN_1, "1");
          break;
        case 49 :
          TogglePIN_Led(ledPIN_2, "2");
          break;
        case 50 :
          TogglePINs(ledPIN_1, ledPIN_2, "1 & 2");
          break;
      }
    }
}
