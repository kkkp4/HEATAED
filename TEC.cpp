#include <OneWire.h>
#include <DallasTemperature.h>

// ================= PIN =================
#define ONE_WIRE_BUS 4   // DS18B20 DATA
#define TEC_PIN 25       // PWM -> MOSFET (TEC)
#define FAN_PIN 26       // PWM -> MOSFET (FAN)

// ================= PWM =================
#define CH_TEC 0
#define CH_FAN 1

// ================= DS18B20 =================
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int pctToDuty(float pct){
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return (int)(pct / 100.0 * 1023);
}

void setup() {
  Serial.begin(115200);

  // เริ่ม DS18B20
  sensors.begin();

  // PWM สำหรับ TEC (2 kHz)
  ledcSetup(CH_TEC, 2000, 10);      // channel, freq, resolution
  ledcAttachPin(TEC_PIN, CH_TEC);

  // PWM สำหรับพัดลม (25 kHz)
  ledcSetup(CH_FAN, 25000, 10);
  ledcAttachPin(FAN_PIN, CH_FAN);

  // ตั้งค่า PWM คงที่ (ปรับได้)
  ledcWrite(CH_TEC, pctToDuty(80)); // 60%
  ledcWrite(CH_FAN, pctToDuty(80)); // 70%

  Serial.println("System started");
}

void loop() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("DS18B20 not found");
  } else {
    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.println(" °C");
  }

  delay(1000);  // อ่านทุก 1 วินาที
}
