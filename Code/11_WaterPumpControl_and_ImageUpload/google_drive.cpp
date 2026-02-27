// Revisions
// February 27, 2026 -
// 1. Added a local function urlEncode() to convert special characters in the Google Drive URL into their percent-encoded forms (e.g., < becomes %3C, > becomes %3E, & becomes %26, = becomes %3D, etc.) to ensure the URL can be safely transmitted and used in HTTP requests when uploading to ThingSpeak or other platforms.
// 2. Added URL encoding to the response URL in uploadToGoogleDrive() to ensure special characters are properly handled when transmitting the URL to ThingSpeak or other platforms.

#include "google_drive.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <base64.h> // Arduino base64 library
#include <ArduinoJson.h>

// URL encode function to handle special characters
// Characters <, >, &, =, etc. should become %3C, %3E, %26, %3D, etc.
static String urlEncode(const String& str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encodedString += c;
    } else {
      encodedString += '%';
      code0 = (c >> 4) & 0xF;
      code1 = c & 0xF;
      encodedString += char(code0 > 9 ? code0 + 'A' - 10 : code0 + '0');
      encodedString += char(code1 > 9 ? code1 + 'A' - 10 : code1 + '0');
    }
  }
  return encodedString;
}

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
bool uploadToGoogleDrive(const String& webAppUrl, uint8_t* imageData, size_t imageSize, String& response) {
    // Encode image data to base64
    String encoded = base64::encode(imageData, imageSize);

    HTTPClient http;
    http.begin(webAppUrl);
    http.setTimeout(30000); // Set timeout to 30 seconds
    http.addHeader("Content-Type", "text/plain");

    // Explicitly tell the client to collect the "Location" header
    const char* headerKeys[] = {"Location"};
    http.collectHeaders(headerKeys, 1);

    int httpCode = http.POST(encoded);

    // Manually handle the redirect
    if (httpCode == 301 || httpCode == 302) {
        String redirectUrl = http.header("Location");
        Serial.println("Redirected to: " + redirectUrl);
        http.end(); // End the first request

        // Make a new request to the redirected URL
        http.begin(redirectUrl);
        httpCode = http.GET(); // The redirected request is a GET
    }

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Google Drive upload response payload: " + payload);

        // Parse the JSON response
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            http.end();
            return false;
        }

        // Extract the URL
        const char* status = doc["status"];
        if (status && strcmp(status, "success") == 0) {
            response = String(doc["url"].as<const char*>());
            response = urlEncode(response); // Replace characters <, >, &, =, etc. into %3C, %3E, %26, %3D for ThingSpeak upload
            http.end();
            return true;
        } else {
            Serial.println("Google Apps Script returned an error:");
            Serial.println(doc["message"].as<const char*>());
            http.end();
            return false;
        }
    } else {
        Serial.println("Error on Google Drive upload. HTTP Code: " + String(httpCode));
        response = http.getString();
        Serial.println("Response: " + response);
        http.end();
        return false;
    }
}