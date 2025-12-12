#ifndef SENSOR_H
#define SENSOR_H

#include "Config.h"
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>
void sendMQTT(const char* t, const char* m);
void triggerDoorOpen();
void setSafe();

DHT dht(DHT_PIN, DHT_TYPE);
MFRC522 mfrc522(SS_PIN, RST_PIN);

unsigned long lastDhtRead = 0;

void setupSensor() {
    dht.begin();
    SPI.begin();
    mfrc522.PCD_Init();
}

void loopSensor() {
    // 1. DHT Sensor
    if (millis() - lastDhtRead > 2000) {
        lastDhtRead = millis();
        float t = dht.readTemperature();
        float h = dht.readHumidity();
        if (!isnan(t)) {
            temp = t; hum = h;
            static int cnt = 0;
            if (++cnt >= 5) { 
                sendMQTT("hospital/room1/env", String(t).c_str());
                sendMQTT("hospital/room1/hum", String(h).c_str());
                cnt = 0;
            }
        }
    }

    // 2. RFID
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
        triggerDoorOpen();
        if (isEmergency || isProcessing) setSafe(); 
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
    }
}
#endif
