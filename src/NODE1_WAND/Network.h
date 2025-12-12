#ifndef NETWORK_H
#define NETWORK_H

#include "Config.h"
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

typedef struct struct_message { char cmd[10]; } struct_message;
struct_message myData;
esp_now_peer_info_t peerInfo;

void setColor(int r, int g, int b); 

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  struct_message dataIn; memcpy(&dataIn, incomingData, sizeof(dataIn));
  if (strcmp(dataIn.cmd, "SAFE") == 0) { setColor(0,0,0); }
}

void sendToSystem(const char* command) {
  strcpy(myData.cmd, command);
  esp_now_send(mac_Node2, (uint8_t *) &myData, sizeof(myData)); 
  esp_now_send(mac_Node3, (uint8_t *) &myData, sizeof(myData)); 
  Serial.print("Gui lenh: "); Serial.println(command);
}

void setupNetwork() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true); 
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE); 
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(OnDataRecv);

  peerInfo.channel = WIFI_CHANNEL; peerInfo.encrypt = false;
  
  // Đăng ký Node 2 & Node 3
  memcpy(peerInfo.peer_addr, mac_Node2, 6); esp_now_add_peer(&peerInfo);
  memcpy(peerInfo.peer_addr, mac_Node3, 6); esp_now_add_peer(&peerInfo);
}
#endif
