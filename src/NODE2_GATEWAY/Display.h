#ifndef DISPLAY_H
#define DISPLAY_H

#include "Config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Control.h" 

Adafruit_SSD1306 display(128, 64, &Wire, -1);
extern char timeStr[10];
extern float temp;
extern float hum;

void setupDisplay() {
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) for(;;);
    display.clearDisplay(); display.display();
}

void updateTime() {
    struct tm ti;
    if(getLocalTime(&ti)) {
        sprintf(timeStr, "%02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);
    } else {
        strcpy(timeStr, "--:--:--");
    }
}

void drawUI_Normal() {
    display.clearDisplay();
    display.fillRect(0, 0, 128, 12, WHITE);
    display.setTextColor(BLACK); display.setTextSize(1); display.setCursor(2, 2);
    
    if(isProcessing) display.print("! DANG XU LY !");
    else if(isReminding) display.print("! NHAC NHO !");
    else display.print("PHONG 01 - OK");

    display.setTextColor(WHITE); display.setTextSize(2); 
    display.setCursor(15, 16); display.print(timeStr); // Giờ:Phút:Giây

    display.drawFastHLine(0, 36, 128, WHITE);
    
    // Nhiệt độ & Độ ẩm
    display.setTextSize(1);
    display.setCursor(0, 42); display.print("T: "); display.print((int)temp); display.print("C");
    display.setCursor(0, 54); display.print("H: "); display.print((int)hum); display.print("%");

    // Icon Đèn (L)
    if(isLightOn) display.fillCircle(85, 48, 5, WHITE); else display.drawCircle(85, 48, 5, WHITE); display.setCursor(80, 56); display.print("L");

    // Icon Quạt (F)
    if(isFanOn) {
       int f = (millis()/200)%2; 
       if(f) display.drawLine(105,48,115,48,WHITE); else display.drawLine(110,43,110,53,WHITE);
    } else display.drawRect(105, 43, 10, 10, WHITE);
    display.setCursor(105, 56); display.print("F");
    display.display();
}

// Giao diện Cảnh báo (Full screen)
void drawUI_Alert(String title, String msg) {
    display.clearDisplay();
    display.fillScreen(WHITE);
    display.setTextColor(BLACK);
    display.setTextSize(2); display.setCursor(5, 10); display.println(title);
    display.setTextSize(1); display.setCursor(5, 35); display.println(msg); display.display();
}

#endif
