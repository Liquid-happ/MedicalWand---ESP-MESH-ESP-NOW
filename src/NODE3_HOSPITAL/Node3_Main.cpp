#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <esp_now.h>
#include <time.h>

// --- CẤU HÌNH ---
#define WIFI_SSID "Tang1"
#define WIFI_PASS "999999999"
#define MQTT_SERVER "v7f16116.ala.asia-southeast1.emqxsl.com" // Server mới
#define MQTT_PORT 8883
#define MQTT_USER "quyet"
#define MQTT_PASS "1"

// Chứng chỉ DigiCert Global Root G2 (Giống Node 2)
const char* root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
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

// MAC Của Node 1 và Node 2
uint8_t mac_Node1[] = {0xD4, 0xE9, 0xF4, 0xA4, 0xE9, 0x58};
uint8_t mac_Node2[] = {0xD4, 0xE9, 0xF4, 0xA4, 0xF2, 0xB8}; 

#define BUZZER_PIN 15
#define BUTTON_PIN 4
#define LED_RED_PIN 19

enum State { NORMAL, SOS, PROCESSING };
State roomState = NORMAL;

WiFiClientSecure espClient; 
PubSubClient client(espClient);
typedef struct struct_message { char cmd[10]; } struct_message;
struct_message myData;
esp_now_peer_info_t peerInfo;

void buzzer(bool on) { digitalWrite(BUZZER_PIN, on); }

void sendToNode1(const char* c) { 
  strcpy(myData.cmd, c); esp_now_send(mac_Node1, (uint8_t*)&myData, sizeof(myData)); 
}
void sendToNode2(const char* c) { 
  strcpy(myData.cmd, c); esp_now_send(mac_Node2, (uint8_t*)&myData, sizeof(myData)); 
}
void reportWeb(const char* status) { 
  if(client.connected()) client.publish("hospital/room1/alert", status); 
}

void OnDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  struct_message in; memcpy(&in, data, sizeof(in));
  if (strcmp(in.cmd,"SOS")==0 || strcmp(in.cmd,"FALL")==0) {
    roomState = SOS; buzzer(true); reportWeb("EMERGENCY");
  } else if (strcmp(in.cmd,"SAFE")==0 || strcmp(in.cmd,"SLEEP")==0) {
    roomState = NORMAL; buzzer(false); reportWeb("CLEAR");
  }
}

void mqttCallback(char* t, byte* p, unsigned int l) {
  String m; for(int i=0;i<l;i++) m+=(char)p[i];
  if (String(t).indexOf("control")>0 && m=="CLEAR_ALARM") {
    roomState = NORMAL; buzzer(false); 
    sendToNode1("SAFE"); sendToNode2("SAFE");
    reportWeb("CLEAR");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT); pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_RED_PIN, OUTPUT); digitalWrite(LED_RED_PIN, LOW);

  // 1. Kết nối WiFi
  WiFi.mode(WIFI_AP_STA); WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi Connecting");
  while(WiFi.status()!=WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(" Connected!");

  // 2. Cập nhật thời gian
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Sync Time");
  struct tm timeinfo;
  while(!getLocalTime(&timeinfo)) { Serial.print("."); delay(500); }
  Serial.println(" Time OK!");

  // 3. Cấu hình SSL
  espClient.setCACert(root_ca);
  client.setServer(MQTT_SERVER, MQTT_PORT); 
  client.setCallback(mqttCallback);
  
  if(esp_now_init()==ESP_OK) esp_now_register_recv_cb(OnDataRecv);
  
  memcpy(peerInfo.peer_addr, mac_Node1, 6); 
  peerInfo.channel = WiFi.channel(); peerInfo.encrypt = false; 
  esp_now_add_peer(&peerInfo);

  memcpy(peerInfo.peer_addr, mac_Node2, 6); 
  esp_now_add_peer(&peerInfo);
}

void loop() {
  // Tự động kết nối lại MQTT
  if(!client.connected()) {
    Serial.print("Connecting MQTT...");
    String clientId = "Nurse_Node3_" + String(random(0xffff), HEX);
    if(client.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
       Serial.println("Connected!");
       client.subscribe("hospital/room1/control");
    } else {
       Serial.print("Failed rc="); Serial.println(client.state());
       delay(2000);
    }
  }
  client.loop();

  static bool lb = HIGH; bool cb = digitalRead(BUTTON_PIN);
  if (lb == HIGH && cb == LOW) { 
    if (roomState == SOS) {
      roomState = PROCESSING; buzzer(false); 
      sendToNode2("MUTE"); reportWeb("PROCESSING");
    } 
    else if (roomState == PROCESSING) {
      roomState = NORMAL; 
      sendToNode1("SAFE"); sendToNode2("SAFE"); reportWeb("CLEAR");
    }
    delay(300);
  }
  lb = cb;

  if(roomState==SOS) { if((millis()/200)%2) digitalWrite(LED_RED_PIN, HIGH); else digitalWrite(LED_RED_PIN, LOW); }
  else if(roomState==PROCESSING) digitalWrite(LED_RED_PIN, HIGH);
  else digitalWrite(LED_RED_PIN, LOW);
}