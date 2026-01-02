// File: 3_ThingSpeak_Upload_Moisture

#include "ThingSpeak.h"
#include "WiFiEsp.h"

#define ESP_BAUDRATE  115200
char ssid[] = "YOUR_SSID";        // your network SSID (name) 
char pass[] = "YOUR_PASSWORD";    // your network password

const unsigned long myChannelNumber = 123456;             //replace it with your channel number
const char *myWriteAPIKey = "channel_write_apikey";       //replace it with your channel write api key
const unsigned int moistureFieldNumber = 1; 

WiFiEspClient thingspeakClient;

#define SENSOR_PIN A0 

// === Enter Your Calibration Values Here ===
// Value from when the sensor was in open air (0% moisture)
const int DRY_VALUE = 680; 
// Value from when the sensor was in water (100% moisture)
const int WET_VALUE = 250;  
// ========================================

void setup() {
  Serial.begin(115200);
  delay(500); //a short delay to let Serial port settle

  WiFi.init(&Serial1);  //using Serial1 for ESP8266 modem
  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }
  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      delay(500);     
    } 
    Serial.println("\nConnected");
  }

  ThingSpeak.begin(thingspeakClient); //initialize ThingSpeak client

}

void loop() {
  bool upload_success_flag = false;

  int rawValue = analogRead(SENSOR_PIN);
  
  // The map() function re-scales a number from one range to another.
  // Notice that our raw value goes DOWN as moisture goes UP. 
  // We map the range [DRY_VALUE, WET_VALUE] to the range [0, 100].
  int moisturePercent = map(rawValue, DRY_VALUE, WET_VALUE, 0, 100);

  // The constrain() function keeps the value within the 0-100 range,
  // which is useful if the sensor reading ever goes slightly outside
  // our calibration range.
  moisturePercent = constrain(moisturePercent, 0, 100);

  Serial.print("Raw: ");
  Serial.print(rawValue);
  Serial.print("  ->  Moisture: ");
  Serial.print(moisturePercent);
  Serial.println("%");

  ThingSpeak.setField(moistureFieldNumber, moisturePercent);

  while(!upload_success_flag){
    //int writeField(unsigned long channelNumber, unsigned int field, int value, const char * writeAPIKey)
    int status_code = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if(status_code == 200){
      Serial.println("Data upload successfully.");
      upload_success_flag = true;
    } else {
      Serial.println("Problem updating field.  HTTP error code " + String(status_code));
      Serial.println("Retrying...");
      delay(15000); //cannot write more frequent than 15sec with free ThingSpeak account
    }
  }

  delay(60000);     //new value in 60sec interval
}
