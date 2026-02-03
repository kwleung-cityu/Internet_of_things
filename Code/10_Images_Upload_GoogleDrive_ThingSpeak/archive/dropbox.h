/**
 * This driver is not deployed in this project because Dropbox's API now requires OAuth 2.0 authentication
 * Dropbox’s move to short-lived tokens and OAuth 2.0 makes it more complex for unattended or embedded devices like ESP32.
 * Therefore this access token is NOT long-lived token generated from Dropbox App Console!!!
 */

#include "Arduino.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

void uploadToDropbox(uint8_t* imageData, size_t imageSize, const String& dropboxPath);
String createDropboxSharedLink(const String& dropboxPath);
