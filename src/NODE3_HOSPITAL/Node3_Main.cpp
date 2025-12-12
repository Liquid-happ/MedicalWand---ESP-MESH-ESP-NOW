#include "Config.h"
#include "Network.h"

State roomState = NORMAL;

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);
  pinMode(LED_RED_PIN, OUTPUT); digitalWrite(LED_RED_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  setupNetwork();
}

void loop() {
  loopNetwork();

  static bool lastBtn = HIGH; 
  bool currBtn = digitalRead(BUTTON_PIN);
  static unsigned long lastPressTime = 0;
  unsigned long debounceDelay = 300;

  if (lastBtn == HIGH && currBtn == LOW) { 
      if (millis() - lastPressTime > debounceDelay) {
          lastPressTime = millis();
          
          if (roomState == SOS) { 
              roomState = PROCESSING; 
              digitalWrite(BUZZER_PIN, LOW); 
              sendToNode2("MUTE"); 
          } 
          else if (roomState == PROCESSING) { 
              roomState = NORMAL; 
              sendToNode2("SAFE"); 
          }
          else if (roomState == NORMAL) {
              sendToNode2("SAFE");
          }
      }
  }
  lastBtn = currBtn;

  // Điều khiển đèn LED trạng thái trên Remote
  if(roomState == SOS) {
      if((millis()/200)%2) digitalWrite(LED_RED_PIN, HIGH); else digitalWrite(LED_RED_PIN, LOW);
  } else if(roomState == PROCESSING) {
      digitalWrite(LED_RED_PIN, HIGH); 
  } else {
      digitalWrite(LED_RED_PIN, LOW);
  }
}
