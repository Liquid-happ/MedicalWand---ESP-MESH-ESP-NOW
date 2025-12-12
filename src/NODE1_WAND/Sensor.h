#ifndef SENSOR_H
#define SENSOR_H

#include "Config.h"
#include "Network.h"
#include <Wire.h>
#include <MPU6050_light.h>

MPU6050 mpu(Wire);
bool gestureSent = false;
unsigned long buttonDownTime = 0;
bool isButtonHeld = false;
unsigned long lastFallSent = 0;
unsigned long lastQuickPress = 0;
unsigned long debounceDelay = 200;

void setColor(int r, int g, int b) { 
    analogWrite(LED_R, r); analogWrite(LED_G, g); analogWrite(LED_B, b); 
}

void setupSensor() {
    pinMode(BUTTON_SOS_PIN, INPUT_PULLUP);
    pinMode(BUTTON_QUICK_PIN, INPUT_PULLUP); 
    pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
    
    Wire.begin(SDA_PIN, SCL_PIN);
    mpu.begin(); delay(1000); 
    mpu.calcOffsets(); 
    setColor(0, 255, 0); delay(500); setColor(0,0,0);
}

void loopSensor() {
    mpu.update();
    int btnSOSState = digitalRead(BUTTON_SOS_PIN);
    int btnQuickState = digitalRead(BUTTON_QUICK_PIN);

    // --- 1. LOGIC NÚT BẤM SOS (NHẤN GIỮ > 2s) ---
    if (btnSOSState == LOW && buttonDownTime == 0) {
        buttonDownTime = millis(); isButtonHeld = false;
    }
    if (btnSOSState == LOW && buttonDownTime > 0) {
        if ((millis() - buttonDownTime > 2000) && !isButtonHeld) {
            isButtonHeld = true; 
            sendToSystem("SOS"); 
            setColor(255, 0, 0); 
        }
    }
    if (btnSOSState == HIGH && buttonDownTime > 0) {
        // Reset trạng thái giữ nút
        buttonDownTime = 0; 
    }

    // --- 2. LOGIC NÚT HÀNH ĐỘNG NHANH (SAFE/RESET) ---
    if (btnQuickState == LOW && (millis() - lastQuickPress > debounceDelay)) {
        sendToSystem("SAFE"); 
        setColor(0, 0, 0);
        lastQuickPress = millis(); 
    }


    // --- 3. CẢM BIẾN NGÃ & CỬ CHỈ ---
    // Chỉ hoạt động khi không có nút nào đang được giữ
    if (buttonDownTime == 0 && digitalRead(BUTTON_QUICK_PIN) == HIGH) { 
        
        // Phát hiện Ngã
        if (millis() - lastFallSent > 5000) { // Chống Ngã liên tục (5s)
            float acc = mpu.getAccX()*mpu.getAccX() + mpu.getAccY()*mpu.getAccY() + mpu.getAccZ()*mpu.getAccZ();
            if (acc > 8.0) { 
                sendToSystem("FALL"); setColor(255,0,0); 
                lastFallSent = millis();
                delay(1000); 
            }
        }

        // Phát hiện Cử chỉ (Nghiêng)
        float angX = abs(mpu.getAngleX()); 
        float angY = abs(mpu.getAngleY());
        if (!gestureSent) {
            if (angY > 40) { sendToSystem("FAN"); gestureSent = true; setColor(0, 255, 0); delay(300); }
            else if (angX > 40) { sendToSystem("LIGHT"); gestureSent = true; setColor(0, 0, 255); delay(300); }
        } else if (angX < 15 && angY < 15) { 
            gestureSent = false; setColor(0,0,0); 
        }
    }
}
#endif
