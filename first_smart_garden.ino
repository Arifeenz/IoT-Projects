// ---------------------- BLYNK CONFIG ----------------------
#define BLYNK_TEMPLATE_ID "TMPL6kmkyh02W"
#define BLYNK_TEMPLATE_NAME "Quickstart Device"
#define BLYNK_AUTH_TOKEN "s8YHHKSjBv90KDx-FBK4b4OEz_WZ8KSM"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Servo.h>
#include <DHT.h>

char ssid[] = "ArifeenCH";
char pass[] = "arf313300";

// ---------------------- SENSOR + PIN CONFIG ----------------------
#define DHTPIN D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define SOIL_MOISTURE_PIN A0
#define RELAY_PIN D1
#define SERVO_1_PIN D5
#define SERVO_2_PIN D6

Servo servo1;
Servo servo2;

bool curtainOpen = false;
bool pumpOn = false;
bool curtainManualOverride = false;

unsigned long lastManualMillis = 0;
const unsigned long manualTimeout = 30000; // 30 วินาที

const int soilMin = 50;
const int soilMax = 400;
const int moistureThresholdPercent = 50;

int currentSoilPercent = 0;
float currentTemp = 0;

BlynkTimer timer;

// ---------------------- SETUP ----------------------
void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  dht.begin();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  servo1.attach(SERVO_1_PIN);
  servo2.attach(SERVO_2_PIN);
  servo1.write(90);
  servo2.write(90);

  timer.setInterval(2000L, readSensors);
  timer.setInterval(2000L, autoCurtainControl);
  timer.setInterval(2000L, autoWateringControl);
  timer.setInterval(2000L, sendSensorData);
}

// ---------------------- LOOP ----------------------
void loop() {
  Blynk.run();
  timer.run();

  // กลับเข้าโหมด auto หลังจาก manual timeout
  if (curtainManualOverride && (millis() - lastManualMillis > manualTimeout)) {
    curtainManualOverride = false;
    Serial.println("หมดเวลา manual → กลับสู่โหมดม่านอัตโนมัติ");
  }
}

// ---------------------- READ SENSOR ----------------------
void readSensors() {
  float temp = dht.readTemperature();
  if (!isnan(temp)) {
    currentTemp = temp;
  }

  int soilRaw = analogRead(SOIL_MOISTURE_PIN);
  int soilPercent = map(soilRaw, soilMin, soilMax, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);
  currentSoilPercent = soilPercent;

  Serial.print("Temp: ");
  Serial.print(currentTemp);
  Serial.print(" °C | Soil Raw: ");
  Serial.print(soilRaw);
  Serial.print(" | Soil %: ");
  Serial.println(currentSoilPercent);
}

// ---------------------- CURTAIN CONTROL ----------------------
void autoCurtainControl() {
  if (curtainManualOverride) return;

  if (currentTemp > 35.0 && !curtainOpen) {
    Serial.println("อุณหภูมิสูง > เปิดม่าน (auto)");
    servo1.write(180);
    servo2.write(180);
    delay(4000);
    servo2.write(90);
    servo1.write(180);
    delay(1000);
    servo1.write(90);
    curtainOpen = true;
    Blynk.virtualWrite(V3, 1);
  }
  else if (currentTemp <= 35.0 && curtainOpen) {
    Serial.println("อุณหภูมิต่ำ <= ปิดม่าน (auto)");
    servo1.write(0);
    servo2.write(0);
    delay(4200);
    servo1.write(90);
    servo2.write(90);
    curtainOpen = false;
    Blynk.virtualWrite(V3, 0);
  }
}

// ---------------------- WATERING CONTROL ----------------------
void autoWateringControl() {
  if (currentSoilPercent < moistureThresholdPercent) {
    digitalWrite(RELAY_PIN, LOW);
    pumpOn = true;
  } else {
    digitalWrite(RELAY_PIN, HIGH);
    pumpOn = false;
  }
}

// ---------------------- SEND TO BLYNK ----------------------
void sendSensorData() {
  Blynk.virtualWrite(V0, currentSoilPercent);
  Blynk.virtualWrite(V1, currentTemp);
  Blynk.virtualWrite(V2, pumpOn ? 1 : 0);
  Blynk.virtualWrite(V3, curtainOpen ? 1 : 0);
}

// ---------------------- MANUAL OPEN CURTAIN ----------------------
BLYNK_WRITE(V4) {
  int value = param.asInt();
  curtainManualOverride = true;
  lastManualMillis = millis();

  if (value == 1) {
    servo1.write(180);
    servo2.write(180);
    curtainOpen = true;
    Blynk.virtualWrite(V3, 1);
    Serial.println("เปิดม่าน (manual)");
  } else {
    servo1.write(90);
    servo2.write(90);
    Serial.println("หยุดม่าน (manual)");
  }
}

// ---------------------- MANUAL CLOSE CURTAIN ----------------------
BLYNK_WRITE(V5) {
  int value = param.asInt();
  curtainManualOverride = true;
  lastManualMillis = millis();

  if (value == 1) {
    servo1.write(0);
    servo2.write(0);
    curtainOpen = false;
    Blynk.virtualWrite(V3, 0);
    Serial.println("ปิดม่าน (manual)");
  } else {
    servo1.write(90);
    servo2.write(90);
    Serial.println("หยุดม่าน (manual)");
  }
}
