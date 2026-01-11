#include <Arduino.h>
#include <Wire.h>

// ---------- MLX90614 ----------
#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// ---------- MAX30102 ----------
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

#define BUFFER_SIZE 100
uint32_t irBuffer[BUFFER_SIZE];
uint32_t redBuffer[BUFFER_SIZE];

int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

void setup() {
  Serial.begin(115200);

  // I2C (ใช้ร่วมกัน)
  Wire.begin(21, 22);
  Wire.setClock(100000); // ลด noise ให้เสถียร

  // ---------- MLX90614 ----------
  if (!mlx.begin(0x5A, &Wire)) {
    Serial.println("❌ MLX90614 not found");
    while (1);
  }
  Serial.println("✅ MLX90614 Ready");

  // ---------- MAX30102 ----------
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("❌ MAX30102 not found");
    while (1);
  }
  Serial.println("✅ MAX30102 Ready");

  // ตั้งค่า MAX30102
  byte ledBrightness = 60;
  byte sampleAverage = 4;
  byte ledMode = 2;        // Red + IR
  int sampleRate = 100;
  int pulseWidth = 411;
  int adcRange = 4096;

  particleSensor.setup(
    ledBrightness,
    sampleAverage,
    ledMode,
    sampleRate,
    pulseWidth,
    adcRange
  );

  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x1F);

  Serial.println("👉 Place finger on MAX30102 sensor");
}

void loop() {
  // ---------- อ่านอุณหภูมิ (MLX90614) ----------
  float objectTempC = mlx.readObjectTempC();

  // ---------- อ่านข้อมูล MAX30102 ----------
  for (int i = 0; i < BUFFER_SIZE; i++) {
    while (!particleSensor.available()) {
      particleSensor.check();
    }

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i]  = particleSensor.getIR();
    particleSensor.nextSample();
  }

  // คำนวณ HR + SpO₂
  maxim_heart_rate_and_oxygen_saturation(
    irBuffer,
    BUFFER_SIZE,
    redBuffer,
    &spo2,
    &validSPO2,
    &heartRate,
    &validHeartRate
  );

  // ---------- แสดงผล ----------
  Serial.print("Temp: ");
  Serial.print(objectTempC, 2);
  Serial.print(" °C | HR: ");

  if (validHeartRate)
    Serial.print(heartRate);
  else
    Serial.print("--");

  Serial.print(" BPM | SpO₂: ");

  if (validSPO2)
    Serial.print(spo2);
  else
    Serial.print("--");

  Serial.println(" %");
  Serial.println("-----------------------");

  delay(1000);
}
