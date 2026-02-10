
#include <WiFi.h>
#include "camera_api.h"
#include "app_httpd.h"

#include "google_drive.h"
#include "thingspeak.h"
#include "water_pump_control.h"

#define BUTTON_PIN  0
// A simple state machine for button press handling
enum {BUTTON_UP, BUTTON_DEBOUNCE, BUTTON_DOWN, BUTTON_HOLD};
byte button_state = BUTTON_UP;

// ... WiFi and camera setup code ...
// ==================================
//    Enter your WiFi credentials
// ==================================
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const String writeApiKey  = "YOUR_THINGSPEAK_API_WRITE_KEY"; // Replace with your ThingSpeak API key
const uint8_t thingSpeakFieldNumber = 2; // Field number to upload the URL to
// Replace with your Google Apps Script Web App URL
const String webAppUrl = "https://script.google.com/macros/s/<YOUR_DEPLOYMENT_ID>/exec"; 

// Local function prototypes
void connectWiFi();

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
  
  InitWaterPump();
}

framesize_t size;
byte quality;
bool camera_shutter_trigger = false;
bool water_pump_trigger = false;

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
  if( Serial.available() ) {
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

        if(WiFi.status() != WL_CONNECTED){
          Serial.println("WiFi disconnected. Attempting to reconnect...");
          connectWiFi();
        }

        if(WiFi.status() == WL_CONNECTED){
          String driveResponse;
          if (uploadToGoogleDrive(webAppUrl, fb->buf, fb->len, driveResponse)) {
              Serial.println("Upload successful!");
              Serial.println("Google Drive response: " + driveResponse);
              uploadUrlToThingSpeak(driveResponse, writeApiKey, thingSpeakFieldNumber);
          } else {
              Serial.println("Upload failed!");
          }
        } 
        cameraFrameBufferTrash(fb);
    } else {
        Serial.println("Camera capture failed.");
    }

    camera_shutter_trigger = false;
    water_pump_trigger = true; // Trigger the water pump cycle after capturing the image
    Serial.println("Select the image quality from 0-17 and quality from 4-63 (e.g. '10 10'):");
  }
  
  if(water_pump_trigger){
    startWaterPumpCycle();
    water_pump_trigger = false;
  }

  controlWaterPump(); // Call this in every loop iteration to manage the pump state and timing
  
  delay(10);
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  delay(500);  // Add a delay to allow the WiFi connection process to start
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");
    Serial.println("Press IO0 button on ESP32-S3 to capture an image, or");
    Serial.println("Select the image quality from 0-17 and quality from 4-63 (e.g. '10 10'):");
    startCameraServer();
  } else {
    Serial.println("");
    Serial.println("WiFi connection failed");
  }
}


