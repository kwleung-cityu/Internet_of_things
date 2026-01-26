/********************************************************************************
 * File: 7_Run_WaterPump_with_Millis_ESP32.ino
 * 
 * Description:
 * This sketch is an advanced version of the Smart Flowerpot project, specifically
 * upgraded to run on the Freenove ESP32-S3 WROOM board. It builds upon the
 * previous non-blocking `millis()`-based timing system and integrates Wi-Fi
 * connectivity for IoT data logging to ThingSpeak.
 *
 * Key Improvements with ESP32-S3:
 * - Simplified Codebase: The ESP32-S3 has a built-in Wi-Fi module, which
 *   eliminates the need for a separate ESP8266 and the complex serial
 *   communication between two microcontrollers. This results in cleaner and
 *   more straightforward code.
 * - Simplified hardware connection because there is no ESP8266 required.
 * - Redundant Variables Removed: Global variables like `isWifiModuleOK` are no
 *   longer necessary. The Wi-Fi status can be checked directly using the
 *   `WiFi.status()` function from the native WiFi library.
 *
 * Core Functionality:
 * 1. Non-Blocking Operation: Uses `millis()` for all timing, ensuring the main
 *    loop runs quickly and responsively.
 * 2. Moisture Sensing: Periodically reads the soil moisture level.
 * 3. ThingSpeak Integration: Uploads moisture data to a ThingSpeak channel at
 *    a regular interval.
 * 4. Automated Watering Cycle: Implements a state machine (`IDLE`, `WATERING`,
 *    `SOAKING`) to control the water pump.
 *    - It uses hysteresis (upper and lower moisture thresholds) to prevent
 *      rapid on/off cycling of the pump.
 *    - When moisture drops below the lower threshold, it runs the pump for a
 *      defined period (`pumpOnTime`) and then waits for a `pumpSoakTime`
 *      before returning to idle, allowing the water to be absorbed.
 * 5. Status Indicator: A dual-color LED provides visual feedback on the Wi-Fi
 *    connection status (Red for disconnected, Blue for connected).
 ********************************************************************************/

// --- Libraries ---
#include <WiFi.h>
#include "ThingSpeak.h"

// --- Hardware Pin Definitions ---
#define SENSOR_PIN        1     //ESP32-S3 GPIO 1 (ADC1_CH0) for the moisture sensor
#define PUMP_RELAY_PIN    38    //ESP32-S3 GPIO 38 to control the water pump relay  
#define LED_RED_PIN       41
#define LED_BLUE_PIN      42

// --- Sensor Calibration ---
// Replace these with the values you found in the calibration step
const int DRY_VALUE = 4095; // Raw ADC value for 0% moisture (in air)
const int WET_VALUE = 1300; // Raw ADC value for 100% moisture (in water)

// --- Wi-Fi & ThingSpeak Configuration ---
#define SERIAL_MON_BAUDRATE 115200

char ssid[] = "YOUR_SSID";     // Your network SSID (name)
char pass[] = "YOUR_PASSWORD"; // Your network password

const unsigned long myChannelNumber = 123456;        // Your ThingSpeak channel number
const char *myWriteAPIKey = "channel_write_apikey";  // Your ThingSpeak Write API Key
const unsigned int moistureFieldNumber = 1;          // Field number for moisture data

// --- Global Variables ---
WiFiClient thingspeakClient;
int currentMoisturePercent = 100; // Holds the latest sensor reading.
                                  // initialize to 100% to avoid unintended water pump action on power up.

// --- Timing Control (Non-Blocking) ---
// Used to track time for various tasks without using delay()
unsigned long previousSensorReadMillis = 0;
unsigned long previousThingSpeakUploadMillis = 0;
unsigned long previousLedBlinkyMillis = 0;

// Set the intervals for how often tasks should run (in milliseconds)
const long sensorReadInterval = 5000;       // Read sensor every 5 seconds
const long thingSpeakUploadInterval = 60000; // Upload to ThingSpeak every 60 seconds
const long ledBlinkyInterval = 1000;  //led blinks in 1 second, with "red led => no wifi", "blue led => good wifi"

// Pump turn-on time and soak time - need tuning for your own case
const unsigned int pumpOnTime = 1000; // Pump ON time in milliseconds (1 second)
const unsigned int pumpSoakTime = 20000; // Soak time in milliseconds (20 seconds)
// Hysteresis upper and lower moisture threshold values - also need tuning for your own case
const unsigned int upperMoistureThreshold = 35; // Upper threshold in %
const unsigned int lowerMoistureThreshold = 30; // Lower threshold in %

// --- Function Prototypes ---
void connectWiFi();
void readMoisture();
void uploadToThingSpeak();
void controlWaterPump();
void ledBlinky();

// --- Add these new global variables ---
enum PumpState { IDLE, WATERING, SOAKING };
PumpState currentPumpState = IDLE;
unsigned long pumpStateChangeMillis = 0; // Tracks time for the current state

// --- The new function to START the cycle ---
void startWaterPumpCycle();
// --- The function that MANAGES the cycle (called from the main loop) ---
void manageWaterPumpCycle(unsigned int onTime, unsigned int soakTime);

// ==============================================================================
// SETUP: Runs once when the Arduino starts up
// ==============================================================================
void setup() {
  Serial.begin(SERIAL_MON_BAUDRATE);
  delay(500); //a short delay to let Serial port settle
    
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, LOW); // Ensure pump is OFF by default
  digitalWrite(LED_RED_PIN, LOW); //turn off RED and BLUE LEDs to start with
  digitalWrite(LED_BLUE_PIN, LOW);

  connectWiFi();

  ThingSpeak.begin(thingspeakClient); // Initialize ThingSpeak client
}

// ==============================================================================
// LOOP: Runs continuously and is kept non-blocking
// ==============================================================================
void loop() {
  // Get the current time at the start of the loop
  unsigned long currentMillis = millis();

  // Task 1: Read the moisture sensor at its specified interval
  if (currentMillis - previousSensorReadMillis >= sensorReadInterval) {
    previousSensorReadMillis = currentMillis; // Save the time of this reading
    readMoisture();
  }

  // Task 2: Attempt to upload data to ThingSpeak at its specified interval
  if (currentMillis - previousThingSpeakUploadMillis >= thingSpeakUploadInterval) {
    previousThingSpeakUploadMillis = currentMillis; // Save the time of this attempt
    
    // Only proceed if the WiFi module was detected at startup
    // If not connected, try to connect now.
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi(); 
    }
    
    // After the connection attempt, check again before uploading.
    if (WiFi.status() == WL_CONNECTED) {
      uploadToThingSpeak();
    }
  }

  // Task 3: Control the water pump (runs on every loop)
  controlWaterPump();

  // Task 4: Blink the LED to give a visual signal
  if(currentMillis - previousLedBlinkyMillis >= ledBlinkyInterval){
    previousLedBlinkyMillis = currentMillis;
    ledBlinky();
  }
}

// ==============================================================================
// --- Helper Functions ---
// ==============================================================================

/**
 * @brief Attempts to connect to the Wi-Fi network.
 * NOTE: WiFi.begin() is a BLOCKING call and can take several seconds to
 * execute. During this time, the main loop will be paused.
 */
void connectWiFi() {
  Serial.println("Attempting to connect to WiFi (this may block for a few seconds)...");
  WiFi.begin(ssid, pass);
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
 * @brief Placeholder function for water pump control logic.
 */
void controlWaterPump() {
  // This is where you would add the logic to control the pump.
  if(currentMoisturePercent < lowerMoistureThreshold){
    //Serial.println("Moisture below lower threshold. Starting watering cycle.");
    startWaterPumpCycle();
  } else if(currentMoisturePercent > upperMoistureThreshold){
    //Serial.println("Moisture above upper threshold. Too much water, need intervention.");
  } else {
    // do nothing, within hysteresis band
  } 

  // Manage the pump state on every single loop iteration
  // The function will handle the timing and state changes internally.
  // Use 1000ms for watering and 20000ms (30s) for soaking.
  manageWaterPumpCycle(pumpOnTime, pumpSoakTime);  
}

/**
 * @brief Blinks the LED to indicate WiFi status.
 * Red LED indicates no WiFi connection. Blue LED indicates good WiFi connection.
 */
void ledBlinky() {
  if(WiFi.status() == WL_CONNECTED){
    // Blink Blue LED
    digitalWrite(LED_RED_PIN, LOW); // Ensure RED is OFF
    digitalWrite(LED_BLUE_PIN, !digitalRead(LED_BLUE_PIN)); // Toggle BLUE LED
  } else {
    // Blink Red LED
    digitalWrite(LED_BLUE_PIN, LOW); // Ensure BLUE is OFF
    digitalWrite(LED_RED_PIN, !digitalRead(LED_RED_PIN)); // Toggle RED LED
  }
}

// --- The new function to START the cycle ---
void startWaterPumpCycle() {
  // Only start a new cycle if the pump is currently idle
  if (currentPumpState == IDLE) {
    currentPumpState = WATERING;
    pumpStateChangeMillis = millis(); // Record the time we started watering
    digitalWrite(PUMP_RELAY_PIN, HIGH);
    Serial.println("Pump cycle started: WATERING");
  }
}

/**
 * @brief Manages the water pump cycle using a state machine.
 * @param onTime Duration for which the pump should be ON (in milliseconds).
 * @param soakTime Duration for which the soil should soak after watering (in milliseconds).
 */
void manageWaterPumpCycle(unsigned int onTime, unsigned int soakTime) {
  // This function is a state machine. It does nothing unless a state is active.
  
  // State 1: The pump is currently WATERING
  if (currentPumpState == WATERING) {
    // Check if the 'onTime' has elapsed
    if (millis() - pumpStateChangeMillis >= onTime) {
      // Time to switch to the SOAKING state
      currentPumpState = SOAKING;
      pumpStateChangeMillis = millis(); // Record the time we started soaking
      digitalWrite(PUMP_RELAY_PIN, LOW);
      Serial.println("Watering finished. Now SOAKING.");
    }
  }  // State 2: The soil is currently SOAKING
  else if (currentPumpState == SOAKING) { 
    // Check if the 'soakTime' has elapsed
    if (millis() - pumpStateChangeMillis >= soakTime) {
      // The cycle is complete, return to IDLE
      currentPumpState = IDLE;
      Serial.println("Soak time complete. Pump cycle finished.");
    }
  }
}