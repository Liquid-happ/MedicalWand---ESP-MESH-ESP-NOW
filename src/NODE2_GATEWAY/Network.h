#ifndef NETWORK_H
#define NETWORK_H
#include "Config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <esp_now.h>

// --- KHAI BÁO TRƯỚC CÁC HÀM TỪ CONTROL.H ---
void ctrlLight(bool state);
void ctrlFan(bool state);
void setSafe();
void setProcessing();

WiFiClientSecure espClient;
PubSubClient client(espClient);
typedef struct struct_message { char cmd[10]; } struct_message;
struct_message myData;
esp_now_peer_info_t peerInfo;

// CA Certificate (Thay đổi với mỗi broker)
const char* root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)EOF";

void sendMQTT(const char* t, const char* m) {
    if(client.connected()) client.publish(t, m, true);
}

void sendToNode1(const char* c) {
    strcpy(myData.cmd, c); esp_now_send(mac_Node1, (uint8_t*)&myData, sizeof(myData));
}

void mqttCallback(char* t, byte* p, unsigned int l) {
    String msg; for(int i=0;i<l;i++) msg+=(char)p[i];
    if (String(t) == "hospital/room1/control") {
        if (msg == "LIGHT_ON") ctrlLight(true);
        else if (msg == "LIGHT_OFF") ctrlLight(false);
        else if (msg == "FAN_ON") ctrlFan(true);
        else if (msg == "FAN_OFF") ctrlFan(false);
        else if (msg == "CLEAR_ALARM") setSafe();
        else if (msg == "MUTE_ALARM") setProcessing();
        else if (msg.startsWith("FAN_SPEED_")) {
            int s = msg.substring(10).toInt();
            // Cập nhật biến toàn cục
            fanSpeedDelay = (s==1)?30 : (s==2)?15 : 5;
            if(!isFanOn) ctrlFan(true);
        }
    }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *data, int len) {
    struct_message in; memcpy(&in, data, sizeof(in));
    String cmd = String(in.cmd);
    if (cmd=="SOS" || cmd=="FALL") { isEmergency=true; isProcessing=false; digitalWrite(BUZZER_PIN, HIGH); sendMQTT("hospital/room1/alert", "EMERGENCY"); } 
    else if (cmd=="MUTE" || cmd=="SAFE") { if (isEmergency) setProcessing(); else setSafe(); }
    else if (cmd=="LIGHT") ctrlLight(!isLightOn); else if (cmd=="FAN") ctrlFan(!isFanOn);
}

void setupNetwork() {
    WiFi.mode(WIFI_AP_STA); WiFi.begin(WIFI_SSID, WIFI_PASS);
    while(WiFi.status()!=WL_CONNECTED) delay(500);
    configTime(7*3600, 0, "pool.ntp.org"); 

    espClient.setCACert(root_ca); client.setServer(MQTT_SERVER, MQTT_PORT); client.setCallback(mqttCallback);

    if(esp_now_init()==ESP_OK) esp_now_register_recv_cb(OnDataRecv);
    memcpy(peerInfo.peer_addr, mac_Node1, 6); peerInfo.channel=WiFi.channel(); peerInfo.encrypt=false;
    esp_now_add_peer(&peerInfo);
}

void loopNetwork() {
    if(!client.connected()) {
        if(client.connect("Node2_Gateway", MQTT_USER, MQTT_PASS)) {
            client.subscribe("hospital/room1/control");
            sendMQTT("hospital/room1/device/light", isLightOn?"ON":"OFF");
            sendMQTT("hospital/room1/device/fan", isFanOn?"ON":"OFF");
        }
    }
    client.loop();
}
#endif
