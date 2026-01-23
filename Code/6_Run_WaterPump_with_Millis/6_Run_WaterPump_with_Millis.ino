/********************************************************************************
 * File: 6_Run_WaterPump_with_Millis.ino
 * 
 * Description:
 * This sketch represents the complete, non-blocking IoT code for a fully
 * automated plant watering system. It builds upon the responsive foundation of
 * the 'v2' sketch and integrates the final component: the water pump.
 * 
 * The core of this sketch is the non-blocking, millis()-based state machine
 * that controls the water pump. This allows the system to run a complete
 * watering and soaking cycle without ever using a delay() function, ensuring
 * the main loop remains responsive at all times.
 * 
 * Key Features:
 * 1. Automated Pump Control: Implements a state machine with IDLE, WATERING,
 *    and SOAKING states to manage the pump cycle.
 * 2. Hysteresis Logic: Uses a lower and upper moisture threshold to prevent
 *    the pump from rapidly turning on and off ("chattering"). The pump only
 *    activates when the soil is sufficiently dry.
 * 3. Fully Non-Blocking: All tasks—sensor reading, ThingSpeak uploads, LED
 *    status, and pump control—are managed with millis(), allowing for
 *    concurrent operation.
 * 4. Configurable Timings: Pump 'on time', 'soak time', and hysteresis
 *    thresholds are defined as constants for easy tuning.
 ********************************************************************************/

// --- Libraries ---
#include "ThingSpeak.h"
#include "WiFiEsp.h"

// --- Hardware Pin Definitions ---
#define SENSOR_PIN A0 
#define PUMP_RELAY_PIN  2
#define LED_RED_PIN     3
#define LED_BLUE_PIN    4

// --- Sensor Calibration ---
// Replace these with the values you found in the calibration step
const int DRY_VALUE = 680; // Raw ADC value for 0% moisture (in air)
const int WET_VALUE = 250; // Raw ADC value for 100% moisture (in water)

// --- Wi-Fi & ThingSpeak Configuration ---
#define ESP_BAUDRATE 115200
#define SERIAL_MON_BAUDRATE 115200
char ssid[] = "YOUR_SSID";     // Your network SSID (name)
char pass[] = "YOUR_PASSWORD"; // Your network password

const unsigned long myChannelNumber = 123456;        // Your ThingSpeak channel number
const char *myWriteAPIKey = "channel_write_apikey";  // Your ThingSpeak Write API Key
const unsigned int moistureFieldNumber = 1;          // Field number for moisture data

// --- Global Variables ---
WiFiEspClient thingspeakClient;
int currentMoisturePercent = 100; // Holds the latest sensor reading
                                  // initialize to 100% to avoid unintended water pump action on power up.
bool isWifiModuleOK = false;      // Flag to track if the ESP8266 is responding

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
  Serial1.begin(ESP_BAUDRATE); // Initialize Serial1 for ESP8266 modem
  delay(500); //a short delay to let Serial port settle
    
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, LOW); // Ensure pump is OFF by default
  digitalWrite(LED_RED_PIN, LOW); //turn off RED and BLUE LEDs to start with
  digitalWrite(LED_BLUE_PIN, LOW);

  WiFi.init(&Serial1);
  
  // Check if the WiFi module is responding.
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("******************************************************");
    Serial.println("ERROR: ESP8266 WiFi module not detected or not responding.");
    Serial.println(" - Check wiring between Mega and ESP8266.");
    Serial.println(" - Ensure ESP8266 has sufficient power.");
    Serial.println(" - The program will continue to run without WiFi.");
    Serial.println("******************************************************");
    isWifiModuleOK = false; // Set flag to prevent further WiFi attempts
  } else {
    Serial.println("WiFi module detected.");
    isWifiModuleOK = true; // WiFi module is OK
  }

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
    if (isWifiModuleOK) {
      // If not connected, try to connect now.
      if (WiFi.status() != WL_CONNECTED) {
        connectWiFi(); 
      }
      
      // After the connection attempt, check again before uploading.
      if (WiFi.status() == WL_CONNECTED) {
        uploadToThingSpeak();
      }
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
  if(isWifiModuleOK && WiFi.status() == WL_CONNECTED){
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