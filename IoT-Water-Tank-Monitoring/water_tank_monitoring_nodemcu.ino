#define BLYNK_AUTH_TOKEN "u2tJI8SgHW76-vLobl9DaPz4vMcxxYdH"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
// Network and Blynk credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "BtechPro";
char pass[] = "r123456789";
BlynkTimer timer;
// Sensor data variables
float roofWaterLeakSensor = 0, groundWaterLeakSensor = 0;
int motionSensorState = LOW;
int electricValveControl = 0; // Electric valve state
unsigned long lastDataReceivedTime = 0; // Track last serial data time
// Pin assignments
const int electricValvePin = 1; // Electric valve control pin (NodeMCU)
// Timeout settings
const unsigned long serialTimeout = 5000; // 5 seconds timeout for Serial data
void setup() {
Serial.begin(9600);
while (!Serial) { continue; }
// Initialize Blynk
Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
// Set up a timer to send data to Blynk every second
timer.setInterval(1000L, myTimerEvent);
// Set electric valve pin mode (NodeMCU)
pinMode(electricValvePin, OUTPUT);
// Initialize electric valve to safe state
digitalWrite(electricValvePin, LOW);
Serial.println("System Initialized. Valve Closed by default.");
}
void myTimerEvent() {
Serial.println("\nTimer started");
// Send sensor data to Blynk
Blynk.virtualWrite(V1, roofWaterLeakSensor); // Roof Water Leak sensor
Blynk.virtualWrite(V2, groundWaterLeakSensor); // Ground Water Leak sensor
Blynk.virtualWrite(V4, electricValveControl); // Electric valve state
Blynk.virtualWrite(V3, motionSensorState); // Motion sensor state
// Log for motion detection
if (motionSensorState == HIGH) {
Blynk.logEvent("motion_detected", "Motion Detected!");
Serial.println("Motion Detected!");
}
// Log for leaks
if (roofWaterLeakSensor >= 50) {
Blynk.logEvent("roof_leak", "Roof Tank Leak Detected!");
Serial.println("Roof Tank Leak Detected!");
}
if (groundWaterLeakSensor >= 50) {
Blynk.logEvent("ground_leak", "Ground Tank Leak Detected!");
Serial.println("Ground Tank Leak Detected!");
}
// Reflect electric valve state based on sensor readings
if (roofWaterLeakSensor < 50) {
electricValveControl = 1; // Open valve if no leak detected
Serial.println("Valve Open");
} else {
electricValveControl = 0; // Close valve if leak detected
Serial.println("Valve Closed");
}
digitalWrite(electricValvePin, electricValveControl ? HIGH : LOW);
// Check for communication timeout
if (millis() - lastDataReceivedTime > serialTimeout) {
electricValveControl = 0; // Close valve for safety
digitalWrite(electricValvePin, LOW);
Serial.println("Timeout! No data received. Valve Closed.");
}
}
void loop() {
// Check for JSON data over Serial (from Arduino Mega)
if (Serial.available() > 0) {
DynamicJsonDocument doc(512);
DeserializationError error = deserializeJson(doc, Serial);
if (error) {
Serial.println("Failed to parse JSON");
Serial.println(error.c_str());
return;
}
// Extract sensor data from the JSON object
roofWaterLeakSensor = doc["waterRoof"];
groundWaterLeakSensor = doc["waterGround"];
motionSensorState = doc["motionSensor"];
lastDataReceivedTime = millis(); // Update the last data received time
}
// Run Blynk and timer tasks
Blynk.run();
Serial.println("\ngoing for blynk");
timer.run();
}