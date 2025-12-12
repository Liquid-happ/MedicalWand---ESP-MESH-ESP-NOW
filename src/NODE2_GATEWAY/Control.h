#ifndef CONTROL_H
#define CONTROL_H
#include "Config.h"
#include <ESP32Servo.h>

void sendMQTT(const char* t, const char* m);
void sendToNode1(const char* c);

Servo servoDoor; Servo servoFan;

// Khởi tạo biến
bool isEmergency=false, isReminding=false, isProcessing=false, isLightOn=false, isFanOn=false;
float temp=0, hum=0;
int fanPos=0; int fanDir=1; unsigned long lastFan=0; 
int fanSpeedDelay=20; 
bool isDoorOpen=false; unsigned long doorTime=0;

void setupControl() {
    pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);
    pinMode(PIN_SINGLE_LED, OUTPUT); digitalWrite(PIN_SINGLE_LED, LOW);
    servoDoor.attach(PIN_SERVO_DOOR); servoDoor.write(0);
    servoFan.attach(PIN_SERVO_FAN);   servoFan.write(0);
}

void ctrlLight(bool state) {
    isLightOn = state;
    digitalWrite(PIN_SINGLE_LED, state ? HIGH : LOW);
    sendMQTT("hospital/room1/device/light", state ? "ON" : "OFF");
}

void ctrlFan(bool state) {
    isFanOn = state;
    sendMQTT("hospital/room1/device/fan", state ? "ON" : "OFF");
    if(!state) servoFan.write(0);
}

void triggerDoorOpen() {
    servoDoor.write(90); isDoorOpen=true; doorTime=millis();
    sendMQTT("hospital/room1/access", "DOOR_OPENED");
}

void setSafe() {
    digitalWrite(BUZZER_PIN, LOW);
    isEmergency=false; isProcessing=false; isReminding=false;
    sendMQTT("hospital/room1/alert", "SAFE");
    sendToNode1("SAFE"); 
}

void setProcessing() {
    digitalWrite(BUZZER_PIN, LOW); 
    isEmergency=false; 
    isProcessing=true;
    sendMQTT("hospital/room1/alert", "PROCESSING");
}

void loopControl() {
    if(isFanOn && millis()-lastFan > fanSpeedDelay) {
        lastFan=millis(); fanPos+=2*fanDir;
        if(fanPos>=180||fanPos<=0) fanDir=-fanDir;
        servoFan.write(fanPos);
    }
    if(isDoorOpen && millis()-doorTime > 3000) {
        servoDoor.write(0); isDoorOpen=false;
    }
}
#endif
