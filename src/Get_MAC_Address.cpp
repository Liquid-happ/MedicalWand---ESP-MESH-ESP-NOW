#include <WiFi.h>
#include <esp_bt.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("--- THÔNG TIN ĐỊA CHỈ MAC (ESP32) ---");

  WiFi.mode(WIFI_STA); 
  String mac_wifi = WiFi.macAddress();
  Serial.print("1. Địa chỉ MAC WiFi (STA): ");
  Serial.println(mac_wifi);
  Serial.println("------------------------------------");

  uint8_t baseMac[6];
  esp_efuse_mac_get_default(baseMac);
  
  char mac_bt[18] = {0};
  sprintf(mac_bt, "%02X:%02X:%02X:%02X:%02X:%02X", 
          baseMac[0], baseMac[1], baseMac[2], 
          baseMac[3], baseMac[4], baseMac[5]);

  Serial.print("2. Địa chỉ MAC cơ sở (Base MAC): ");
  Serial.println(mac_bt);
  Serial.println("   (Đây là MAC mà ESP-NOW sử dụng)");
  Serial.println("------------------------------------");
}

void loop() {
}
