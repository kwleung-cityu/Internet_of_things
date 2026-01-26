#ifndef THINGSPEAK_H
#define THINGSPEAK_H

#include <Arduino.h>

/**
 * Uploads a URL to ThingSpeak.
 * @param url The URL to be uploaded.
 * @param apiKey The ThingSpeak API key.
 * @param fieldNumber The field number to which the URL should be uploaded.
 */
void uploadUrlToThingSpeak(const String& url, const String& apiKey, uint8_t fieldNumber);

#endif
