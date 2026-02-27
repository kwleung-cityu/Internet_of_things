#ifndef GOOGLE_DRIVE_H
#define GOOGLE_DRIVE_H

#include <Arduino.h>

/**
 * @brief Uploads an image to Google Drive via a Google Apps Script Web App.
 * @param webAppUrl The URL of the deployed Google Apps Script Web App that handles the upload.
 * @param imageData A pointer to the image data in memory (e.g., from the camera frame buffer).
 * @param imageSize The size of the image data in bytes.
 * @param response  A reference to a String variable where the function will store the URL of the uploaded image if the upload is successful.
 *                  Special characters in the URL will be URL-encoded (e.g., < becomes %3C, > becomes %3E, & becomes %26, = becomes %3D, etc.) to ensure it can be safely transmitted and used in HTTP requests.
 *                  The URL will be in the format "https://drive.google.com/uc?export=view&id=FILE_ID" which can be directly used to display the image in ThingSpeak or other platforms.
 * @return Returns true if the upload was successful and the URL was retrieved, false otherwise.
 */
bool uploadToGoogleDrive(const String& webAppUrl, uint8_t* imageData, size_t imageSize, String& response);

#endif