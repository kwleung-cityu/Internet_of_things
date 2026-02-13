#include "thingspeak_url.h"
#include <HTTPClient.h>

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
 * Uploads a URL to ThingSpeak.
 * @param url The URL to be uploaded.
 * @param apiKey The ThingSpeak API key.
 * @param fieldNumber The field number to which the URL should be uploaded.
 */
void uploadUrlToThingSpeak(const String& url, const String& apiKey, uint8_t fieldNumber) {
  HTTPClient http;
  String encodeUrl = urlEncode(url);
  String requestUrl = "http://api.thingspeak.com/update?api_key=" + apiKey + "&field" + String(fieldNumber) + "=" + encodeUrl;    
  http.begin(requestUrl);
  int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.println("ThingSpeak update response: " + String(httpCode));
  } else {
    Serial.println("Error on ThingSpeak update: " + String(httpCode));
  }
  http.end();
}   
