#include "Config.h"
#include "Control.h"
#include "Network.h"
#include "Display.h" 
#include "Sensor.h"  

void setup() {
  Serial.begin(115200);
  
  setupControl(); 
  setupDisplay(); 
  setupNetwork(); 
  setupSensor(); 
}

void loop() {
  loopNetwork(); 
  loopControl(); 
  loopSensor(); 
  updateTime();

  // Logic hiển thị & Còi
  if(isEmergency && !isProcessing) {
     if((millis()/200)%2) digitalWrite(BUZZER_PIN, HIGH); else digitalWrite(BUZZER_PIN, LOW);
     drawUI_Alert("! SOS !", "CAN CUU HO");
  } 
  else if(isProcessing) {
     drawUI_Alert("DANG XU LY", "Y TA DA DEN");
  }
  else if(isReminding) {
     if((millis()/500)%2) digitalWrite(BUZZER_PIN, HIGH); else digitalWrite(BUZZER_PIN, LOW);
     drawUI_Alert("NHAC NHO", "UONG THUOC");
     static unsigned long rs=0; if(rs==0) rs=millis();
     if(millis()-rs>60000) { isReminding=false; digitalWrite(BUZZER_PIN,LOW); rs=0; }
  } else {
     drawUI_Normal();
  }

  struct tm ti; if(getLocalTime(&ti)) {
     if((ti.tm_hour==8 || ti.tm_hour==18) && ti.tm_min==0 && ti.tm_sec==0 && !isReminding) {
        isReminding=true; sendMQTT("hospital/room1/alert", "MEDICATION");
     }
  }
}
