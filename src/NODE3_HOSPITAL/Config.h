#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

#define WIFI_SSID "Tang1"
#define WIFI_PASS "999999999"
#define MQTT_SERVER "v7f16116.ala.asia-southeast1.emqxsl.com"
#define MQTT_PORT 8883
#define MQTT_USER "quyet"
#define MQTT_PASS "1"

const uint8_t mac_Node2[] = {0xD4, 0xE9, 0xF4, 0xA4, 0xF2, 0xB8}; // MAC Gateway (Node 2)

#define BUZZER_PIN 15
#define BUTTON_PIN 4
#define LED_RED_PIN 19

enum State { NORMAL, SOS, PROCESSING };
extern State roomState;
#endif
