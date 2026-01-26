#include <WiFi.h>
#include "esp_camera.h"
#include "board_config.h"
#include "ws2812.h"
#include "sd_read_write.h"
#include "google_drive.h"
#include "thingspeak.h"

int cameraSetup(void);
void startCameraServer();

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
  sdmmcInit();
  //removeDir(SD_MMC, "/camera");
  createDir(SD_MMC, "/camera");
  listDir(SD_MMC, "/camera", 0);
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
      while(digitalRead(BUTTON_PIN)==LOW);
      camera_fb_t * fb = NULL;
      fb = esp_camera_fb_get();
      if (fb != NULL) {
        int photo_index = readFileNum(SD_MMC, "/camera");
        if(photo_index!=-1)
        {
          String filePath = "/camera/" + String(photo_index) +".jpg";
          writejpg(SD_MMC, filePath.c_str(), fb->buf, fb->len);
          String driveResponse;
          if (uploadToGoogleDrive(webAppUrl, fb->buf, fb->len, driveResponse)) {
              Serial.println("Upload successful!");
              Serial.println("Google Drive response: " + driveResponse);
              uploadUrlToThingSpeak(driveResponse, writeApiKey, thingSpeakFieldNumber);
          } else {
              Serial.println("Upload failed!");
          }
        }
        esp_camera_fb_return(fb);
      }
      else {
        Serial.println("Camera capture failed.");
      }
      ws2812SetColor(2);

      while(digitalRead(BUTTON_PIN)==LOW);  //wait for button release
    }
  }

  delay(10);
}

int cameraSetup(void) {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.frame_size = FRAMESIZE_VGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  // for larger pre-allocated frame buffer.
  if(psramFound()){
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    // Limit the frame size when PSRAM is not available
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return 0;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  s->set_vflip(s, 1); // flip it back
  s->set_brightness(s, 1); // up the brightness just a bit
  s->set_saturation(s, 0); // lower the saturation

  Serial.println("Camera configuration complete!");
  return 1;
}
