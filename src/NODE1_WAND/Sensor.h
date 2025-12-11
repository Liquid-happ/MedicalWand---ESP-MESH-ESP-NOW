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

void setColor(int r, int g, int b) { 
    analogWrite(LED_R, r); analogWrite(LED_G, g); analogWrite(LED_B, b); 
}

void setupSensor() {
    pinMode(BUTTON_SOS_PIN, INPUT_PULLUP);
    pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
    
    Wire.begin(SDA_PIN, SCL_PIN);
    byte status = mpu.begin();
    if(status != 0){ Serial.println("MPU connect failed"); while(1); }
    delay(1000); 
    mpu.calcOffsets(); // Giữ yên cảm biến khi khởi động
    setColor(0, 255, 0); delay(500); setColor(0,0,0); // Báo hiệu sẵn sàng
}

void loopSensor() {
    mpu.update();
    int btnState = digitalRead(BUTTON_SOS_PIN);

    // 1. Xử lý Nút bấm SOS
    if (btnState == LOW && buttonDownTime == 0) {
        buttonDownTime = millis(); isButtonHeld = false;
    }
    if (btnState == LOW && buttonDownTime > 0) {
        if ((millis() - buttonDownTime > 2000) && !isButtonHeld) {
            isButtonHeld = true; 
            sendToSystem("SOS"); setColor(255, 0, 0);
        }
    }
    if (btnState == HIGH && buttonDownTime > 0) {
        if (!isButtonHeld) { // Nhấn nhả nhanh
            sendToSystem("SAFE"); setColor(0, 0, 0);
        }
        buttonDownTime = 0;
    }

    // 2. Chỉ phát hiện Ngã/Cử chỉ khi KHÔNG bấm nút
    if (buttonDownTime == 0) {
        // Phát hiện Ngã
        float acc = mpu.getAccX()*mpu.getAccX() + mpu.getAccY()*mpu.getAccY() + mpu.getAccZ()*mpu.getAccZ();
        if (acc > 8.0) { // Ngưỡng ngã (tùy chỉnh)
            sendToSystem("FALL"); setColor(255,0,0); delay(1000); 
        }

        // Phát hiện Cử chỉ (Nghiêng)
        float angX = abs(mpu.getAngleX()); 
        float angY = abs(mpu.getAngleY());
        if (!gestureSent) {
            if (angY > 40) { // Nghiêng trái/phải -> Quạt
                setColor(0, 255, 0); sendToSystem("FAN"); gestureSent = true; delay(300); 
            }
            else if (angX > 40) { // Nghiêng trước/sau -> Đèn
                setColor(0, 0, 255); sendToSystem("LIGHT"); gestureSent = true; delay(300); 
            }
        } else if (angX < 15 && angY < 15) { 
            gestureSent = false; setColor(0,0,0); 
        }
    }
}
#endif