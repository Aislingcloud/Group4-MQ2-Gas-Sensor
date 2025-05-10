
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

const int mq2Pin = A0;
const int buzzerPin = 8;
const int ledPin = 7;
const int servoPin = 9;

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo gasValveServo;

const float R0 = 15.36;
const float RL = 5.0;
const int thresholdPPM = 300;

bool gasShutOff = false;

void setup() {
Serial.begin(9600);
pinMode(buzzerPin, OUTPUT);
pinMode(ledPin, OUTPUT);
digitalWrite(buzzerPin, LOW);
digitalWrite(ledPin, LOW);

lcd.begin(16, 2);
lcd.backlight();
lcd.setCursor(0, 0);
lcd.print("MQ-2 Sensor Init");
delay(2000);
lcd.clear();

gasValveServo.attach(servoPin);
gasValveServo.write(0); // Initial position (gas ON)
}

void loop() {
int sensorValue = analogRead(mq2Pin);
float voltage = sensorValue * (5.0 / 1023.0);
float sensorRs = RL * (5.0 - voltage) / voltage;
float ratio = sensorRs / R0;

float m = -0.47;
float b = 0.38;
float ppm = pow(10, (log10(ratio) - b) / m);

// Serial output
Serial.print("Sensor Value: ");
Serial.print(sensorValue);
Serial.print(" Rs: ");
Serial.print(sensorRs, 2);
Serial.print(" kOhm Ratio: ");
Serial.print(ratio, 2);
Serial.print(" Gas Concentration (LPG): ");
Serial.print(ppm, 0);
Serial.println(" ppm");

// LCD output
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("LPG: ");
lcd.print((int)ppm);
lcd.print(" ppm");

lcd.setCursor(0, 1);
if (ppm > thresholdPPM) {
lcd.print("ALERT! Gas High");

digitalWrite(buzzerPin, HIGH);
digitalWrite(ledPin, HIGH);

if (!gasShutOff) {
gasValveServo.write(90); // Rotate to turn off gas
gasShutOff = true;
}
} else {
lcd.print("Status: Normal");

digitalWrite(buzzerPin, LOW);
digitalWrite(ledPin, LOW);

gasValveServo.write(0); // Return to open gas
gasShutOff = false;
}

delay(1000);
}