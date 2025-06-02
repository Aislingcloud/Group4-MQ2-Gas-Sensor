#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL63gfXiBWP"
#define BLYNK_TEMPLATE_NAME "Gas Leak Detector"
#define BLYNK_AUTH_TOKEN "IQqpFUMgeOu8u_sKFLjVIzcQPxIYGN7v"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <DHT.h>

// WiFi credentials
char ssid[] = "ZTE_2.4G";
char pass[] = "pangetkim!";

// Sensor and actuator pins
#define DHTPIN 14
#define DHTTYPE DHT22
#define MQ2_PIN 34
#define BUZZER_PIN 26
#define LED_PIN 25
#define SERVO_PIN 27
#define GAS_THRESHOLD 2000

// Libraries and objects
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo valveServo;

bool systemOn = true;

BLYNK_WRITE(V2) {  // On/Off switch
  systemOn = param.asInt();
}

BLYNK_WRITE(V3) {  // Manual LED control (optional)
  digitalWrite(LED_PIN, param.asInt());
}

void setup() {
  Serial.begin(9600);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  dht.begin();
  lcd.init();
  lcd.backlight();

  pinMode(MQ2_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  valveServo.attach(SERVO_PIN);
  valveServo.write(0);  // Gas valve open

  lcd.setCursor(0, 0);
  lcd.print("System Initializing");
  delay(2000);
  lcd.clear();
}

void loop() {
  Blynk.run();

  float gasPPM = (analogRead(MQ2_PIN) / 4095.0) * 10000;
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // Send values to Blynk
  Blynk.virtualWrite(V0, temp);     // Temperature
  Blynk.virtualWrite(V1, gasPPM);   // Gas
  Blynk.virtualWrite(V5, hum);      // Humidity

  // LCD display
  lcd.setCursor(0, 0);
  lcd.print("G:");
  lcd.print((int)gasPPM);
  lcd.print("ppm ");

  lcd.setCursor(0, 1);
  lcd.print("T:");
  lcd.print((int)temp);
  lcd.print("C H:");
  lcd.print((int)hum);
  lcd.print("%");

  if (systemOn) {
    if (gasPPM > GAS_THRESHOLD) {
      digitalWrite(BUZZER_PIN, HIGH);
      digitalWrite(LED_PIN, HIGH);
      valveServo.write(90);  // Close valve
      Blynk.logEvent("gas_leak_alert", "Gas Leak Detected!");
    } else {
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED_PIN, LOW);
      valveServo.write(0);   // Keep valve open
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    valveServo.write(0);
  }

  delay(1000);
}
