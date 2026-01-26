#ifndef GOOGLE_DRIVE_H
#define GOOGLE_DRIVE_H

#include <Arduino.h>

/**
 * Uploads image data to Google Drive via a Google Apps Script Web App.
 * @param webAppUrl The URL of the Google Apps Script Web App.
 * @param imageData Pointer to the image data to be uploaded.
 * @param imageSize Size of the image data in bytes.
 * @param response Reference to a String to store the response URL on success.
 * @return true if the upload was successful, false otherwise.
 */
bool uploadToGoogleDrive(const String& webAppUrl, uint8_t* imageData, size_t imageSize, String& response);

#endif