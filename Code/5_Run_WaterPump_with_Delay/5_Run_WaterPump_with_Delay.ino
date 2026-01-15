/********************************************************************************
 * File: 5_Run_WaterPump_with_Delay.ino
 * 
 * Description:
 * This sketch demonstrates how to integrate a water pump into the IoT system
 * using a simple, but flawed, `delay()`-based approach. It serves as an
 * educational example to highlight the problems with blocking code in a
 * responsive system.
 * 
 * While the non-blocking `millis()` structure is used for sensor reading and
 * ThingSpeak uploads, the water pump control itself is handled by a function
 * that uses long, blocking `delay()` calls.
 * 
 * Key Features & Learning Points:
 * 1. Blocking Pump Control: The `runWaterPumpCycle_with_delay()` function
 *    completely halts the microcontroller's execution.
 * 2. Hysteresis Logic: It correctly uses a lower and upper moisture threshold
 *    to decide when to start the watering cycle.
 * 3. The "Delay" Problem: When the pump cycle runs, you will observe that
 *    the status LEDs stop blinking and the Serial Monitor provides no updates.
 *    This is because the main `loop()` is frozen, demonstrating why `delay()`
 *    is unsuitable for responsive, multi-tasking projects.
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
int currentMoisturePercent = 0; // Holds the latest sensor reading
bool isWifiModuleOK = false;    // Flag to track if the ESP8266 is responding

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
void runWaterPumpCycle_with_delay(unsigned int onTime, unsigned int soakTime);
void ledBlinky();

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
    Serial.println("Moisture below lower threshold. Starting watering cycle.");
    runWaterPumpCycle_with_delay(pumpOnTime, pumpSoakTime);
  } else if(currentMoisturePercent > upperMoistureThreshold){
    Serial.println("Moisture above upper threshold. Too much water, need intervention.");
  } else {
    // do nothing, within hysteresis band
  } 
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

/**
 * * 
 * @brief Runs a water pump cycle with blocking delays.
 * @param onTime Duration (in milliseconds) to keep the pump ON.
 * @param soakTime Duration (in milliseconds) to wait after turning the pump OFF.
 * NOTE: This function uses blocking delay() calls, which will pause the main loop.
 */
void runWaterPumpCycle_with_delay(unsigned int onTime, unsigned int soakTime) {
  // 1. Turn the water pump ON
  digitalWrite(PUMP_RELAY_PIN, HIGH);
  Serial.println("Pump ON");

  // 2. Wait for the specified 'onTime' duration.
  //    !!! This is a BLOCKING call. The MCU can do nothing else. !!!
  delay(onTime);

  // 3. Turn the water pump OFF
  digitalWrite(PUMP_RELAY_PIN, LOW);
  Serial.println("Pump OFF");

  // 4. Wait for the 'soakTime' to allow water to absorb.
  //    !!! This is also a BLOCKING call. !!!
  Serial.println("Program trapped in delay(soakTime). You won't see LED blinking!");
  delay(soakTime);
  Serial.println("Soak time complete. Cycle finished.");
}