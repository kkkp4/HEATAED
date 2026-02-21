#include <Arduino.h>
#include <Wire.h>

// ---------- MLX90614 ----------
#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// ---------- MAX30102 ----------
#include "MAX30105.h"
#include "spo2_algorithm.h"

// ---------- DS18B20 ----------
#include <OneWire.h>
#include <DallasTemperature.h>

// ================= BUTTON =================
#define BUTTON_PIN 13

// ================= DS18B20 PINS =================
#define ONE_WIRE_BUS1 4
#define ONE_WIRE_BUS2 16
#define ONE_WIRE_BUS3 17

// ================= TEC PINS =================
#define TEC1_PIN 25
#define TEC2_PIN 27
#define TEC3_PIN 33

// ================= FAN PINS =================
#define FAN1_PIN 26
#define FAN2_PIN 14
#define FAN3_PIN 32

// ================= PWM CHANNEL =================
#define CH_TEC1 0
#define CH_TEC2 1
#define CH_TEC3 2
#define CH_FAN1 3
#define CH_FAN2 4
#define CH_FAN3 5

// ================= DS18B20 OBJECT =================
OneWire oneWire1(ONE_WIRE_BUS1);
OneWire oneWire2(ONE_WIRE_BUS2);
OneWire oneWire3(ONE_WIRE_BUS3);

DallasTemperature sensor1(&oneWire1);
DallasTemperature sensor2(&oneWire2);
DallasTemperature sensor3(&oneWire3);

// ================= SYSTEM STATE =================
bool systemRunning = false;
bool lastButtonState = HIGH;

// ================= MAX30102 =================
MAX30105 particleSensor;
const int bufferSize = 100;
uint32_t irBuffer[bufferSize];
uint32_t redBuffer[bufferSize];

int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

// ================= FUNCTION =================
int pctToDuty(float pct){
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return (int)(pct / 100.0 * 1023);
}

void turnOffAll() {
  ledcWrite(CH_TEC1, 0);
  ledcWrite(CH_TEC2, 0);
  ledcWrite(CH_TEC3, 0);
  ledcWrite(CH_FAN1, 0);
  ledcWrite(CH_FAN2, 0);
  ledcWrite(CH_FAN3, 0);
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // ===== DS18B20 =====
  sensor1.begin();
  sensor2.begin();
  sensor3.begin();

  // ===== PWM =====
  ledcSetup(CH_TEC1, 2000, 10);
  ledcSetup(CH_TEC2, 2000, 10);
  ledcSetup(CH_TEC3, 2000, 10);

  ledcAttachPin(TEC1_PIN, CH_TEC1);
  ledcAttachPin(TEC2_PIN, CH_TEC2);
  ledcAttachPin(TEC3_PIN, CH_TEC3);

  ledcSetup(CH_FAN1, 25000, 10);
  ledcSetup(CH_FAN2, 25000, 10);
  ledcSetup(CH_FAN3, 25000, 10);

  ledcAttachPin(FAN1_PIN, CH_FAN1);
  ledcAttachPin(FAN2_PIN, CH_FAN2);
  ledcAttachPin(FAN3_PIN, CH_FAN3);

  // ===== I2C =====
  Wire.begin(21, 22);
  Wire.setClock(100000);

  // ===== MLX90614 =====
  if (!mlx.begin(0x5A, &Wire)) {
    Serial.println("MLX90614 not found");
    while (1);
  }

  // ===== MAX30102 =====
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found");
    while (1);
  }

  byte ledBrightness = 60;
  byte sampleAverage = 4;
  byte ledMode = 2;
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

  Serial.println("System Ready - Press Button to Start");
}

void loop() {

  // ===== อ่านปุ่ม =====
  bool currentButtonState = digitalRead(BUTTON_PIN);

  if (lastButtonState == HIGH && currentButtonState == LOW) {
    systemRunning = !systemRunning;

    if (systemRunning) {
      Serial.println("SYSTEM ON");
    } else {
      Serial.println("SYSTEM OFF");
      turnOffAll();
    }

    delay(200); // debounce
  }

  lastButtonState = currentButtonState;

  // ถ้ายังไม่เปิดระบบ → ไม่ทำอะไรเลย
  if (!systemRunning) {
    return;
  }

  // ===== อ่าน MLX90614 =====
  float objectTempC = mlx.readObjectTempC();

  // ===== อ่าน MAX30102 =====
  for (int i = 0; i < bufferSize; i++) {
    while (!particleSensor.available()) {
      particleSensor.check();
    }

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i]  = particleSensor.getIR();
    particleSensor.nextSample();
  }

  maxim_heart_rate_and_oxygen_saturation(
    irBuffer,
    bufferSize,
    redBuffer,
    &spo2,
    &validSPO2,
    &heartRate,
    &validHeartRate
  );

  // ===== อ่าน DS18B20 =====
  sensor1.requestTemperatures();
  sensor2.requestTemperatures();
  sensor3.requestTemperatures();

  float temp1 = sensor1.getTempCByIndex(0);
  float temp2 = sensor2.getTempCByIndex(0);
  float temp3 = sensor3.getTempCByIndex(0);

  // ===== แสดงผล =====
  Serial.print("Temp: ");
  Serial.print(objectTempC);
  Serial.print(" °C | HR: ");

  if (validHeartRate)
    Serial.print(heartRate);
  else
    Serial.print("--");

  Serial.print(" BPM | SpO2: ");

  if (validSPO2)
    Serial.print(spo2);
  else
    Serial.print("--");

  Serial.println(" %");

  Serial.print("T1: "); Serial.print(temp1);
  Serial.print(" | T2: "); Serial.print(temp2);
  Serial.print(" | T3: "); Serial.println(temp3);

  // ===== ระบบควบคุม =====
  if (objectTempC > 38.5 && temp1 >= 10 && temp2 >= 10 && temp3 >= 10) {

    Serial.println("DANGER");
    ledcWrite(CH_TEC1, pctToDuty(90));
    ledcWrite(CH_TEC2, pctToDuty(90));
    ledcWrite(CH_TEC3, pctToDuty(90));
    ledcWrite(CH_FAN1, pctToDuty(90));
    ledcWrite(CH_FAN2, pctToDuty(90));
    ledcWrite(CH_FAN3, pctToDuty(90));

  }
  else if (objectTempC >= 38.1 && objectTempC <= 38.5 &&
           temp1 >= 10 && temp2 >= 10 && temp3 >= 10) {

    Serial.println("ALERT");
    ledcWrite(CH_TEC1, pctToDuty(80));
    ledcWrite(CH_TEC2, pctToDuty(80));
    ledcWrite(CH_TEC3, pctToDuty(80));
    ledcWrite(CH_FAN1, pctToDuty(80));
    ledcWrite(CH_FAN2, pctToDuty(80));
    ledcWrite(CH_FAN3, pctToDuty(80));

  }
  else {
    Serial.println("NORMAL");
    turnOffAll();
  }

  Serial.println("-----------------------");

  delay(1000);
}
