#ifndef APP_HTTPD_H
#define APP_HTTPD_H

// Uncomment this line if you want to save images to SD_MMC card of ESP32-S3
#define USE_SD_MMC

#ifdef USE_SD_MMC
#include "sd_read_write.h"
#endif

void startCameraServer();

#endif