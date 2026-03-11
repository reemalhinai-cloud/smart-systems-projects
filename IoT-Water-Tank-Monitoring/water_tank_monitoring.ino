#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
// Define pins for the ultrasonic sensors to measure the level of water in the tank
const int trigPinRoof = 53; // Roof ultrasonic sensor
const int echoPinRoof = 51; // Roof ultrasonic sensor
const int trigPinGround = 49; // Ground ultrasonic sensor
const int echoPinGround = 47; // Ground ultrasonic sensor
// Define pins for water pump
const int waterPumpPin = 24; // Water pump control
// Define pins for water drop sensors and rain detection
const int waterSensorPinRoof = A1; // Roof water sensor
const int waterSensorPinGround = A2; // Ground water sensor
const int waterSensorPinRain = A3; // Rain water sensor
// Define pins for the buzzer and PIR sensor
const int buzzerPin = 41;
const int pirSensorPin = 45;
// Define pin for the flow sensor
const int flowSensorPin = 2;
// Define LCD properties
const int lcdColumns = 16;
const int lcdRows = 4;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
// Define LEDs for each sensor
const int ledUltrasonicRoof = 10; // LED for roof water level
const int ledUltrasonicGround = 9; // LED for ground water level
const int ledWaterLeakRoof = 8; // LED for roof tank leak
const int ledWaterLeakGround = 7; // LED for ground tank leak
const int ledRain = 6; // LED for rain detection
const int ledPIR = 5; // LED for PIR sensor
const int ledFlow = 4; // LED for flow sensor
// Flow sensor variables
volatile int flowCount = 0;
unsigned long oldTime = 0;
float calibrationFactor = 4.5;
float flowRate = 0.0;
float flowMilliLitres = 0;
unsigned long totalMilliLitres = 0;
// Set up the software serial communication
SoftwareSerial XSerial(14, 15); // TX, RX
void setup() {
// Initialize Serial
Serial.begin(9600);
XSerial.begin(9600);
// Initialize LCD
lcd.init();
lcd.backlight();
// Initialize pins
pinMode(buzzerPin, OUTPUT);
pinMode(trigPinRoof, OUTPUT);
pinMode(echoPinRoof, INPUT);
pinMode(trigPinGround, OUTPUT);
pinMode(echoPinGround, INPUT);
pinMode(waterPumpPin, OUTPUT);
pinMode(waterSensorPinRoof, INPUT);
pinMode(waterSensorPinGround, INPUT);
pinMode(waterSensorPinRain, INPUT);
pinMode(pirSensorPin, INPUT);
pinMode(flowSensorPin, INPUT);
attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, FALLING);
pinMode(ledUltrasonicRoof, OUTPUT);
pinMode(ledUltrasonicGround, OUTPUT);
pinMode(ledWaterLeakRoof, OUTPUT);
pinMode(ledWaterLeakGround, OUTPUT);
pinMode(ledRain, OUTPUT);
pinMode(ledPIR, OUTPUT);
pinMode(ledFlow, OUTPUT);
}
void loop() {
handleUltrasonicSensors();
handleWaterSensors();
handleRainDetection();
handlePIRSensor();
handleFlowSensor();
handleWaterPump();
// Create JSON data with sensor readings
StaticJsonDocument<256> doc;
JsonObject root = doc.to<JsonObject>();
// Adding sensor data to the JSON object
root["distanceRoof"] = getDistance(trigPinRoof, echoPinRoof);
root["distanceGround"] = getDistance(trigPinGround, echoPinGround);
root["waterRoof"] = analogRead(waterSensorPinRoof);
root["waterGround"] = analogRead(waterSensorPinGround);
root["rain"] = analogRead(waterSensorPinRain);
root["flowRate"] = flowRate;
root["flowTotal"] = totalMilliLitres;
// Send the JSON data over serial
serializeJson(doc, XSerial);
// Check and send alerts for specific events
sendAlerts();
delay(5000); // Delay to reduce the frequency of data transmission
}
void sendAlerts() {
StaticJsonDocument<200> alertDoc;
// Check for water leak in the ground tank
int waterGround = analogRead(waterSensorPinGround);
if (waterGround >= 50) {
alertDoc["alert"] = "Ground Tank Leak Detected";
serializeJson(alertDoc, XSerial);
}
// Check for water leak in the roof tank
int waterRoof = analogRead(waterSensorPinRoof);
if (waterRoof >= 50) {
alertDoc["alert"] = "Roof Tank Leak Detected";
serializeJson(alertDoc, XSerial);
}
// Check for motion using PIR sensor
int pirState = digitalRead(pirSensorPin);
if (pirState == HIGH) {
alertDoc["alert"] = "Motion Detected";
serializeJson(alertDoc, XSerial);
}
}
void handleUltrasonicSensors() {
float distanceRoof = getDistance(trigPinRoof, echoPinRoof);
float distanceGround = getDistance(trigPinGround, echoPinGround);
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("RT: ");
lcd.print(map(distanceRoof, 1, 9, 100, 0));
lcd.print("%");
lcd.setCursor(0, 1);
lcd.print("GT: ");
lcd.print(map(distanceGround, 1, 9, 100, 0));
lcd.print("%");
Serial.println("RT LEVEL:");
Serial.println(map(distanceRoof, 1, 9, 100, 0));
Serial.println("GT LEVEL:");
Serial.println(map(distanceGround, 1, 9, 100, 0));
digitalWrite(ledUltrasonicRoof, distanceRoof < 1 ? LOW : HIGH);
digitalWrite(ledUltrasonicGround, distanceGround < 1 ? LOW : HIGH);
}
void handleWaterSensors() {
int waterRoof = analogRead(waterSensorPinRoof);
int waterGround = analogRead(waterSensorPinGround);
digitalWrite(ledWaterLeakRoof, waterRoof < 50 ? LOW : HIGH);
digitalWrite(ledWaterLeakGround, waterGround < 50 ? LOW : HIGH);
if (waterRoof >= 50) {
lcd.setCursor(0, 2);
lcd.print("RT Leak");
Serial.println("RT Leak");
}
if (waterGround >= 50) {
lcd.setCursor(0, 3);
lcd.print("GT Leak");
Serial.println("GT Leak");
}
}
void handleRainDetection() {
int rain = analogRead(waterSensorPinRain);
digitalWrite(ledRain, rain < 50 ? LOW : HIGH);
if (rain >= 50) {
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("Rain Detected");
Serial.println("Rain Detected");
}
}
void handlePIRSensor() {
int pirState = digitalRead(pirSensorPin);
digitalWrite(ledPIR, pirState);
Serial.println("motion detection");
}
void handleFlowSensor() {
if ((millis() - oldTime) > 1000) {
detachInterrupt(digitalPinToInterrupt(flowSensorPin));
// Calculate the flow rate
flowRate = ((1000.0 / (millis() - oldTime)) * flowCount) / calibrationFactor;
// Update total volume
totalMilliLitres += flowRate * (1000.0 / 60.0);
// Check and handle flow state
if (flowRate > 0) {
digitalWrite(ledFlow, LOW); // Turn on flow LED
Serial.println("Flow detected");
} else {
digitalWrite(ledFlow, HIGH); // Turn off flow LED
lcd.setCursor(0, 1);
lcd.print("No flow detected");
Serial.println("No flow detected");
}
// Reset variables for the next cycle
flowCount = 0;
oldTime = millis();
// Re-enable interrupts
attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, FALLING);
}
}
void handleWaterPump() {
float distanceRoof = getDistance(trigPinRoof, echoPinRoof);
int waterDropStatus = analogRead(waterSensorPinRoof);
// Determine the pump state
bool pumpState = (distanceRoof > 1 && waterDropStatus < 50);
// Set the pump state
digitalWrite(waterPumpPin, pumpState ? HIGH : LOW);
// Display the appropriate message
lcd.setCursor(0, 2);
if (pumpState) {
lcd.print("Filling roof tank ");
Serial.println("Filling roof tank");
} else {
lcd.print("Roof tank is full ");
Serial.println("Roof tank is full");
}
}
void pulseCounter() {
flowCount++;
}
float getDistance(int trigPin, int echoPin) {
digitalWrite(trigPin, LOW);
delayMicroseconds(5);
digitalWrite(trigPin, HIGH);
delayMicroseconds(10);
digitalWrite(trigPin, LOW);
long duration = pulseIn(echoPin, HIGH);
return duration * 0.034 / 2.0;
}

