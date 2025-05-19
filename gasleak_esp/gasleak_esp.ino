#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

// ----- BLYNK SETUP -----
#define BLYNK_TEMPLATE_ID "TMPL63gfXiBWP"
#define BLYNK_TEMPLATE_NAME "Gas Leak Detector"
#define BLYNK_AUTH_TOKEN "Your Auth Token"

char ssid[] = "YourWiFiName";
char pass[] = "YourWiFiPassword";

// ----- PIN SETUP -----
const int mq2Pin = 34;       // Updated: GPIO34 for MQ2 analog input
const int buzzerPin = 14;
const int ledPin = 27;
const int servoPin = 18;
const int dhtPin = 4;        // DHT22 data pin

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo gasValveServo;
DHT dht(dhtPin, DHT22);

// ----- MQ2 Calibration -----
const float R0 = 15.36;
const float RL = 5.0;
const int thresholdPPM = 300;

bool gasShutOff = false;

void setup() {
  Serial.begin(115200);

  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, LOW);

  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("MQ-2 + DHT22 Init");
  delay(2000);
  lcd.clear();

  gasValveServo.attach(servoPin);
  gasValveServo.write(0); // gas ON

  dht.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  Blynk.run();

  // --- Sensor Readings ---
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  int sensorValue = analogRead(mq2Pin);
  float voltage = sensorValue * (3.3 / 4095.0); // 12-bit ADC
  float sensorRs = RL * (3.3 - voltage) / voltage;
  float ratio = sensorRs / R0;

  float m = -0.47;
  float b = 0.38;
  float ppm = pow(10, (log10(ratio) - b) / m);

  // --- Numerical Compensation ---
  float correctedPPM = ppm;
  if (!isnan(temp)) {
    correctedPPM = ppm * (1 + (temp - 25.0) * 0.01);  // ±1%/°C correction
  }

  // --- Debug Output ---
  Serial.print("Temp: "); Serial.print(temp);
  Serial.print(" Hum: "); Serial.print(hum);
  Serial.print(" Raw PPM: "); Serial.print(ppm);
  Serial.print(" Corrected PPM: "); Serial.println(correctedPPM);

  // --- LCD Output ---
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PPM: "); lcd.print((int)correctedPPM);
  lcd.setCursor(0, 1);
  lcd.print("T:"); lcd.print((int)temp); lcd.print(" H:"); lcd.print((int)hum);

  // --- Alert Logic ---
  if (correctedPPM > thresholdPPM) {
    digitalWrite(buzzerPin, HIGH);
    digitalWrite(ledPin, HIGH);
    lcd.setCursor(10, 0);
    lcd.print("ALERT");

    if (!gasShutOff) {
      gasValveServo.write(90); // close gas
      Blynk.logEvent("gas_alert", "High gas concentration!");
      gasShutOff = true;
    }
  } else {
    digitalWrite(buzzerPin, LOW);
    digitalWrite(ledPin, LOW);
    gasValveServo.write(0); // gas ON
    gasShutOff = false;
  }

  delay(2000);
}
