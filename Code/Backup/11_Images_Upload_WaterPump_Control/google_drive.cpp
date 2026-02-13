#include "google_drive.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <base64.h> // Arduino base64 library
#include <ArduinoJson.h>

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