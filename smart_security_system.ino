#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DHTPIN 7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define PIR_PIN 2
#define LDR_PIN A0
#define GREEN_LED 9
#define RED_LED 8
#define BUZZER 4

Adafruit_MPU6050 mpu;

const float MOTION_THRESHOLD = 1.0;
const int DARK_THRESHOLD = 500;
const unsigned long SENSOR_INTERVAL = 1000;

unsigned long lastSensorRead = 0;

String lastLCDLine0 = "";
String lastLCDLine1 = "";

enum SystemState {
  SAFE,
  NIGHT_MODE,
  MOTION_ALERT,
  TAMPER_ALERT,
  MOTION_TAMPER_ALERT
};

SystemState currentState = SAFE;

volatile bool motionDetectedFlag = false;

void motionISR() {
  motionDetectedFlag = true;
}

void updateLCD(String line0, String line1) {
  if (line0 != lastLCDLine0 || line1 != lastLCDLine1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line0);
    lcd.setCursor(0, 1);
    lcd.print(line1);
    lastLCDLine0 = line0;
    lastLCDLine1 = line1;
  }
}

void applyState(SystemState state, float temp, float hum) {
  switch (state) {
    case SAFE:
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BUZZER, LOW);
      updateLCD("SAFE MODE", "T:" + String(temp, 1) + "C H:" + String(hum, 0));
      break;

    case NIGHT_MODE:
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BUZZER, LOW);
      updateLCD("NIGHT MODE", "T:" + String(temp, 1) + "C H:" + String(hum, 0));
      break;

    case MOTION_ALERT:
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BUZZER, HIGH);
      digitalWrite(GREEN_LED, LOW);
      updateLCD("MOTION ALERT!", "Security Risk");
      break;

    case TAMPER_ALERT:
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BUZZER, HIGH);
      digitalWrite(GREEN_LED, LOW);
      updateLCD("TAMPER ALERT!", "Security Risk");
      break;

    case MOTION_TAMPER_ALERT:
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BUZZER, HIGH);
      digitalWrite(GREEN_LED, LOW);
      updateLCD("MOTION+TAMPER!", "Security Risk");
      break;
  }
}

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System Starting");
  lcd.setCursor(0, 1);
  lcd.print("Please Wait...");
  delay(2000);

  dht.begin();

  pinMode(PIR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), motionISR, RISING);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER, LOW);

  if (!mpu.begin()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MPU6050 ERROR");
    while (1);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  lcd.setCursor(0, 1);
  lcd.print("Monitoring...");
  delay(2000);
}

void loop() {
  if (motionDetectedFlag && currentState != MOTION_ALERT && currentState != MOTION_TAMPER_ALERT) {
    currentState = MOTION_ALERT;
    applyState(currentState, 0.0, 0.0);
    Serial.println(">>> INTERRUPT: MOTION DETECTED <<<");
  }

  unsigned long currentTime = millis();

  if (currentTime - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = currentTime;

    int lightValue = analogRead(LDR_PIN);
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
      Serial.println("DHT22 READ FAILED - skipping cycle");
      return;
    }

    sensors_event_t a, g, tempSensor;
    mpu.getEvent(&a, &g, &tempSensor);

    float ax = abs(a.acceleration.x / 9.8);
    float ay = abs(a.acceleration.y / 9.8);

    bool pirTriggered = motionDetectedFlag || (digitalRead(PIR_PIN) == HIGH);
    motionDetectedFlag = false;

    bool tamperTriggered = (ax >= MOTION_THRESHOLD || ay >= MOTION_THRESHOLD);
    bool nightMode = (lightValue < DARK_THRESHOLD);

    if (pirTriggered && tamperTriggered) {
      currentState = MOTION_TAMPER_ALERT;
    } else if (pirTriggered) {
      currentState = MOTION_ALERT;
    } else if (tamperTriggered) {
      currentState = TAMPER_ALERT;
    } else if (nightMode) {
      currentState = NIGHT_MODE;
    } else {
      currentState = SAFE;
    }

    applyState(currentState, temp, hum);

    Serial.print("State: ");
    switch (currentState) {
      case SAFE:                Serial.print("SAFE"); break;
      case NIGHT_MODE:          Serial.print("NIGHT_MODE"); break;
      case MOTION_ALERT:        Serial.print("MOTION_ALERT"); break;
      case TAMPER_ALERT:        Serial.print("TAMPER_ALERT"); break;
      case MOTION_TAMPER_ALERT: Serial.print("MOTION_TAMPER_ALERT"); break;
    }

    Serial.print(" | Light: "); Serial.print(lightValue);
    Serial.print(" | PIR: "); Serial.print(pirTriggered);
    Serial.print(" | Temp: "); Serial.print(temp);
    Serial.print(" | Hum: "); Serial.print(hum);
    Serial.print(" | AX: "); Serial.print(ax);
    Serial.print(" | AY: "); Serial.println(ay);
  }
}
