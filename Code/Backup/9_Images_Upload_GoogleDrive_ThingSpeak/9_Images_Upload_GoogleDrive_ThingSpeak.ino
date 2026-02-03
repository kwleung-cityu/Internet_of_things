
#include <WiFi.h>

#include "camera_api.h"
#include "app_httpd.h"

#include "ws2812.h"

#include "google_drive.h"
#include "thingspeak.h"


// ... WiFi and camera setup code ...
// ==================================
//    Enter your WiFi credentials
// ==================================
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const String writeApiKey  = "YOUR_THINGSPEAK_API_KEY"; // Replace with your ThingSpeak API key
const uint8_t thingSpeakFieldNumber = 2; // Field number to upload the URL to
// Replace with your Google Apps Script Web App URL
// How to get this webAppUrl? Please refer to <4.3_Challenge3_Uploading_the_images_to_GoogleDrive.md>
const String webAppUrl = "https://script.google.com/macros/s/xyz......./exec"; 

#define BUTTON_PIN  0

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  ws2812Init();

#ifdef USE_SD_MMC
  sdmmcInit();

  //removeDir(SD_MMC, "/camera");
  createDir(SD_MMC, "/camera");
  listDir(SD_MMC, "/camera", 0);
#endif

  if(cameraSetup()==1){
    ws2812SetColor(2);
  } else {
    ws2812SetColor(1);
    Serial.println("Error: Check your camera setup");
    return;
  }

  WiFi.begin(ssid, password);
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
}

void loop() {

  if(digitalRead(BUTTON_PIN)==LOW){
    delay(20);
    if(digitalRead(BUTTON_PIN)==LOW){
	  	ws2812SetColor(3);
      camera_fb_t * fb = NULL;
      fb = cameraSnapShot();
      if (fb != NULL) {
          #ifdef USE_SD_MMC
          int photo_index = readFileNum(SD_MMC, "/camera");
          if(photo_index!=-1)
          {
            String filePath = "/camera/" + String(photo_index) +".jpg";
            writejpg(SD_MMC, filePath.c_str(), fb->buf, fb->len);
          }
          #endif
          String driveResponse;
          if (uploadToGoogleDrive(webAppUrl, fb->buf, fb->len, driveResponse)) {
              Serial.println("Upload successful!");
              Serial.println("Google Drive response: " + driveResponse);
              uploadUrlToThingSpeak(driveResponse, writeApiKey, thingSpeakFieldNumber);
          } else {
              Serial.println("Upload failed!");
          }
        
          cameraFrameBufferTrash(fb);
      } else {
        Serial.println("Camera capture failed.");
      }
      ws2812SetColor(2);

      while(digitalRead(BUTTON_PIN)==LOW);  //wait for button release
    }
  }

  delay(10);
}


