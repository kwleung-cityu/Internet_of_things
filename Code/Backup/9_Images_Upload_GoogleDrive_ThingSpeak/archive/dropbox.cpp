#include "dropbox.h"

const char* host = "content.dropboxapi.com";
const int httpsPort = 443;
//Dropbox’s move to short-lived tokens and OAuth 2.0 makes it more complex for unattended or embedded devices like ESP32
//Therefore this access token is NOT long-lived token generated from Dropbox App Console!!!
String accessToken = "sl.u.AGJktLcSAf4ksFT4R4ciWZCvs07Z-aGpA014ZA0dj3pXD2p8j2iv8DkoJsNGlD8gZwPNnqv-sloNYZMAcJ797tyA7xhIPhBVvWX2tOVHqsMiFeVlBCvsidx5FWjs-A3nlrCeeSG8LK5soLSHGZMtkD97GA8C2jPOn7YiMQ40piQIU_bxqGbANdBotQt3wTLlbNtJ45z0WhOKI1lrss1XgkyWLAVTRiQlXovYjTWVnXxGkmdJT1cMx1B5_Td94BhwepMKGbpwrldpbtNddTtAnyKdaFlo5WUhmHX1L6jUAedOmTWF7UXiK0VQ4vgQUKMFQRYZrjAWyZ32reLvZJfqc1_8NTfBB5Ed07A48ZlCejFMp-yZ5DEnvjIWVTXtIjrRRYPNGg_l4XD3dIJzqmRGRlvJq9II22SR6FXBzDydtl0IYVoMxikgF5CUsiOFKuicDUeK8ODYDZme92GQGnhH7kRb790UP_xZ8wzthBLQtKDhgr9bOblOE21LXxAplUyHSSMb2yodi08BXodzHShFqVIgGCflpGIwbUuU1mg6MXefTytaZ094iytdSIoP9u_l3uTEntosMDpNIu7v-XJQq6Szq36o4iSDWSw_I1rvECQOsnSlEdDM-39a7zoH8gyrxcru48PuSnBZmAFjdDyIajBXc0gyMWNZPGK022byjJnk1judOT9MrrYsz7bCGhCIGFhlCHGmQnQmvSPpgvjju1t7Bp0bHf8Z7y5CiyK972N1FnmUWM9LFZ2SeIbRpxQ7AVkQf_kL7YPYeFilHzVGmwo1-Oto8figHQGgCb6O---aBs21k3PQRZc2MoskVcfB5ryncYRHh2Kpc4xOkFxtogcs4qHNliSybf-gthLO1-6LGiS7H_bFRsramu80o3kZs4buQEyObrtuqq0KNNz-X5w01mIn3P2W5GeAE9Bh6Wfuh3YuujzAjDDGcrimxiMMqHeCL5ZOVCV-i2WfFVKmk8-lzUaJ0OC4UrKHRHeVxJxFfAC8lnB9wrWLQXXS_BMiaCfDahJMMAT8qjQdPpOmXhXw-DxK8GqxX4-6P85x-prbXiSajYegM7WOCTVtG5S7FqTvj9N_lAclx42BD1ui64qaFZVt8rXAFpdzZ2EkuLNjsHX4EbC1mJTQUILG2QwZTnaAKn1CI742P5yFtd916Ta6h67iQne-GRUYAU9g70U7QuwdkIcuq6GzC8-3Ejzic1PMvmhBUO6-ZfFxeMP_FbHWZ1kUnpL5Hye27gmb2WlHymrUep-aorU0QhnVmyN1YAYFBIdqKp6oGDaQteGaOxIs";

static String convertToRawUrl(String url);

// Local function to convert shared link to direct download link
String convertToRawUrl(String url) {
    url.replace("dl=0", "raw=1");
    return url;
}

// Call uploadToDropbox() after capturing an image
void uploadToDropbox(uint8_t* imageData, size_t imageSize, const String& dropboxPath) {
  WiFiClientSecure client;
  client.setInsecure(); // For simplicity, skip certificate validation

  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed!");
    return;
  }

  String apiArg = "{\"path\": \"" + dropboxPath + "\",\"mode\": \"add\",\"autorename\": true,\"mute\": false}";
  String header = "POST /2/files/upload HTTP/1.1\r\n";
  header += "Host: " + String(host) + "\r\n";
  header += "Authorization: Bearer " + accessToken + "\r\n";
  header += "Dropbox-API-Arg: " + apiArg + "\r\n";
  header += "Content-Type: application/octet-stream\r\n";
  header += "Content-Length: " + String(imageSize) + "\r\n\r\n";

  client.print(header);
  client.write(imageData, imageSize);

  // Read response (optional)
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
    Serial.println(line);
  }
  client.stop();
}

// Create a shared link for the uploaded file
// String createDropboxSharedLink(const String& accessToken, const String& dropboxPath) {
String createDropboxSharedLink(const String& dropboxPath) {
    HTTPClient http;
    http.begin("https://api.dropboxapi.com/2/sharing/create_shared_link_with_settings");
    http.addHeader("Authorization", "Bearer " + accessToken);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"path\": \"" + dropboxPath + "\"}";
    int httpCode = http.POST(payload);
    
    String sharedUrl = "";
    String response = http.getString();
    Serial.println("Create link response: " + response);

    if (httpCode == 200) {
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);
        sharedUrl = doc["url"].as<String>();
    } else if (httpCode == 409) {
        // Shared link exists, list shared links
        http.end();
        http.begin("https://api.dropboxapi.com/2/sharing/list_shared_links");
        http.addHeader("Authorization", "Bearer " + accessToken);
        http.addHeader("Content-Type", "application/json");
        String listPayload = "{\"path\": \"" + dropboxPath + "\"}";
        int listHttpCode = http.POST(listPayload);
        String listResponse = http.getString();
        Serial.println("List link response: " + listResponse);
        if (listHttpCode == 200) {
            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, listResponse);
            if (!error && doc["links"].size() > 0) {
                sharedUrl = doc["links"][0]["url"].as<String>();
            }
        }
    }
    http.end();

    // Convert to direct download link
    sharedUrl = convertToRawUrl(sharedUrl);

    return sharedUrl;
}