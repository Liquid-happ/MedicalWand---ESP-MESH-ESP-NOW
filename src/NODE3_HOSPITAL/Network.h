#ifndef NETWORK_H
#define NETWORK_H
#include "Config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <esp_now.h>

WiFiClientSecure espClient;
PubSubClient client(espClient);
typedef struct struct_message { char cmd[10]; } struct_message;
struct_message myData;
esp_now_peer_info_t peerInfo;

extern const char* root_ca;

// Prototype hàm
void sendToNode2(const char* c) { 
    strcpy(myData.cmd, c); 
    esp_now_send(mac_Node2, (uint8_t*)&myData, sizeof(myData)); 
}

void OnDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
    struct_message in; memcpy(&in, data, sizeof(in));
    // Đồng bộ trạng thái đèn LED trên Remote
    if (strcmp(in.cmd,"SOS")==0) roomState = SOS;
    else if (strcmp(in.cmd,"SAFE")==0) roomState = NORMAL;
}

void mqttCallback(char* t, byte* p, unsigned int l) {
  String m; for(int i=0;i<l;i++) m+=(char)p[i];
  if (String(t).indexOf("control")>0 && m=="CLEAR_ALARM") { 
      roomState = NORMAL; 
      digitalWrite(BUZZER_PIN, LOW); 
      sendToNode2("SAFE"); 
  }
}

void setupNetwork() {
    WiFi.mode(WIFI_AP_STA); WiFi.begin(WIFI_SSID, WIFI_PASS);
    while(WiFi.status()!=WL_CONNECTED) delay(500);

    espClient.setCACert(root_ca); client.setServer(MQTT_SERVER, MQTT_PORT); client.setCallback(mqttCallback);
    
    if(esp_now_init()==ESP_OK) esp_now_register_recv_cb(OnDataRecv);
    memcpy(peerInfo.peer_addr, mac_Node2, 6); peerInfo.channel = WiFi.channel(); peerInfo.encrypt = false; esp_now_add_peer(&peerInfo);
}

void loopNetwork() {
  if(!client.connected()) {
    if(client.connect("Node3_Nurse", MQTT_USER, MQTT_PASS)) client.subscribe("hospital/room1/control");
  }
  client.loop();
}
#endif
