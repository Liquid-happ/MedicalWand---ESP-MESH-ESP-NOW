#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

// --- 1. CẤU HÌNH WIFI & MQTT ---
#define WIFI_SSID "Tang1"           
#define WIFI_PASS "999999999"       
#define MQTT_SERVER "v7f16116.ala.asia-southeast1.emqxsl.com" 
#define MQTT_PORT 8883
#define MQTT_USER "quyet"           
#define MQTT_PASS "1"               

// Chứng chỉ SSL (Giữ nguyên như cũ)
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

// MAC của Node 1 (Thay bằng MAC thật)
uint8_t mac_Node1[] = {0xD4, 0xE9, 0xF4, 0xA4, 0xE9, 0x58};

// --- CHÂN KẾT NỐI ---
#define SS_PIN 5
#define RST_PIN 4
#define BUZZER_PIN 27
#define PIN_SERVO_DOOR 13
#define PIN_SERVO_FAN  14
#define PIN_SINGLE_LED 32 
#define DHT_PIN 15
#define DHT_TYPE DHT11

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

WiFiClientSecure espClient; 
PubSubClient client(espClient);
MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo servoDoor; Servo servoFan;
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

typedef struct struct_message { char cmd[10]; } struct_message;
struct_message myData;
esp_now_peer_info_t peerInfo;

// --- BIẾN TRẠNG THÁI ---
bool isEmergency=false, isReminding=false, isLightOn=false;
bool isFanOn=false; 
bool isProcessing=false; 

// Biến Quạt
int fanPos = 0;
int fanSpeedDelay = 20; 
unsigned long lastFanMove = 0;
int fanDirection = 1;

// Biến khác
unsigned long reminderStart=0, lastDhtRead=0;
float temp=0, hum=0;
char timeStr[10]; 
char dateStr[12];
bool isDoorOpen = false;
unsigned long doorOpenTime = 0;

// --- CÁC HÀM GỬI DỮ LIỆU ---
void sendMQTT(const char* t, const char* m) { 
  if(client.connected()) client.publish(t, m, true); 
}

void sendToNode1(const char* c) { 
  strcpy(myData.cmd, c); esp_now_send(mac_Node1, (uint8_t*)&myData, sizeof(myData)); 
}

// --- CẬP NHẬT THỜI GIAN ---
void updateTime() {
  struct tm ti;
  if(getLocalTime(&ti)) {
    sprintf(timeStr, "%02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);
    sprintf(dateStr, "%02d/%02d", ti.tm_mday, ti.tm_mon + 1);
  } else {
    strcpy(timeStr, "--:--:--");
  }
}

// --- VẼ GIAO DIỆN OLED ---
void drawUI_Normal() {
  display.clearDisplay();
  display.fillRect(0, 0, 128, 12, WHITE);
  display.setTextColor(BLACK);
  display.setTextSize(1);
  display.setCursor(2, 2);
  
  if(isProcessing) display.print("! DANG XU LY !"); // Hiển thị trạng thái xử lý
  else if(isReminding) display.print("! NHAC NHO !");
  else display.print("PHONG 01 - OK");

  display.setTextColor(WHITE);
  display.setTextSize(2);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 16);
  display.print(timeStr);

  display.drawFastHLine(0, 36, 128, WHITE);

  display.setTextSize(1);
  display.setCursor(0, 42); display.print("Temp: "); display.print((int)temp); display.print("C");
  display.setCursor(0, 54); display.print("Hum : "); display.print((int)hum); display.print("%");

  // Icon Đèn
  if(isLightOn) display.fillCircle(85, 48, 5, WHITE); else display.drawCircle(85, 48, 5, WHITE);
  display.setCursor(80, 56); display.print("L");

  // Icon Quạt
  if(isFanOn) {
    int frame = (millis() / 200) % 4;
    if(frame==0) display.drawLine(110,43,110,53,WHITE);
    else if(frame==1) display.drawLine(105,48,115,48,WHITE);
    else if(frame==2) display.drawLine(106,44,114,52,WHITE);
    else if(frame==3) display.drawLine(106,52,114,44,WHITE);
  } else {
    display.drawRect(105, 43, 10, 10, WHITE);
  }
  display.setCursor(105, 56); display.print("F");
  display.display();
}

void drawUI_Alert(String title, String msg) {
  display.clearDisplay();
  display.fillScreen(WHITE);
  display.setTextColor(BLACK);
  display.setTextSize(2);
  display.setCursor(5, 10); display.println(title);
  display.setTextSize(1);
  display.setCursor(5, 35); display.println(msg);
  display.display();
}

// --- LOGIC HỆ THỐNG ---
void ctrlFan(bool state) {
  isFanOn = state;
  sendMQTT("hospital/room1/device/fan", state ? "ON" : "OFF");
  if(!state) servoFan.write(0);
}

void ctrlLight(bool state) {
  isLightOn = state;
  digitalWrite(PIN_SINGLE_LED, state ? HIGH : LOW);
  sendMQTT("hospital/room1/device/light", state ? "ON" : "OFF");
}

void triggerDoorOpen() { servoDoor.write(90); isDoorOpen = true; doorOpenTime = millis(); sendMQTT("hospital/room1/access", "DOOR_OPENED"); }

// --- HÀM QUAN TRỌNG: XỬ LÝ TRẠNG THÁI "TIẾP NHẬN" (MUTE/PROCESSING) ---
void setProcessingState() {
  // 1. Tắt còi NGAY LẬP TỨC
  digitalWrite(BUZZER_PIN, LOW);
  
  // 2. Cập nhật biến trạng thái
  isEmergency = false; 
  isProcessing = true; // Đánh dấu là đang xử lý
  
  // 3. Gửi thông báo lên Web (để Web hiện màu Xanh Dương/Vàng)
  sendMQTT("hospital/room1/alert", "PROCESSING");
  
  // 4. Vẽ lại giao diện OLED (Hiện chữ "DANG XU LY")
  drawUI_Normal(); 
}

void setSafeState() {
  digitalWrite(BUZZER_PIN, LOW);
  isEmergency=false; isProcessing=false; isReminding=false;
  sendMQTT("hospital/room1/alert", "SAFE");
  sendToNode1("SAFE");
  drawUI_Normal();
}

// --- NHẬN ESP-NOW TỪ NODE 1 & NODE 3 ---
void OnDataRecv(const uint8_t * mac, const uint8_t *data, int len) {
  struct_message in; memcpy(&in, data, sizeof(in));
  String cmd = String(in.cmd);
  
  // 1. Tín hiệu SOS từ Node 1
  if (cmd=="SOS" || cmd=="FALL") {
    isEmergency=true; isProcessing=false;
    digitalWrite(BUZZER_PIN, HIGH); 
    sendMQTT("hospital/room1/alert", "EMERGENCY");
  } 
  
  // 2. Tín hiệu "TIẾP NHẬN" từ Node 3 (Gửi lệnh MUTE hoặc SAFE đều được xử lý)
  else if (cmd=="MUTE" || cmd=="SAFE") {
     // Khi Node 3 bấm nút, ta coi như Y tá đã đến và xử lý -> Tắt còi
     if (isEmergency) {
        setProcessingState(); // Chuyển sang trạng thái "Đã tiếp nhận" (Còi tắt)
     } else {
        setSafeState(); // Nếu không có gì thì Reset về Safe
     }
  }
  
  // 3. Điều khiển thiết bị từ Node khác (nếu có)
  else if (cmd=="LIGHT") ctrlLight(!isLightOn);
  else if (cmd=="FAN") ctrlFan(!isFanOn);
}

// --- NHẬN MQTT TỪ WEB ---
void mqttCallback(char* t, byte* p, unsigned int l) {
  String msg; for(int i=0;i<l;i++) msg+=(char)p[i];
  String topic = String(t);

  if(topic == "hospital/room1/control") {
    if (msg == "LIGHT_ON") ctrlLight(true);
    else if (msg == "LIGHT_OFF") ctrlLight(false);
    else if (msg == "FAN_ON") ctrlFan(true);
    else if (msg == "FAN_OFF") ctrlFan(false);
    else if (msg.startsWith("FAN_SPEED_")) {
       int speed = msg.substring(10).toInt();
       if(speed == 1) fanSpeedDelay = 30;
       if(speed == 2) fanSpeedDelay = 15;
       if(speed == 3) fanSpeedDelay = 5;
       if(!isFanOn) ctrlFan(true);
    }
    else if (msg == "CLEAR_ALARM") setSafeState();
    else if (msg == "MUTE_ALARM") setProcessingState(); // Web bấm nút xử lý -> Tắt còi
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);
  pinMode(PIN_SINGLE_LED, OUTPUT); digitalWrite(PIN_SINGLE_LED, LOW);
  
  servoDoor.attach(PIN_SERVO_DOOR); servoDoor.write(0);
  servoFan.attach(PIN_SERVO_FAN);   servoFan.write(0); 
  
  dht.begin(); SPI.begin(); mfrc522.PCD_Init();

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) for(;;);
  display.clearDisplay(); display.display();

  // Wifi & Time
  WiFi.mode(WIFI_AP_STA); WiFi.begin(WIFI_SSID, WIFI_PASS);
  while(WiFi.status()!=WL_CONNECTED) delay(500);
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  // MQTT
  espClient.setCACert(root_ca); 
  client.setServer(MQTT_SERVER, MQTT_PORT); 
  client.setCallback(mqttCallback);

  // ESP-NOW
  if(esp_now_init()==ESP_OK) esp_now_register_recv_cb(OnDataRecv);
  memcpy(peerInfo.peer_addr, mac_Node1, 6); 
  peerInfo.channel = WiFi.channel(); peerInfo.encrypt = false; 
  esp_now_add_peer(&peerInfo);
}

void loop() {
  if(!client.connected()) {
    String clientId = "Node2_" + String(random(0xffff), HEX);
    if(client.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      client.subscribe("hospital/room1/control");
      sendMQTT("hospital/room1/device/light", isLightOn ? "ON" : "OFF");
      sendMQTT("hospital/room1/device/fan", isFanOn ? "ON" : "OFF");
    }
  }
  client.loop();

  updateTime();

  if (isFanOn && (millis() - lastFanMove > fanSpeedDelay)) {
    lastFanMove = millis();
    fanPos += (2 * fanDirection); 
    if (fanPos >= 180 || fanPos <= 0) fanDirection = -fanDirection;
    servoFan.write(fanPos);
  }

  if (isDoorOpen && millis() - doorOpenTime > 3000) { servoDoor.write(0); isDoorOpen = false; }

  // RFID: Y tá quẹt thẻ -> Mở cửa & Tắt báo động
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
     digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
     triggerDoorOpen();
     if(isEmergency || isProcessing) setSafeState(); // Quẹt thẻ là an toàn luôn
     mfrc522.PICC_HaltA(); mfrc522.PCD_StopCrypto1();
  }

  if(millis() - lastDhtRead > 2000) {
    lastDhtRead = millis();
    float t = dht.readTemperature(); float h = dht.readHumidity();
    if(!isnan(t)) { 
      temp = t; hum = h;
      static int cnt=0; if(++cnt>=5){ sendMQTT("hospital/room1/env", String(t).c_str()); sendMQTT("hospital/room1/hum", String(h).c_str()); cnt=0; }
    }
  }

  // LOGIC CÒI: Chỉ kêu khi Emergency và chưa được Xử lý
  if(isEmergency && !isProcessing) {
     if((millis()/200)%2) digitalWrite(BUZZER_PIN, HIGH); else digitalWrite(BUZZER_PIN, LOW);
     drawUI_Alert("! SOS !", "CAN CUU HO");
  } 
  else if(isReminding) {
     if((millis()/500)%2) digitalWrite(BUZZER_PIN, HIGH); else digitalWrite(BUZZER_PIN, LOW);
     drawUI_Alert("NHAC NHO", "UONG THUOC");
     if(millis() - reminderStart > 60000) { isReminding = false; digitalWrite(BUZZER_PIN, LOW); }
  } 
  else {
     drawUI_Normal(); // Vẽ giao diện đồng hồ/nhiệt độ
  }
  
  struct tm ti;
  if(getLocalTime(&ti)){
     if((ti.tm_hour==8 || ti.tm_hour==18) && ti.tm_min==0 && ti.tm_sec==0 && !isReminding) {
        isReminding=true; reminderStart=millis(); sendMQTT("hospital/room1/alert", "MEDICATION");
     }
  }
}