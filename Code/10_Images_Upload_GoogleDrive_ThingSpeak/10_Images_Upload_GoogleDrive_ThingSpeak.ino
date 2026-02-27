// This code captures images using an ESP32 camera, uploads them to Google Drive, and then sends the image URL to ThingSpeak.
// It also includes a button to trigger the image capture and allows setting the image quality via serial input.
// The code is structured to be modular and easy to read, with clear separation of tasks in the main loop 
// and functions for specific operations like image capture, Google Drive upload, and ThingSpeak updates.

// Revision history:
// - Initial version: Feb 3, 2026
// - Update on Feb 27, 2026:
// 1. Added a local function urlEncode() to convert special characters in the Google Drive URL into their percent-encoded forms (e.g., < becomes %3C, > becomes %3E, & becomes %26, = becomes %3D, etc.) to ensure the URL can be safely transmitted and used in HTTP requests when uploading to ThingSpeak or other platforms.
// 2. Added URL encoding to the response URL in uploadToGoogleDrive() to ensure special characters are properly handled when transmitting the URL to ThingSpeak or other platforms.
// 3. Removed thingspeak_url.cpp and thingspeak_url.h as they are no longer needed with the new URL encoding implementation in google_drive.cpp.
// 4. Add #include "ThingSpeak.h" to the main .ino file to ensure ThingSpeak functions are available for uploading the URL to ThingSpeak after getting the response from Google Drive.
// 5. Add `myChannelID` and `urlFieldNumber` constants to specify the ThingSpeak channel and field number for uploading the URL, and use `ThingSpeak.writeField()` to upload the URL to ThingSpeak after a successful upload to Google Drive.

#include <WiFi.h>
#include "ThingSpeak.h"
#include "camera_api.h"
#include "app_httpd.h"
#include "google_drive.h"

#define BUTTON_PIN  0
// A simple state machine for button press handling
enum {BUTTON_UP, BUTTON_DEBOUNCE, BUTTON_DOWN, BUTTON_HOLD};
byte button_state = BUTTON_UP;


// ... WiFi and camera setup code ...
// ==================================
//    Enter your WiFi credentials
// ==================================
const char* ssid   = "YOUR_WIFI_SSID";
const char* pass 	 = "YOUR_WIFI_PASSWORD";

const unsigned long myChannelID = 1234567;        // Your ThingSpeak channel number
const char* writeApiKey  = "YOUR_THINGSPEAK_API_WRITE_KEY"; // Replace with your ThingSpeak API key
const uint8_t urlFieldNumber = 2; // Field number to upload the URL to
// Replace with your Google Apps Script Web App URL
const String webAppUrl = "https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec"; 

WiFiClient thingspeakClient; // Create a WiFi client for ThingSpeak
framesize_t size;
byte quality;
bool camera_shutter_trigger;

// ==============================================================================
// SETUP: Runs once at startup
// ==============================================================================
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  pinMode(BUTTON_PIN, INPUT_PULLUP);

#ifdef USE_SD_MMC
  sdmmcInit();

  //removeDir(SD_MMC, "/camera");
  createDir(SD_MMC, "/camera");
  listDir(SD_MMC, "/camera", 0);
#endif

  if(cameraSetup()==1){
    Serial.println("Camera setup successful");
  } else {
    Serial.println("Error: Check your camera setup");
    return;
  }

  WiFi.begin(ssid, pass);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  Serial.println("Press IO0 button on ESP32-S3 to capture an image, or");
  Serial.println("Select the image quality from 0-17 and quality from 4-63 (e.g. '10 10'):");

  ThingSpeak.begin(thingspeakClient); // Initialize ThingSpeak client
}

void loop() {

  // Handle button press with simple state machine
  if(digitalRead(BUTTON_PIN) == LOW){
    switch (button_state){
      case BUTTON_UP:
        button_state = BUTTON_DEBOUNCE;
        break;
      case BUTTON_DEBOUNCE:
        button_state = BUTTON_DOWN;
        break;
      case BUTTON_DOWN:
        size = DEFAULT_FRAME_SIZE;
        quality = DEFAULT_JPEG_QUALITY;
        camera_shutter_trigger = true;
        Serial.println("Button pressed. Capturing image with default settings.");      
        button_state = BUTTON_HOLD; 
        break;
      case BUTTON_HOLD:
        // do nothing even the user holding the button, 
        // the program will proceed with other tasks without hanging it up
        break;
    }
  } else {
    button_state = BUTTON_UP;
  }

   // Handle serial input for size and quality
  if(Serial.available()){
      String inputString = Serial.readStringUntil('\n');
      int spaceIndex = inputString.indexOf(' ');
      if (spaceIndex != -1) {
        String sizeStr = inputString.substring(0, spaceIndex);
        String qualityStr = inputString.substring(spaceIndex + 1);
        int s = sizeStr.toInt();
        int q = qualityStr.toInt();

        if((s >= 0) && (s <= 17) && (q >= 4) && (q <= 63)){
          size = (framesize_t)s;
          quality = (byte)q;
          camera_shutter_trigger = true;
          Serial.printf("Capturing image with size: %d, quality: %d\n", s, q);
        } else {
          Serial.println("Invalid size or quality. Size: 0-17, Quality: 4-63.");
        }
      } else {
        Serial.println("Invalid command. Please enter size and quality separated by a space.");
      }
  }

  if( camera_shutter_trigger ) {
    camera_fb_t * fb = NULL;
    fb = cameraSnapShot(size, quality);
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
          Serial.println("Image uploaded to Google Drive successfully. URL: " + driveResponse);
          // Upload the URL to ThingSpeak
          ThingSpeak.setField(urlFieldNumber, driveResponse);
          int httpCode = ThingSpeak.writeField(myChannelID, urlFieldNumber, driveResponse, writeApiKey);
          if (httpCode == 200) {
            Serial.println("Image URL uploaded to ThingSpeak successfully.");
          } else {
            Serial.println("Failed to upload image URL to ThingSpeak. Response code: " + String(httpCode));
          }
        } else {
          Serial.println("Upload to Google Drive failed!");
        }
    } else {
        Serial.println("Camera capture failed.");
    }

    camera_shutter_trigger = false;
    Serial.println("Select the image quality from 0-17 and quality from 4-63 (e.g. '10 10'):");
  }
  
  delay(10);
}


