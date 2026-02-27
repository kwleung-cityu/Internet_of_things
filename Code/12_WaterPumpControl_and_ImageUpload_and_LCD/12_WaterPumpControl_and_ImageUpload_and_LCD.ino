/**
 * 12_WaterPumpControl_and_ImageUpload_and_LCD.ino
 *
 * Based on demo `11_WaterPumpControl_and_ImageDemoUpload.ino`, this sketch is enhanced with an LCD display (LovyanGFX/ST7789) to display the moisture value.
 *
 * High-Level Flow:
 * 1. Periodically read soil moisture sensor.
 * 2. Update LCD with latest moisture value.
 * 3. Capture image, upload to Google Drive, and get URL.
 * 4. Upload both moisture value and image URL to ThingSpeak in a single request.
 * 5. Control water pump using hysteresis and soak cycle.
 * 6. Blink LEDs to indicate WiFi status.
 *
 * How to use:
 * 1. Set up hardware according to pin definitions in Hardware section below.
 * 2. Calibrate the moisture sensor and update DRY_VALUE and WET_VALUE.
 * 3. Update WiFi credentials, ThingSpeak channel info, and Google Apps Script URL.
 * 4. Upload the sketch to your ESP32 and monitor the Serial output for status and debugging.
 *
 * Hardware:
 * - Freenove ESP32-S3-WROOM development board, N8R8 version
 * - ST7789 LCD 1.47" IPS 172x320 (LovyanGFX driver)
 * - Moisture sensor (analog)
 * - Water pump relay
 * - Camera module OV2640 compatible
 * - LEDs for WiFi status
 * - Pinouts: 
 *    - Moisture sensor: GPIO 1 (ADC1_CH0)
 *    - Water pump relay: GPIO 47
 *    - Red LED: GPIO 41
 *    - Blue LED: GPIO 42
 *    - LCD SPI pins defined in LGFX_ESP32_ST7789.hpp (SCLK=GPIO 46, MOSI=GPIO 3, DC=GPIO 2, CS=GPIO 14, RST=GPIO 48)
 *
 * Libraries & Dependencies:
 * - LovyanGFX (LCD) — install via Arduino Library Manager
 * - ThingSpeak (cloud upload) — install via Arduino Library Manager
 * - ArduinoJson, base64, and custom camera_api.h, google_drive.h, app_httpd.h (see repo or project docs)
 *
 * LCD Integration:
 * - Uses LGFX_ESP32_ST7789.hpp and LGFX_ESP32_ST7789.cpp for display control
 * - Library: LovyanGFX (https://github.com/lovyan03/LovyanGFX)
 * - lcdMoistureUpdate() updates moisture value on LCD
 *
 * Author: John Leung
 * Date: February 27, 2026
 *
 * -----------------------------------------------------------------------------
 * Main Loop Tasks:
 * 1. Periodically read moisture sensor, update LCD, capture/upload image, and update ThingSpeak.
 * 2. Manage water pump state machine.
 * 3. Blink LED for WiFi status.
 * -----------------------------------------------------------------------------
 * Troubleshooting:
 * - If WiFi does not connect, check credentials and signal strength.
 * - If ThingSpeak field is empty or truncated, ensure URL encoding is applied and field length is sufficient.
 * - If Google Drive image is not viewable, check sharing permissions (must be public or anyone with link).
 * - If LCD does not display, check wiring and LovyanGFX configuration.
 * - For custom libraries (camera_api.h, google_drive.h), see project documentation for installation and usage.
 */

// --- Libraries ---
#include <WiFi.h>
#include "ThingSpeak.h"
#include "camera_api.h"
#include "app_httpd.h"
#include "google_drive.h"
#include "LGFX_ESP32_ST7789.hpp"  //new

// --- Hardware Pin Definitions ---
#define SENSOR_PIN        1     //ESP32-S3 GPIO 1 (ADC1_CH0) for the moisture sensor
#define PUMP_RELAY_PIN    47    //ESP32-S3 GPIO 47 to control the water pump relay  
#define LED_RED_PIN       41
#define LED_BLUE_PIN      42

// --- Sensor Calibration ---
// Replace these with the values you found in the calibration step
const int DRY_VALUE = 4095; // Raw ADC value for 0% moisture (in air)
const int WET_VALUE = 1300; // Raw ADC value for 100% moisture (in water)

// --- Wi-Fi & ThingSpeak Configuration ---
#define SERIAL_MON_BAUDRATE 115200

const char* ssid = "YOUR_WIFI_SSID";     // Your network SSID (name)
const char* pass = "YOUR_WIFI_PASSWORD"; // Your network password

const unsigned long myChannelID = 123456;        // Your ThingSpeak channel number
const char *writeApiKey = "YOUR_THINGSPEAK_API_WRITE_KEY"; // Replace with your ThingSpeak API key
const unsigned int moistureFieldNumber = 1;          // Field number for moisture data

const uint8_t urlFieldNumber = 2; // Field number to upload the URL to

// Replace with your Google Apps Script Web App URL
const String webAppUrl = "https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec"; 

// --- Global Variables ---
WiFiClient thingspeakClient;

// --- Timing Control (Non-Blocking) ---
// Used to track time for various tasks without using delay()
unsigned long previousSensorReadMillis = 0;
unsigned long previousLedBlinkyMillis = 0;

// Set the intervals for how often tasks should run (in milliseconds)
const long sensorReadInterval = 30000;        // Read sensor every 30 seconds
const long ledBlinkyInterval = 1000;          // led blinks in 1 second, with "red led => no wifi", "blue led => good wifi"

// Pump turn-on time and soak time - need tuning for your own case
const unsigned int pumpOnTime = 1000;     // Pump ON time in milliseconds (1 second)
const unsigned int pumpSoakTime = 20000;  // Soak time in milliseconds (20 seconds)
// Hysteresis upper and lower moisture threshold values - also need tuning for your own case
const unsigned int upperMoistureThreshold = 35; // Upper threshold in %
const unsigned int lowerMoistureThreshold = 30; // Lower threshold in %

// --- Function Prototypes ---
void connectWiFi();
uint8_t readMoisture();
void thingspeakChannelsUpdateWithUrl(uint8_t moistureValue, const String& imageUrl);
void ledBlinky();
String imageCaptureGoogleDriveUploadAndGetUrl();
void startWaterPumpCycle();
void manageWaterPumpCycle(unsigned int onTime, unsigned int soakTime);
void lcdMoistureUpdate(uint8_t moistureValue);

// --- Add these new global variables ---
enum PumpState { IDLE, WATERING, SOAKING };
PumpState currentPumpState = IDLE;
unsigned long pumpStateChangeMillis = 0; // Tracks time for the current state

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

#ifdef USE_SD_MMC
  sdmmcInit();
  createDir(SD_MMC, "/camera");
  listDir(SD_MMC, "/camera", 0);
#endif

  if(cameraSetup()==1){
    Serial.println("Camera setup successful");
  } else {
    Serial.println("Error: Check your camera setup");
    return;
  }

  // --- new code to initialize the LCD and show a startup screen ---
  lcdInit();
  spriteDrawBackground();
  spriteSetFont(&fonts::DejaVu24);
  spritePrintf(10, 40, 0xFFFF00, "System initializing...");

  connectWiFi();
  delay(500); //a short delay to allow WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("******************************************************");
    Serial.println("ERROR: ESP32 is not connected to the WiFi router");
    Serial.println(" - The program will continue to run without WiFi.");
    Serial.println("******************************************************");
  } else {
    Serial.println("WiFi is connected.");
    startCameraServer();
    Serial.print("Camera Ready! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");
  }

  ThingSpeak.begin(thingspeakClient); // Initialize ThingSpeak client
}

// ==============================================================================
// LOOP: Runs continuously and is kept non-blocking
// ==============================================================================
void loop() {
  // Main loop tasks:
  // 1. Periodically read moisture sensor, update LCD, capture/upload image, and update ThingSpeak.
  // 2. Manage water pump state machine.
  // 3. Blink LED for WiFi status.

  unsigned long currentMillis = millis();
  uint8_t moistureValue;
  String imageUrl = "";

  // Task 1: Read the moisture sensor at its specified interval
  if (currentMillis - previousSensorReadMillis >= sensorReadInterval) {
    previousSensorReadMillis = currentMillis;
    moistureValue = readMoisture();
    lcdMoistureUpdate(moistureValue);
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
    }
    if (WiFi.status() == WL_CONNECTED) {
      imageUrl = imageCaptureGoogleDriveUploadAndGetUrl();
      thingspeakChannelsUpdateWithUrl(moistureValue, imageUrl);
    }
    if(moistureValue < lowerMoistureThreshold){
      startWaterPumpCycle();
    } else if(moistureValue > upperMoistureThreshold){
      // Optionally handle overwatering alert/action here.
    } // else: do nothing, within hysteresis band
  }
  // Task 2: Manage the pump state on every single loop iteration
  manageWaterPumpCycle(pumpOnTime, pumpSoakTime);
  // Task 3: Blink the LED to give a visual signal in the background.
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
 * @brief A new version of the readMoisture() function that returns the moisture percentage instead of updating a global variable. 
 *        This allows for more flexible use of the moisture value in different parts of the code without relying on a global state.
 * @return The current soil moisture percentage (0-100%).
 */
uint8_t readMoisture() {
  int rawValue = analogRead(SENSOR_PIN);
  
  // Map the raw value to a percentage
  uint8_t moisturePercent = map(rawValue, DRY_VALUE, WET_VALUE, 0, 100);
  
  // Constrain the value to the 0-100 range to prevent invalid readings
  moisturePercent = constrain(moisturePercent, 0, 100);

  Serial.print("Sensor Reading -> Raw: ");
  Serial.print(rawValue);
  Serial.print(", Moisture: ");
  Serial.print(moisturePercent);
  Serial.println("%");

  return moisturePercent;
}

/**
 * @brief Uploads the given moisture value and image URL to ThingSpeak in a single upload.
 * @param moistureValue The soil moisture percentage to upload.
 * @param imageUrl The URL of the uploaded image to include in the ThingSpeak upload. This should be URL-encoded if it contains special characters.
 */
void thingspeakChannelsUpdateWithUrl(uint8_t moistureValue, const String& imageUrl) {

  Serial.println("Uploading data to ThingSpeak with image URL...");
  ThingSpeak.setField(moistureFieldNumber, moistureValue);
  if(imageUrl != "") {
    ThingSpeak.setField(urlFieldNumber, imageUrl);
  }

  int httpCode = ThingSpeak.writeFields(myChannelID, writeApiKey);

  if (httpCode == 200) {
    Serial.println("Image URL uploaded to ThingSpeak successfully.");
  } else {
    Serial.println("Failed to upload image URL to ThingSpeak. Response code: " + String(httpCode));
  }
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

/**
 * @brief Captures an image using the camera, uploads it to Google Drive, and returns the URL of the uploaded image.
 * @return A String containing the URL of the uploaded image if successful, or an empty string if the capture or upload fails.
 * The URL will be URL-encoded (e.g., < becomes %3C, > becomes %3E, & becomes %26, = becomes %3D, etc.) to ensure it can be safely transmitted and used in HTTP requests.
 * The URL will be in the format "https://drive.google.com/uc?export=view&id=FILE_ID" which can be directly used to display the image in ThingSpeak or other platforms. 
 */
String imageCaptureGoogleDriveUploadAndGetUrl() {

    camera_fb_t * fb = NULL;
    //assumes default frame size and quality, you can modify the function call to specify them if needed
    fb = cameraSnapShot();  
    if (fb != NULL) {
      #ifdef USE_SD_MMC
      int photo_index = readFileNum(SD_MMC, "/camera");
      if(photo_index!=-1)
      {
        String filePath = "/camera/" + String(photo_index) +".jpg";
        writejpg(SD_MMC, filePath.c_str(), fb->buf, fb->len);
        Serial.printf("Image saved to %s\n", filePath.c_str());
      }
      #endif

      String driveResponse;
      bool uploadSuccess = uploadToGoogleDrive(webAppUrl, fb->buf, fb->len, driveResponse);
      cameraFrameBufferTrash(fb);
      if (uploadSuccess) {
        Serial.println("imageCaptureGoogleDriveUploadAndGetUrl(): " + driveResponse);
        return driveResponse; // Return the URL
      } else {
        Serial.println("Upload to Google Drive failed!");
        return ""; // Return empty string on failure
      }
    } else {
      Serial.println("Camera capture failed.");
      return ""; // Return empty string on failure
    }
}

/**
 * @brief Updates the soil moisture value displayed on the LCD. This function takes the moisture value as a parameter, 
 * allowing it to be called with any moisture value without relying on a global variable. 
 * The LCD will show the text "Moisture:" followed by the percentage value of the soil moisture.
 * @param moistureValue The soil moisture percentage to display on the LCD (0-100%
 */
void lcdMoistureUpdate(uint8_t moistureValue) {
  spriteDrawBackground(); // Redraw background to clear previous text
  spriteSetFont(&fonts::DejaVu24);
  spritePrintf(40, 40, 0xFFFF00U, "Moisture:"); 
  spriteSetFont(&fonts::DejaVu40);
  spritePrintf(40, 80, 0xFFFF00U, "%d%%", moistureValue);
}