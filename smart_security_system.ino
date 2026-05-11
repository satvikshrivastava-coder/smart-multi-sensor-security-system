// ======================================================
// SMART MULTI-SENSOR SECURITY & ENVIRONMENT SYSTEM
// ======================================================

// ================= LIBRARIES =================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// ================= LCD SETUP =================

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= DHT SENSOR =================

#define DHTPIN 7
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// ================= PIN DEFINITIONS =================

#define PIR_PIN 2
#define LDR_PIN A0

#define GREEN_LED 9
#define RED_LED 8
#define BUZZER 4

// ================= MPU6050 OBJECT =================

Adafruit_MPU6050 mpu;

// ================= VARIABLES =================

// Motion threshold for MPU6050
float motionThreshold = 1.0;

// ======================================================
// SETUP FUNCTION
// ======================================================

void setup() {

  // Start serial communication
  Serial.begin(9600);

  // ================= LCD START =================

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("System Starting");

  lcd.setCursor(0, 1);
  lcd.print("Please Wait...");
  
  delay(2000);

  // ================= SENSOR START =================

  dht.begin();

  pinMode(PIR_PIN, INPUT);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // Default safe condition
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER, LOW);

  // ================= MPU6050 START =================

  if (!mpu.begin()) {

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MPU6050 ERROR");

    while (1);
  }

  // Configure MPU6050
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Ready message
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("System Ready");

  lcd.setCursor(0, 1);
  lcd.print("Monitoring...");

  delay(2000);
}

// ======================================================
// LOOP FUNCTION
// ======================================================

void loop() {

  // ================= READ SENSOR VALUES =================

  int pirState = digitalRead(PIR_PIN);

  int lightValue = analogRead(LDR_PIN);

  float temperature = dht.readTemperature();

  float humidity = dht.readHumidity();

  // ================= READ MPU6050 =================

  sensors_event_t a, g, temp;

  mpu.getEvent(&a, &g, &temp);

  // Convert acceleration into G force
  float accelX = abs(a.acceleration.x / 9.8);

  float accelY = abs(a.acceleration.y / 9.8);

  // ================= MPU6050 MOTION CHECK =================

  bool mpuAlert = false;

  // Ignore Z-axis and check only X and Y
  if (accelX >= motionThreshold || accelY >= motionThreshold) {

    mpuAlert = true;
  }

  // ================= ALERT LOGIC =================

  bool alertState = false;

  // Activate alert if motion or tampering detected
  if (pirState == HIGH || mpuAlert == true) {

    alertState = true;
  }

  // ================= OUTPUT CONTROL =================

  if (alertState == true) {

    // Turn ON alarm outputs
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER, HIGH);

    // Turn OFF safe indicator
    digitalWrite(GREEN_LED, LOW);

    // ================= LCD ALERT DISPLAY =================

    lcd.clear();

    if (pirState == HIGH) {

      lcd.setCursor(0, 0);
      lcd.print("MOTION ALERT!");
    }

    else if (mpuAlert == true) {

      lcd.setCursor(0, 0);
      lcd.print("TAMPER ALERT!");
    }

    lcd.setCursor(0, 1);
    lcd.print("Security Risk");
  }

  else {

    // Safe condition
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER, LOW);

    digitalWrite(GREEN_LED, HIGH);

    // ================= LCD SAFE DISPLAY =================

    lcd.clear();

    // Light condition check
    if (lightValue < 500) {

      lcd.setCursor(0, 0);
      lcd.print("Night Mode ON");
    }

    else {

      lcd.setCursor(0, 0);
      lcd.print("SAFE MODE");
    }

    // Temperature and humidity display
    lcd.setCursor(0, 1);

    lcd.print("T:");
    lcd.print(temperature, 1);

    lcd.print("C H:");
    lcd.print(humidity, 0);
  }

  // ================= SERIAL MONITOR OUTPUT =================

  Serial.print("Light: ");
  Serial.print(lightValue);

  Serial.print(" | PIR: ");
  Serial.print(pirState);

  Serial.print(" | Temp: ");
  Serial.print(temperature);

  Serial.print(" | Humidity: ");
  Serial.print(humidity);

  Serial.print(" | AX: ");
  Serial.print(accelX);

  Serial.print(" | AY: ");
  Serial.println(accelY);

  delay(1000);
}