#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <DHT.h>

#define BLYNK_TEMPLATE_ID "TMPL63gfXiBWP"
#define BLYNK_TEMPLATE_NAME "Gas Leak Detector"
#define BLYNK_AUTH_TOKEN "lTS6E2BqTouC-pmtlhCfiQiUbpJpmfed"

// ----- Pin Configuration -----
#define MQ2_PIN        34    // Analog input (safe ADC pin)
#define BUZZER_PIN     14
#define LED_PIN        27
#define SERVO_PIN      18
#define DHT_PIN        4     // DHT22 data pin

// ----- LCD and Sensor Setup -----
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Confirm your LCD's I2C address (usually 0x27 or 0x3F)
Servo gasValveServo;
DHT dht(DHT_PIN, DHT22);

// ----- MQ2 Calibration -----
const float R0 = 15.36;
const float RL = 5.0;
const int thresholdPPM = 300;

bool gasShutOff = false;

void setup() {
  Serial.begin(115200);

  // Initialize I2C for ESP32 - set SDA = GPIO 21, SCL = GPIO 22 (change if needed)
  Wire.begin(21, 22);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  lcd.init();            // Use init() instead of begin() for ESP32 compatibility
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("MQ-2 + DHT22 Init");
  delay(2000);
  lcd.clear();

  gasValveServo.attach(SERVO_PIN);
  gasValveServo.write(0); // Gas ON (default)

  dht.begin();

  Serial.println("Setup complete");
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    temperature = 25.0; // fallback default temp
    humidity = 50.0;    // fallback default humidity
  }

  int sensorValue = analogRead(MQ2_PIN);
  float voltage = sensorValue * (3.3 / 4095.0); // For 12-bit ADC
  float sensorRs = RL * (3.3 - voltage) / voltage;
  float ratio = sensorRs / R0;

  // LPG curve coefficients for MQ2 sensor (approximate)
  float m = -0.47;
  float b = 0.38;
  float ppm = pow(10, (log10(ratio) - b) / m);

  // Apply temperature correction
  float correctedPPM = ppm * (1 + (temperature - 25.0) * 0.01);  // ±1%/°C correction

  // Serial debug output
  Serial.print("Temp: "); Serial.print(temperature);
  Serial.print(" C, Hum: "); Serial.print(humidity);
  Serial.print(" %, PPM: "); Serial.println((int)correctedPPM);

  // Update LCD without clearing whole screen
  lcd.setCursor(0, 0);
  lcd.print("PPM:      ");  // Clear line 1
  lcd.setCursor(5, 0);
  lcd.print((int)correctedPPM);

  lcd.setCursor(0, 1);
  lcd.print("T:    H:   ");  // Clear line 2
  lcd.setCursor(2, 1);
  lcd.print((int)temperature);
  lcd.setCursor(7, 1);
  lcd.print((int)humidity);

  // Alert logic
  if (correctedPPM > thresholdPPM) {
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    lcd.setCursor(11, 0);
    lcd.print("ALERT");

    if (!gasShutOff) {
      gasValveServo.write(90);  // Close gas valve
      gasShutOff = true;
      Serial.println("Gas valve CLOSED due to gas leak");
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    lcd.setCursor(11, 0);
    lcd.print("     ");       // Clear ALERT
    if (gasShutOff) {
      gasValveServo.write(0);    // Open gas valve
      gasShutOff = false;
      Serial.println("Gas valve OPEN");
    }
  }

  delay(2000);
}