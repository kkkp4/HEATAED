#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// สำหรับ SH1106 ใช้ class นี้
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  delay(200);

  Wire.begin(21, 22);
  Wire.setClock(100000);

  if (!display.begin(0x3C, true)) {   // true = reset display
    Serial.println("❌ SH1106 not found");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("SH1106 OLED OK");
  display.println("ESP32 Ready");
  display.display();
}

void loop() {}
