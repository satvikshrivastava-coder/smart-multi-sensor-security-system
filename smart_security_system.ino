#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= DHT =================
#define DHTPIN 7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ================= PINS =================
#define PIR_PIN    2
#define LDR_PIN    A0
#define GREEN_LED  9
#define RED_LED    8
#define BUZZER     4

// ================= MPU6050 =================
Adafruit_MPU6050 mpu;
const float MOTION_THRESHOLD = 1.0; // G-force threshold for tamper detection

// ================= STATE MACHINE =================
// Three states: NORMAL → ALERT → COOLDOWN → NORMAL
enum SystemState { NORMAL, ALERT, COOLDOWN };
SystemState currentState = NORMAL;

// ================= TIMING (millis-based, no delay) =================
// Each sensor has its own independent poll interval
unsigned long lastPIRCheck     = 0;  const unsigned long PIR_INTERVAL     = 100;   // ms
unsigned long lastLDRCheck     = 0;  const unsigned long LDR_INTERVAL     = 1000;  // ms
unsigned long lastDHTCheck     = 0;  const unsigned long DHT_INTERVAL     = 2000;  // ms
unsigned long lastMPUCheck     = 0;  const unsigned long MPU_INTERVAL     = 200;   // ms
unsigned long lastLCDUpdate    = 0;  const unsigned long LCD_INTERVAL     = 500;   // ms
unsigned long lastSerialPrint  = 0;  const unsigned long SERIAL_INTERVAL  = 1000;  // ms

// Alert holds for 5 seconds after trigger clears (prevents buzzer stutter)
unsigned long alertStartTime   = 0;  const unsigned long ALERT_HOLD       = 5000;  // ms
// Cooldown period before system re-arms (prevents immediate re-trigger)
unsigned long cooldownStart    = 0;  const unsigned long COOLDOWN_PERIOD  = 3000;  // ms

// ================= SENSOR STATE VARIABLES =================
int   pirState   = LOW;
int   lightValue = 1023;  // default: bright (safe)
float temp       = 0.0;
float hum        = 0.0;
float ax         = 0.0;
float ay         = 0.0;

bool  mpuAlert   = false;
bool  nightMode  = false;  // true when LDR reads dark (lightValue < 500)

// ================= SETUP =================
void setup() {
  Serial.begin(9600);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("System Starting");
  lcd.setCursor(0, 1); lcd.print("Please Wait...");
  delay(2000); // only delay allowed: boot splash, before loop starts

  // Sensors
  dht.begin();
  pinMode(PIR_PIN, INPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED,   OUTPUT);
  pinMode(BUZZER,    OUTPUT);

  // Safe default output state
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED,   LOW);
  digitalWrite(BUZZER,    LOW);

  // MPU6050
  if (!mpu.begin()) {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("MPU6050 ERROR");
    while (1); // halt — sensor is critical
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("System Ready");
  lcd.setCursor(0, 1); lcd.print("Monitoring...");
  delay(1000);
  lcd.clear();
}

// ================= MAIN LOOP =================
void loop() {
  unsigned long now = millis();

  // ---- 1. POLL SENSORS (each on its own interval) ----

  // PIR: check every 100ms
  if (now - lastPIRCheck >= PIR_INTERVAL) {
    lastPIRCheck = now;
    pirState = digitalRead(PIR_PIN);
  }

  // LDR: check every 1000ms — also sets nightMode flag
  if (now - lastLDRCheck >= LDR_INTERVAL) {
    lastLDRCheck = now;
    lightValue = analogRead(LDR_PIN);
    // nightMode = true when light level is low (dark environment)
    // In night mode, PIR alerts are suppressed to reduce false positives
    nightMode = (lightValue < 500);
  }

  // DHT22: check every 2000ms (sensor hardware limit is ~0.5Hz)
  if (now - lastDHTCheck >= DHT_INTERVAL) {
    lastDHTCheck = now;
    float newTemp = dht.readTemperature();
    float newHum  = dht.readHumidity();
    // Only update if reading is valid (DHT returns NaN on failure)
    if (!isnan(newTemp)) temp = newTemp;
    if (!isnan(newHum))  hum  = newHum;
  }

  // MPU6050: check every 200ms
  if (now - lastMPUCheck >= MPU_INTERVAL) {
    lastMPUCheck = now;
    sensors_event_t a, g, tempSensor;
    mpu.getEvent(&a, &g, &tempSensor);
    ax = abs(a.acceleration.x / 9.8);
    ay = abs(a.acceleration.y / 9.8);
    mpuAlert = (ax >= MOTION_THRESHOLD || ay >= MOTION_THRESHOLD);
  }

  // ---- 2. STATE MACHINE ----

  switch (currentState) {

    case NORMAL:
      // PIR alert is suppressed in night mode (LDR-based suppression)
      // MPU tamper alert always active regardless of light level
      if (mpuAlert || (!nightMode && pirState == HIGH)) {
        currentState   = ALERT;
        alertStartTime = now;
      }
      break;

    case ALERT:
      // Hold alert for ALERT_HOLD ms after trigger clears (no buzzer stutter)
      if (!mpuAlert && pirState == LOW) {
        // Trigger has cleared — check if hold time has elapsed
        if (now - alertStartTime >= ALERT_HOLD) {
          currentState  = COOLDOWN;
          cooldownStart = now;
        }
      } else {
        // Trigger still active — reset the hold timer
        alertStartTime = now;
      }
      break;

    case COOLDOWN:
      // System re-arms after COOLDOWN_PERIOD ms — prevents immediate re-trigger
      if (now - cooldownStart >= COOLDOWN_PERIOD) {
        currentState = NORMAL;
      }
      break;
  }

  // ---- 3. OUTPUT CONTROL (driven by state, not raw sensor reads) ----

  if (currentState == ALERT) {
    digitalWrite(RED_LED,   HIGH);
    digitalWrite(BUZZER,    HIGH);
    digitalWrite(GREEN_LED, LOW);
  } else {
    // NORMAL and COOLDOWN both show safe outputs
    digitalWrite(RED_LED,   LOW);
    digitalWrite(BUZZER,    LOW);
    // During cooldown blink green LED to show system is re-arming
    if (currentState == COOLDOWN) {
      digitalWrite(GREEN_LED, (now / 500) % 2); // blink every 500ms
    } else {
      digitalWrite(GREEN_LED, HIGH);
    }
  }

  // ---- 4. LCD UPDATE (every 500ms, non-blocking) ----

  if (now - lastLCDUpdate >= LCD_INTERVAL) {
    lastLCDUpdate = now;
    lcd.clear();

    if (currentState == ALERT) {
      lcd.setCursor(0, 0);
      // Tamper alert takes priority over motion alert on display
      if (mpuAlert)           lcd.print("TAMPER ALERT!");
      else if (pirState == HIGH) lcd.print("MOTION ALERT!");
      lcd.setCursor(0, 1);    lcd.print("Security Risk!");

    } else if (currentState == COOLDOWN) {
      lcd.setCursor(0, 0); lcd.print("Re-Arming...");
      lcd.setCursor(0, 1); lcd.print("Please Wait");

    } else {
      // NORMAL state — show environment data
      lcd.setCursor(0, 0);
      if (nightMode) lcd.print("Night Mode ON");
      else           lcd.print("SAFE MODE");

      lcd.setCursor(0, 1);
      lcd.print("T:");  lcd.print(temp, 1);
      lcd.print("C H:"); lcd.print(hum, 0);
      lcd.print("%");
    }
  }

  // ---- 5. SERIAL MONITOR (every 1000ms, non-blocking) ----

  if (now - lastSerialPrint >= SERIAL_INTERVAL) {
    lastSerialPrint = now;

    // State label for serial readability
    const char* stateLabel = (currentState == NORMAL)   ? "NORMAL"   :
                             (currentState == ALERT)    ? "ALERT"    : "COOLDOWN";

    Serial.print("State: ");    Serial.print(stateLabel);
    Serial.print(" | Light: "); Serial.print(lightValue);
    Serial.print(" Night: ");   Serial.print(nightMode ? "YES" : "NO");
    Serial.print(" | PIR: ");   Serial.print(pirState);
    Serial.print(" | Temp: ");  Serial.print(temp, 1);
    Serial.print("C Hum: ");    Serial.print(hum, 0);
    Serial.print("% | AX: ");   Serial.print(ax, 2);
    Serial.print(" AY: ");      Serial.println(ay, 2);
  }

  // No delay() here — loop runs freely, timing handled entirely by millis()
}
