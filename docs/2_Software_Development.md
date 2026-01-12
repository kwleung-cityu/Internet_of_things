# Software Development

**Writing the Software: A Step-by-Step Approach**
A complete IoT system has many parts that need to work together. Instead of writing one giant program, the best practice is to build and test the system in small, manageable pieces. This makes it much easier to understand and debug.

Our first step is to get a reliable reading from the moisture sensor.

## Part 1: Calibrating the Moisture Sensor

The analog output of the moisture sensor gives us a raw number, not a percentage. This raw value can vary between different sensors and depends on the voltage supplied. To get a meaningful moisture percentage (0% to 100%), we first need to calibrate our specific sensor.

**The Goal:** Find the sensor's raw output value for two extremes:

**0% Moisture:** The sensor is completely dry (in open air).
**100% Moisture:** The sensor is fully submerged in water.

```C
// File: 1_Calibrate_Moisture_Sensor.ino
// Connect the sensor's AO pin to the Arduino's A0 pin.
#define SENSOR_PIN A0 

void setup() {
  // Start the serial communication to see output on the Serial Monitor
  Serial.begin(115200);
}

void loop() {
  // Read the raw analog value from the sensor
  int rawValue = analogRead(SENSOR_PIN);
  
  Serial.print("Raw Analog Value: ");
  Serial.println(rawValue);

  delay(1000); // Wait for one second
}
```

**The Calibration Process**

The code for this step can be found in the `Code/1_Calibrate_Moisture_Sensor/` folder. 

1. Upload the `.ino` code above to your Arduino.
2. Open the Serial Monitor (`Tools -> Serial Monitor`) and set the baud rate to **115200**.
3. **Test the "Dry" value:** Hold the sensor in the open air for a few seconds and write down the raw value you see. This will be your 0% moisture reading.
4. **Test the "Wet" value:** Dip the sensor prongs completely into a glass of water. Write down the new raw value. This will be your 100% moisture reading.

**Example Results**

Your values will be slightly different, but they should look something like this:

- **In Open Air (0% moisture):** The raw value is around **680**.
- **In Water (100% moisture):** The raw value is around **250**.

```
// Example Serial Monitor Output
11:35:06.673 -> Raw Analog Value: 682
11:35:07.641 -> Raw Analog Value: 681
...
11:35:10.670 -> Raw Analog Value: 238
11:35:11.643 -> Raw Analog Value: 237
```

---

## Part 2: Converting Raw Values to a Percentage

Now that we have our unique calibration values, we can write a new program to convert the sensor's raw output into an intuitive 0% to 100% moisture scale. For this, we will use the incredibly useful `map()` function in Arduino.

**The Goal:** Create a program that prints the soil moisture as a percentage.

**1. The Code**

This program uses the `WET_VALUE` and `DRY_VALUE` you found in Part 1. It reads the sensor's current raw value and mathematically maps it to a 0-100 scale.

**Important:** Remember to replace the example values in the code below with the actual calibration values you recorded for your sensor.

```C++
// File: 2_Percentage_Moisture.ino
#define SENSOR_PIN A0 

// === Enter Your Calibration Values Here ===
// Value from when the sensor was in open air (0% moisture)
const int DRY_VALUE = 680; 
// Value from when the sensor was in water (100% moisture)
const int WET_VALUE = 250;  
// ========================================

void setup() {
  Serial.begin(115200);
}

void loop() {
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

  delay(1000);
}
```

**2. How the Code Works**

- **`DRY_VALUE` and `WET_VALUE`:** We store our calibration numbers in constant variables at the top of the code. This makes them easy to find and change.
- **`map(rawValue, DRY_VALUE, WET_VALUE, 0, 100)`:** This is the core of our conversion. It tells the Arduino: "Take the `rawValue`. If it's equal to `DRY_VALUE` (680), the result is 0%. If it's equal to `WET_VALUE` (250), the result is 100%. For any value in between, calculate the proportional percentage." The function is smart enough to handle the fact that our "from" range is inverted (from a high number to a low one).
- **`constrain(moisturePercent, 0, 100)`:** This is a safety function. It ensures that our final `moisturePercent` value will never be displayed as less than 0 or greater than 100, even if there's a strange spike in the sensor reading.

**3. Testing the Program**

The code for this step can be found in the `Code/2_Percentage_Moisture/` folder. 

1. Upload the new code to your Arduino.
2. Open Serial Monitor.
3. Place the sensor in soil with varying levels of moisture. You should now see a clear percentage that reflects how wet the soil is!

```
// Example Serial Monitor Output
Raw: 675  ->  Moisture: 1%
Raw: 510  ->  Moisture: 39%
Raw: 300  ->  Moisture: 88%
```

With this reliable percentage reading, we are now ready to connect our project to the internet and complete the feedback loop.

---

## Part 3A: Uploading moisture data to ThingSpeak - version 1

Now that we can accurately measure soil moisture, the next step is to send this data to the ThingSpeak channel we configured. This will involve writing Arduino code that connects to your Wi-Fi and sends HTTP requests containing the sensor data.

**The Goal:** Upload the soil moisture as a percentage to our ThingSpeak channel that we created in [**Part 3: Cloud Configuration**](../docs/3_Cloud_Configuration.md) and trigger to send a WhatsApp message to your smartphone when the percentage is less than a predefined threshold value.

**Prerequisites:**

* ThingSpeak library by MathWorks should be installed. 

  <img src = "./images/thingspeak_library_installed.png">

* WiFiEsp library by bportaluri should be installed

  <img src = "./images/wifiesp_library_installed.png">

**Only new code** relevant to WiFi connection and ThingSpeak code are listed below. 

**The complete source code can be found in the `Code/3_ThingSpeak_Upload_Moisture_v1/` folder.** 

````C++
#include "ThingSpeak.h"
#include "WiFiEsp.h"

#define ESP_BAUDRATE  115200
char ssid[] = "YOUR_SSID";        // your network SSID (name) 
char pass[] = "YOUR_PASSWORD";    // your network password

const unsigned long myChannelNumber = 123456;             //replace it with your channel number
const char *myWriteAPIKey = "channel_write_apikey";       //replace it with your channel write api key
const unsigned int moistureFieldNumber = 1; 			  //replace it with your field number

WiFiEspClient thingspeakClient;

//existing code...

void setup() {
    //existing code...
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
      	WiFi.begin(ssid, pass);
      	delay(500);     
    	} 
    Serial.println("\nConnected");
  	}

  	ThingSpeak.begin(thingspeakClient); //initialize ThingSpeak client    
}

void loop() {
	bool upload_success_flag = false;
    //existing code...
  	ThingSpeak.setField(moistureFieldNumber, moisturePercent);

      while(!upload_success_flag){
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
````

**Results of running the program:**

<img src = "./images/Thingspeak_upload_v1_serialmon.png">

<img src = "./images/1767932416225.jpg">

<img src= "./images/1767932427613.jpg">

To send the soil moisture data to the cloud, the code calls the `ThingSpeak.writeFields()` function. ThingSpeak then automatically adds a timestamp to each new reading as it arrives, which allows us to track the moisture levels over time.

We can test the alert system by simulating a "dry soil" event (0% moisture) by simply taking the sensor out of the water. This low value triggers a 'React' app within ThingSpeak, which is configured to use the ThingHTTP service (see [3_Cloud_Configuration.md](3_Cloud_Configuration.md) for setup details). 

<img src = "./images/Thingspeak_upload_v1_on_field1.png">

ThingHTTP then sends a REST API request to CallMeBot, an external service that sends a notification to my phone via WhatsApp, as shown in the screenshot below.

<img src = "./images/callmebot_message_1.jpg">

**Critique of this Preliminary Version**

The code above works, but it has several major flaws that make it unsuitable for a real-world IoT project.

1.  **Blocking Code with `delay()`:** The most significant problem lies in the use of the `delay()` function. `delay(60000)` pauses the entire program for 60 seconds. During this time, the Arduino can do nothing else—it cannot read other sensors, respond to button presses, or, most importantly, **control a water pump**. This makes the system unresponsive.

2.  **No Offline Resilience:** The program requires a constant internet connection. If the Wi-Fi router is down or the credentials are wrong, the `WiFi.begin(ssid, pass)` call inside the `while` loop will block forever, preventing the device from performing any other tasks, like controlling a pump locally. A robust IoT device should be able to function offline.

3.  **No Power Management:** The code runs in a continuous loop, consuming power at all times. For a battery-powered device that only needs to report data periodically, this is extremely inefficient. A real-world device would use a **deep sleep** mode to conserve battery, waking up only to take a reading and upload data.
Unfortunately, the existing hardware combination **Arduino Mega 2560 + ESP8266** is not well-suited for simple, low-power sleep because we are running two independent chips. We can put the Mega 2560 into a low-power sleep mode, but the ESP8266 module will remain fully powered on and continue to draw a significant amount of current. We will solve this with a standalone ESP32 solution coming next.

For now, we will focus on solving the first two problems. The next section introduces an improved sketch that uses **non-blocking code** and handles connection failures gracefully, creating a more robust foundation for our final project.

## Part 3B: Uploading moisture data to ThingSpeak - version 2

As we discussed, the `v1` sketch has major flaws: its use of `delay()` makes it unresponsive, and its Wi-Fi connection logic can cause the entire program to freeze. This second version (`v2`) solves these problems and adds visual feedback, creating a much more robust and user-friendly foundation.

**The Goal:** Rewrite the program to be non-blocking, resilient, and provide clear visual status updates.

**Key Concepts in the New Code**

1. **Non-Blocking Timing with `millis()`:** The `millis()` function returns the number of milliseconds since the Arduino board began running. By storing the time of the last action (e.g., `previousSensorReadMillis` ) and periodically checking if enough time has passed (`currentMillis - previousSensorReadMillis >= interval`), we can execute tasks at set intervals without ever pausing the entire program.
2. **A Responsive `loop()`:** Because we removed all the long `delay()` calls, the main `loop()` function now runs thousands of times per second. This allows us to check sensors, manage connections, and react to events in near real-time.
3. **Resilient Module Check:** In `setup()`, the code now checks if the ESP8266 module is physically connected and responding. If not, it prints a clear error message and disables all network functions, allowing the rest of the program (like sensor reading and pump control) to run without getting stuck.
4. **Efficient, On-Demand Wi-Fi:** Instead of constantly trying to stay connected, the code now only attempts to connect to Wi-Fi right before it needs to upload data. This is much more efficient and significantly reduces how often the blocking `WiFi.begin()` function pauses the loop.
5. **Visual Status with LEDs:** A new `ledBlinky()` function provides instant visual feedback on the system's status:
   - A **blinking red LED** indicates that there is no Wi-Fi connection (or the module is missing).
   - A **blinking blue LED** provides a clear confirmation that the device is successfully connected to Wi-Fi.

**The Complete V2 Code**
The complete source code can be found in the `Code/4_ThingSpeak_Upload_Moisture_v2/` folder. This new structure is the foundation for our final project. Notice the function `controlWaterPump()`is a placeholder for later implementation.

```C++
/********************************************************************************
 * File: 4_ThingSpeak_Upload_Moisture_v2.ino
 * 
 * Description:
 * This sketch is an improved, non-blocking version for reading a soil moisture
 * sensor and uploading the data to ThingSpeak using an Arduino Mega and
 * ESP8266 WiFi module. It avoids using delay() by tracking time with the 
 * millis() function. This allows the program to remain responsive and handle 
 * multiple tasks concurrently.
 * 
 * Key Improvements from v1:
 * 1. Non-Blocking Timing: Uses millis() instead of delay() to manage sensor 
 *    reading and data upload intervals.
 * 2. Modular Functions: Code is broken into smaller, single-purpose functions.
 * 3. Responsive Loop: The main loop() runs continuously, allowing for near
 *    real-time control and responsiveness.
 * 4. Efficient WiFi Handling: The code now only attempts to connect to WiFi
 *    right before it needs to upload data, reducing unnecessary blocking.
 * 5. New blinky LED feature to indicate WiFi status.
 *    Red LED indicates no WiFi connection. Blue LED indicates good WiFi connection.
 * 
 * Hardware Connections (Arduino Mega + ESP8266):
 * - Moisture Sensor AO -> Arduino A0
 * - ESP8266 TX/RX     -> Arduino RX1/TX1 (Serial1)
 * - Water Pump Relay  -> Arduino GPIO2
 * - Red LED Input -> Arduino GPIO3 
 * - Blue LED Input -> Arduino GPIO4
 * 
 ********************************************************************************/

// --- Libraries ---
#include "ThingSpeak.h"
#include "WiFiEsp.h"

// --- Hardware Pin Definitions ---
#define SENSOR_PIN A0 
#define PUMP_RELAY_PIN  2
#define LED_RED_PIN     3
#define LED_BLUE_PIN    4

// --- Sensor Calibration ---
// Replace these with the values you found in the calibration step
const int DRY_VALUE = 680; // Raw ADC value for 0% moisture (in air)
const int WET_VALUE = 250; // Raw ADC value for 100% moisture (in water)

// --- Wi-Fi & ThingSpeak Configuration ---
#define ESP_BAUDRATE 115200
#define SERIAL_MON_BAUDRATE 115200
char ssid[] = "YOUR_SSID";     // Your network SSID (name)
char pass[] = "YOUR_PASSWORD"; // Your network password

const unsigned long myChannelNumber = 123456;        // Your ThingSpeak channel number
const char *myWriteAPIKey = "channel_write_apikey";  // Your ThingSpeak Write API Key
const unsigned int moistureFieldNumber = 1;          // Field number for moisture data

// --- Global Variables ---
WiFiEspClient thingspeakClient;
int currentMoisturePercent = 0; // Holds the latest sensor reading
bool isWifiModuleOK = false;    // Flag to track if the ESP8266 is responding

// --- Timing Control (Non-Blocking) ---
// Used to track time for various tasks without using delay()
unsigned long previousSensorReadMillis = 0;
unsigned long previousThingSpeakUploadMillis = 0;
unsigned long previousLedBlinkyMillis = 0;

// Set the intervals for how often tasks should run (in milliseconds)
const long sensorReadInterval = 5000;       // Read sensor every 5 seconds
const long thingSpeakUploadInterval = 60000; // Upload to ThingSpeak every 60 seconds
const long ledBlinkyInterval = 1000;  //led blinks in 1 second, with "red led => no wifi", "blue led => good wifi"

// --- Function Prototypes ---
void connectWiFi();
void readMoisture();
void uploadToThingSpeak();
void controlWaterPump();
void ledBlinky();

// ==============================================================================
// SETUP: Runs once when the Arduino starts up
// ==============================================================================
void setup() {
  Serial.begin(SERIAL_MON_BAUDRATE);
  Serial1.begin(ESP_BAUDRATE); // Initialize Serial1 for ESP8266 modem
  delay(500); //a short delay to let Serial port settle
    
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, LOW); // Ensure pump is OFF by default
  digitalWrite(LED_RED_PIN, LOW); //turn off RED and BLUE LEDs to start with
  digitalWrite(LED_BLUE_PIN, LOW);

  WiFi.init(&Serial1);
  
  // Check if the WiFi module is responding.
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("******************************************************");
    Serial.println("ERROR: ESP8266 WiFi module not detected or not responding.");
    Serial.println(" - Check wiring between Mega and ESP8266.");
    Serial.println(" - Ensure ESP8266 has sufficient power.");
    Serial.println(" - The program will continue to run without WiFi.");
    Serial.println("******************************************************");
    isWifiModuleOK = false; // Set flag to prevent further WiFi attempts
  } else {
    Serial.println("WiFi module detected.");
    isWifiModuleOK = true; // WiFi module is OK
  }

  ThingSpeak.begin(thingspeakClient); // Initialize ThingSpeak client
}

// ==============================================================================
// LOOP: Runs continuously and is kept non-blocking
// ==============================================================================
void loop() {
  // Get the current time at the start of the loop
  unsigned long currentMillis = millis();

  // Task 1: Read the moisture sensor at its specified interval
  if (currentMillis - previousSensorReadMillis >= sensorReadInterval) {
    previousSensorReadMillis = currentMillis; // Save the time of this reading
    readMoisture();
  }

  // Task 2: Attempt to upload data to ThingSpeak at its specified interval
  if (currentMillis - previousThingSpeakUploadMillis >= thingSpeakUploadInterval) {
    previousThingSpeakUploadMillis = currentMillis; // Save the time of this attempt
    
    // Only proceed if the WiFi module was detected at startup
    if (isWifiModuleOK) {
      // If not connected, try to connect now.
      if (WiFi.status() != WL_CONNECTED) {
        connectWiFi(); 
      }
      
      // After the connection attempt, check again before uploading.
      if (WiFi.status() == WL_CONNECTED) {
        uploadToThingSpeak();
      }
    }
  }

  // Task 3: Control the water pump (runs on every loop)
  controlWaterPump();

  // Task 4: Blink the LED to give a visual signal
  if(currentMillis - previousLedBlinkyMillis >= ledBlinkyInterval){
    previousLedBlinkyMillis = currentMillis;
    ledBlinky();
  }
}

// ==============================================================================
// --- Helper Functions ---
// ==============================================================================

/**
 * @brief Attempts to connect to the Wi-Fi network.
 * NOTE: WiFi.begin() is a BLOCKING call and can take several seconds to
 * execute. During this time, the main loop will be paused.
 */
void connectWiFi() {
  Serial.println("Attempting to connect to WiFi (this may block for a few seconds)...");
  WiFi.begin(ssid, pass);
}

/**
 * @brief Reads the moisture sensor and updates the global variable.
 */
void readMoisture() {
  int rawValue = analogRead(SENSOR_PIN);
  
  // Map the raw value to a percentage
  currentMoisturePercent = map(rawValue, DRY_VALUE, WET_VALUE, 0, 100);
  
  // Constrain the value to the 0-100 range to prevent invalid readings
  currentMoisturePercent = constrain(currentMoisturePercent, 0, 100);

  Serial.print("Sensor Reading -> Raw: ");
  Serial.print(rawValue);
  Serial.print(", Moisture: ");
  Serial.print(currentMoisturePercent);
  Serial.println("%");
}

/**
 * @brief Uploads the current moisture percentage to ThingSpeak.
 */
void uploadToThingSpeak() {
  Serial.println("Uploading data to ThingSpeak...");
  ThingSpeak.setField(moistureFieldNumber, currentMoisturePercent);

  int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if (httpCode == 200) {
    Serial.println("ThingSpeak upload successful.");
  } else {
    Serial.println("Problem uploading to ThingSpeak. HTTP error code " + String(httpCode));
  }
}

/**
 * @brief Placeholder function for water pump control logic.
 */
void controlWaterPump() {
  // This is where you would add the logic to control the pump.
  // For example:
  // if (currentMoisturePercent < 30) {
  //   digitalWrite(PUMP_RELAY_PIN, HIGH); // Turn pump ON
  // } else {
  //   digitalWrite(PUMP_RELAY_PIN, LOW);  // Turn pump OFF
  // }
}

/**
 * @brief Blinks the LED to indicate WiFi status.
 * Red LED indicates no WiFi connection. Blue LED indicates good WiFi connection.
 */
void ledBlinky() {
  if(isWifiModuleOK && WiFi.status() == WL_CONNECTED){
    // Blink Blue LED
    digitalWrite(LED_RED_PIN, LOW); // Ensure RED is OFF
    digitalWrite(LED_BLUE_PIN, !digitalRead(LED_BLUE_PIN)); // Toggle BLUE LED
  } else {
    // Blink Red LED
    digitalWrite(LED_BLUE_PIN, LOW); // Ensure BLUE is OFF
    digitalWrite(LED_RED_PIN, !digitalRead(LED_RED_PIN)); // Toggle RED LED
  }
}
```


## Part 4: React - Closing the Loop with Automated Watering

*   Safely connect a water pump to the Arduino using a relay module.
*   Modify the Arduino code to automatically activate the water pump when the moisture level drops below the safe range.