#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// --- WIFI & MQTT 
#define WIFI_SSID "Tang1"           
#define WIFI_PASS "999999999"       
#define MQTT_SERVER "v7f16116.ala.asia-southeast1.emqxsl.com" 
#define MQTT_PORT 8883
#define MQTT_USER "quyet"           
#define MQTT_PASS "1"               

// --- PINOUT ---
#define SS_PIN 5
#define RST_PIN 4
#define BUZZER_PIN 27
#define PIN_SERVO_DOOR 13
#define PIN_SERVO_FAN  14
#define PIN_SINGLE_LED 32 
#define DHT_PIN 15
#define DHT_TYPE DHT11

// --- MAC NODE KHÁC ---
const uint8_t mac_Node1[] = {0xD4, 0xE9, 0xF4, 0xA4, 0xE9, 0x58}; // <--- SỬA MAC NODE 1
const uint8_t mac_Node3[] = {0xAC, 0x15, 0x18, 0xD7, 0xCD, 0x08}; // <--- SỬA MAC NODE 3

// --- BIẾN TOÀN CỤC (Extern) ---
extern bool isEmergency;
extern bool isReminding;
extern bool isProcessing;
extern bool isLightOn;
extern bool isFanOn;
extern float temp;
extern float hum;
extern int fanSpeedDelay; 
#endif
