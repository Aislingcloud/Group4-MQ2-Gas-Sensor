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
LiquidCrystal_I2C lcd(0x27, 16, 2); // Change to 0x3F if needed
Servo gasValveServo;
DHT dht(DHT_PIN, DHT22);

// ----- MQ2 Calibration -----
const float R0 = 15.36;
const float RL = 5.0;
const int thresholdPPM = 300;

bool gasShutOff = false;

void setup() {
  Serial.begin(115200);

  // ✅ Swapped SDA and SCL for ESP32 (SDA = GPIO 22, SCL = GPIO 21)
  Wire.begin(22, 21);
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("MQ-2 + DHT22 Init");
  delay(2000);
  lcd.clear();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  gasValveServo.attach(SERVO_PIN);
  gasValveServo.write(0); // Gas ON (default)

  dht.begin();
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  int sensorValue = analogRead(MQ2_PIN);
  float voltage = sensorValue * (3.3 / 4095.0); // For 12-bit ADC
  float sensorRs = RL * (3.3 - voltage) / voltage;
  float ratio = sensorRs / R0;

  // LPG curve coefficients for MQ2 sensor (approximate)
  float m = -0.47;
  float b = 0.38;
  float ppm = pow(10, (log10(ratio) - b) / m);

  // Apply basic numerical correction for temperature
  float correctedPPM = ppm;
  if (!isnan(temperature)) {
    correctedPPM = ppm * (1 + (temperature - 25.0) * 0.01);  // ±1%/°C correction
  }

  // Debug output
  Serial.print("Temp: "); Serial.print(temperature);
  Serial.print(" Hum: "); Serial.print(humidity);
  Serial.print(" Raw PPM: "); Serial.print(ppm);
  Serial.print(" Corrected PPM: "); Serial.println(correctedPPM);

  // LCD display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PPM: "); lcd.print((int)correctedPPM);
  lcd.setCursor(0, 1);
  lcd.print("T:"); lcd.print((int)temperature);
  lcd.print(" H:"); lcd.print((int)humidity);

  // Alert logic
  if (correctedPPM > thresholdPPM) {
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    lcd.setCursor(10, 0); lcd.print("ALERT");

    if (!gasShutOff) {
      gasValveServo.write(90);  // Close gas
      gasShutOff = true;
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    gasValveServo.write(0);    // Open gas
    gasShutOff = false;
  }
  
  delay(2000);
}