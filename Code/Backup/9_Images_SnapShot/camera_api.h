#ifndef CAMERA_API_H
#define CAMERA_API_H

#include <Arduino.h>
// Select the Freenove ESP32-S3 board version-Has PSRAM (Read 8_Sketch_07.1_CameraWebServer.ino for other options)
#define CAMERA_MODEL_ESP32S3_EYE 
#include "camera_pins.h"
#include "esp_camera.h"

//Default frame size and JPEG quality assuming PSRAM is available
#define DEFAULT_FRAME_SIZE   FRAMESIZE_SVGA
#define DEFAULT_JPEG_QUALITY 10

/**
 * @brief Setup the camera with predefined configuration
 * @return 1 on success, 0 on failure
 */
int cameraSetup(void);

/**
 * @brief Capture a snapshot from the camera
 * @param size Frame size for the snapshot
 * enum type of framesize_t is defined in sensor.h (you may right click on it and choose "Go to Definition" to see more details after a successful build)
  Possible values are:
  typedef enum {
      FRAMESIZE_96X96,    // 96x96
      FRAMESIZE_QQVGA,    // 160x120
      FRAMESIZE_QCIF,     // 176x144
      FRAMESIZE_HQVGA,    // 240x176
      FRAMESIZE_240X240,  // 240x240
      FRAMESIZE_QVGA,     // 320x240
      FRAMESIZE_CIF,      // 400x296
      FRAMESIZE_HVGA,     // 480x320
      FRAMESIZE_VGA,      // 640x480
      FRAMESIZE_SVGA,     // 800x600
      FRAMESIZE_XGA,      // 1024x768
      FRAMESIZE_HD,       // 1280x720
      FRAMESIZE_SXGA,     // 1280x1024
      FRAMESIZE_UXGA,     // 1600x1200
      // 3MP Sensors
      FRAMESIZE_FHD,      // 1920x1080
      FRAMESIZE_P_HD,     //  720x1280
      FRAMESIZE_P_3MP,    //  864x1536
      FRAMESIZE_QXGA,     // 2048x1536
      // 5MP Sensors
      FRAMESIZE_QHD,      // 2560x1440
      FRAMESIZE_WQXGA,    // 2560x1600
      FRAMESIZE_P_FHD,    // 1080x1920
      FRAMESIZE_QSXGA,    // 2560x1920
      FRAMESIZE_INVALID
  } framesize_t;
  Choose according to your camera module capability
  Higher resolution requires more memory.
 * @param quality Quality of JPEG output. 0-63 lower means higher quality
  (e.g. 0=best, 63=worst). Default is 10.
 * @return Pointer to the captured frame buffer
  The returned pointer is of type camera_fb_t*, defined in esp_camera.h
 */
camera_fb_t* cameraSnapShot(framesize_t size = DEFAULT_FRAME_SIZE, byte quality = DEFAULT_JPEG_QUALITY);

/**
 * @brief Return the frame buffer back to the driver for reuse
 * @param fb Pointer to the frame buffer to be returned
 */
void cameraFrameBufferTrash(camera_fb_t* fb);
#endif