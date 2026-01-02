/********************************************************************************
 * File: 4_ThingSpeak_Upload_Moisture_v2.ino
 * 
 * Description:
 * This sketch is an improved, non-blocking version for reading a soil moisture
 * sensor and uploading the data to ThingSpeak. It avoids using delay() by
 * tracking time with the millis() function. This allows the program to remain
 * responsive and handle multiple tasks concurrently, such as checking sensor
 * values, managing a Wi-Fi connection, and (in the future) controlling a 
 * water pump.
 * 
 * Key Improvements from v1:
 * 1. Non-Blocking Timing: Uses millis() instead of delay() to manage sensor 
 *    reading and data upload intervals.
 * 2. Modular Functions: Code is broken into smaller, single-purpose functions
 *    (e.g., readMoisture(), handleWiFi(), uploadToThingSpeak()).
 * 3. Responsive Loop: The main loop() runs continuously, allowing for near
 *    real-time control and responsiveness.
 * 4. Scalability: This structure makes it easy to add new features, like a
 *    water pump, without rewriting the core logic.
 * 
 * Hardware Connections (as per 1_Hardware_Setup.md):
 * - Moisture Sensor AO -> Arduino A0
 * - ESP8266 TX/RX     -> Arduino RX1/TX1 (Serial1)
 * - Water Pump Relay  -> Arduino GPIO2
 * 
 ********************************************************************************/

// --- Libraries ---
#include "ThingSpeak.h"
#include "WiFiEsp.h"

// --- Hardware Pin Definitions ---
#define SENSOR_PIN A0 
#define PUMP_RELAY_PIN 2

// --- Sensor Calibration ---
// Replace these with the values you found in the calibration step
const int DRY_VALUE = 680; // Raw ADC value for 0% moisture (in air)
const int WET_VALUE = 250; // Raw ADC value for 100% moisture (in water)

// --- Wi-Fi & ThingSpeak Configuration ---
#define SERIAL_MON_BAUDRATE 115200
#define ESP_BAUDRATE 115200
char ssid[] = "YOUR_SSID";     // Your network SSID (name)
char pass[] = "YOUR_PASSWORD"; // Your network password

const unsigned long myChannelNumber = 123456;        // Your ThingSpeak channel number
const char *myWriteAPIKey = "channel_write_apikey";  // Your ThingSpeak Write API Key
const unsigned int moistureFieldNumber = 1;          // Field number for moisture data

// --- Global Variables ---
WiFiEspClient thingspeakClient;
int currentMoisturePercent = 0; // Holds the latest sensor reading

// --- Timing Control (Non-Blocking) ---
// Used to track time for various tasks without using delay()
unsigned long previousSensorReadMillis = 0;
unsigned long previousThingSpeakUploadMillis = 0;

// Set the intervals for how often tasks should run (in milliseconds)
const long sensorReadInterval = 5000;       // Read sensor every 5 seconds
const long thingSpeakUploadInterval = 60000; // Upload to ThingSpeak every 60 seconds

// --- Function Prototypes ---
void connectWiFi();
void readMoisture();
void uploadToThingSpeak();
void controlWaterPump();

// ==============================================================================
// SETUP: Runs once when the Arduino starts up
// ==============================================================================
void setup() {

  Serial.begin(115200);
  Serial1.begin(ESP_BAUDRATE); // Initialize Serial1 for ESP8266 modem

  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, LOW); // Ensure pump is OFF by default

  connectWiFi(); // Initial attempt to connect to Wi-Fi

  ThingSpeak.begin(thingspeakClient); // Initialize ThingSpeak client
}

// ==============================================================================
// LOOP: Runs continuously and is kept non-blocking
// ==============================================================================
void loop() {
  // Get the current time at the start of the loop
  unsigned long currentMillis = millis();

  // Task 1: Maintain Wi-Fi Connection (runs on every loop)
  // If Wi-Fi disconnects, this will attempt to reconnect.
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // Task 2: Read the moisture sensor at its specified interval
  if (currentMillis - previousSensorReadMillis >= sensorReadInterval) {
    previousSensorReadMillis = currentMillis; // Save the time of this reading
    readMoisture();
  }

  // Task 3: Upload data to ThingSpeak at its specified interval
  if (currentMillis - previousThingSpeakUploadMillis >= thingSpeakUploadInterval) {
    previousThingSpeakUploadMillis = currentMillis; // Save the time of this upload
    uploadToThingSpeak();
  }

  // Task 4: Control the water pump (runs on every loop)
  // This function will contain the logic to turn the pump on/off based on moisture
  controlWaterPump();
}

// ==============================================================================
// --- Helper Functions ---
// ==============================================================================

/**
 * @brief Connects to the Wi-Fi network.
 * It will block until a connection is established.
 */
void connectWiFi() {
  WiFi.init(&Serial1);
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("ERROR: WiFi shield not present");
    while (true); // Halt execution
  }

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    Serial.print(".");
    delay(2000); // Use a short delay ONLY for connection attempts
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

/**
 * @brief Reads the moisture sensor and updates the global variable.
 */
void readMoisture() {
  int rawValue = analogRead(SENSOR_PIN);
  
  // Map the raw value to a percentage
  currentMoisturePercent = map(rawValue, DRY_VALUE, WET_VALUE, 0, 100);
  
  // Constrain the value to the 0-100 range to prevent invalid readings
  currentMoisturePercent = constrain(currentMoisturePercent, 0, 100);

  Serial.print("Sensor Reading -> Raw: ");
  Serial.print(rawValue);
  Serial.print(", Moisture: ");
  Serial.print(currentMoisturePercent);
  Serial.println("%");
}

/**
 * @brief Uploads the current moisture percentage to ThingSpeak.
 */
void uploadToThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot upload data: WiFi not connected.");
    return;
  }

  Serial.println("Uploading data to ThingSpeak...");
  ThingSpeak.setField(moistureFieldNumber, currentMoisturePercent);

  int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if (httpCode == 200) {
    Serial.println("ThingSpeak upload successful.");
  } else {
    Serial.println("Problem uploading to ThingSpeak. HTTP error code " + String(httpCode));
    // Note: No delay() or while() loop here. If it fails, it will simply
    // try again on the next scheduled interval.
  }
}

/**
 * @brief Placeholder function for water pump control logic.
 * This function can now be called on every loop without being blocked.
 */
void controlWaterPump() {
  // This is where you would add the logic to control the pump.
  // For example:
  // if (currentMoisturePercent < 30) {
  //   digitalWrite(PUMP_RELAY_PIN, HIGH); // Turn pump ON
  // } else {
  //   digitalWrite(PUMP_RELAY_PIN, LOW);  // Turn pump OFF
  // }
}
