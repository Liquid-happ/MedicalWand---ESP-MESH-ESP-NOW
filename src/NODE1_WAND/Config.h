#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- MAC ADDRESS 
const uint8_t mac_Node2[] = {0xD4, 0xE9, 0xF4, 0xA4, 0xF2, 0xB8}; // Gateway
const uint8_t mac_Node3[] = {0xAC, 0x15, 0x18, 0xD7, 0xCD, 0x08}; // Remote Y tá

// --- PINOUT ---
#define SDA_PIN 21
#define SCL_PIN 22
#define BUTTON_SOS_PIN 15    // Nút SOS (Nhấn giữ)
#define BUTTON_QUICK_PIN 13  // Nút Nhấn nhanh (SAFE)
#define LED_R 5 
#define LED_G 4 
#define LED_B 2

#define WIFI_CHANNEL 11 

#endif
