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

// --- enable/disable ThingSpeak at compile time ---
// uncomment to enable ThingSpeak uploads  
// #define THINGSPEAK_ENABLE

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
bool manualMoistureOverride = false; // NEW: Flag to enable manual override

// --- Pump Control ---
const int moistureThreshold = 20;     // Turn pump on if moisture is below this %
const int maxPumpOnTime = 5000;       // Max pump on-time in ms (for 0% moisture)
const int minPumpOnTime = 1000;       // Min pump on-time in ms (for just below threshold)
bool pumpIsOn = false;                // Tracks the pump's current state
unsigned long pumpStartTime = 0;      // Tracks when the pump was turned on
unsigned long pumpOnTimeMillis = 0;   // Holds the calculated pump on-time

// --- Timing Control (Non-Blocking) ---
// Used to track time for various tasks without using delay()
unsigned long previousSensorReadMillis = 0;
unsigned long previousThingSpeakUploadMillis = 0;
unsigned long previousWaterPumpCheckMillis = 0;
unsigned long previousWifiConnectMillis = 0;

// Set the intervals for how often tasks should run (in milliseconds)
const long sensorReadInterval = 5000;       // Read sensor every 5 seconds
const long thingSpeakUploadInterval = 60000; // Upload to ThingSpeak every 60 seconds
const long waterPumpCheckInterval = 500;   // Check water pump control every 0.5 second
const long wifiConnectInterval = 10000;     // Attempt to reconnect to Wi-Fi every 10 seconds

// --- Function Prototypes ---
void connectWiFi();
void readMoisture();
void uploadToThingSpeak();
void controlWaterPump();
void handleSerialInput(); // NEW: Function to handle serial commands

// ==============================================================================
// SETUP: Runs once when the Arduino starts up
// ==============================================================================
void setup() {

  Serial.begin(SERIAL_MON_BAUDRATE);
  delay(500); // Give some time for Serial to initialize

#if defined(THINGSPEAK_ENABLE)
  Serial1.begin(ESP_BAUDRATE); // Initialize Serial1 for ESP8266 modem
  connectWiFi(); // Initial attempt to connect to Wi-Fi
  ThingSpeak.begin(thingspeakClient); // Initialize ThingSpeak client
#endif

  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, LOW); // Ensure pump is OFF by default

  // NEW: Instructions for manual override
  Serial.println("\n--- System Ready ---");
  Serial.println("Enter a number (0-100) to manually set moisture %.");
  Serial.println("Enter 'auto' to return to sensor-based reading.");
  Serial.println("--------------------");
}

// ==============================================================================
// LOOP: Runs continuously and is kept non-blocking
// ==============================================================================
void loop() {
  // Get the current time at the start of the loop
  unsigned long currentMillis = millis();

  // NEW: Task 1: Check for serial input to update settings
  handleSerialInput();

  // Task 2: Read the moisture sensor at its specified interval
  if (currentMillis - previousSensorReadMillis >= sensorReadInterval) {
    previousSensorReadMillis = currentMillis; // Save the time of this reading
    readMoisture();
  }

  // Task 3: Upload data to ThingSpeak at its specified interval 
  #if defined(THINGSPEAK_ENABLE)
  if (currentMillis - previousThingSpeakUploadMillis >= thingSpeakUploadInterval) {
    previousThingSpeakUploadMillis = currentMillis; // Save the time of this upload
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
    }
    uploadToThingSpeak();
  }
  #endif

  // Task 4: Control the water pump (runs on every loop)
  if (currentMillis - previousWaterPumpCheckMillis >= waterPumpCheckInterval) {
    previousWaterPumpCheckMillis = currentMillis; // Save the time of this check
    controlWaterPump();
  }
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
 * MODIFIED: Skips reading if manual override is active.
 */
void readMoisture() {
  // Only read from the sensor if we are in automatic mode
  if (!manualMoistureOverride) {
    int rawValue = analogRead(SENSOR_PIN);
    currentMoisturePercent = map(rawValue, DRY_VALUE, WET_VALUE, 0, 100);
    currentMoisturePercent = constrain(currentMoisturePercent, 0, 100);

    Serial.print("Sensor Reading -> Raw: ");
    Serial.print(rawValue);
    Serial.print(", Moisture: ");
    Serial.print(currentMoisturePercent);
    Serial.println("% (Auto)");
  }
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
  }
}

/**
 * @brief Controls the water pump with dynamic duration based on moisture level.
 * This function is non-blocking.
 */
void controlWaterPump() {
  unsigned long currentMillis = millis();

  if (!pumpIsOn && currentMoisturePercent < moistureThreshold) {
    pumpOnTimeMillis = map(currentMoisturePercent, 0, moistureThreshold, maxPumpOnTime, minPumpOnTime);
    digitalWrite(PUMP_RELAY_PIN, HIGH);
    pumpIsOn = true;
    pumpStartTime = currentMillis;

    Serial.print("Soil is dry. Turning pump ON for ");
    Serial.print(pumpOnTimeMillis);
    Serial.println(" ms.");
  }

  if (pumpIsOn && (currentMillis - pumpStartTime >= pumpOnTimeMillis)) {
    digitalWrite(PUMP_RELAY_PIN, LOW);
    pumpIsOn = false;
    Serial.println("Pump OFF. Watering cycle complete.");
  }
}

/**
 * @brief NEW: Handles incoming serial data to update settings.
 * This function is non-blocking.
 */
void handleSerialInput() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("auto")) {
      manualMoistureOverride = false;
      Serial.println("Switched to AUTOMATIC mode. Using soil sensor.");
    } else {
      int manualValue = input.toInt();
      // Check if the input is a valid number and within range
      if (manualValue >= 0 && manualValue <= 100) {
        manualMoistureOverride = true;
        currentMoisturePercent = manualValue;
        Serial.print("MANUAL OVERRIDE: Moisture set to ");
        Serial.print(currentMoisturePercent);
        Serial.println("%.");
      } else {
        Serial.println("Invalid input. Enter a number 0-100 or 'auto'.");
      }
    }
  }
}