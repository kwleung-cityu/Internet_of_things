# Advanced Challenges

This section provides guides for the advanced challenges to extend the functionality of your Smart Flowerpot project.

## Challenge 1: Porting to ESP32 / ESP32-S3 Wifi SoC

**Objective:** Upgrade the project from the Arduino Mega 2560 + ESP8266 combination to a more powerful, all-in-one ESP32 or ESP32-S3 System-on-Chip (SoC).

**Why do this?**
- **Simplicity:** The ESP32 has built-in Wi-Fi, removing the need for a separate Wi-Fi module and complex serial communication between two chips.
- **Power:** The ESP32 is significantly more powerful than the Arduino Mega, with a faster dual-core processor, more RAM, and more GPIO pins.
- **Future-Proofing:** This architecture is more modern and opens the door to more complex tasks like running a web server, handling Bluetooth communication, or using a camera.

**What you will learn:**

- How to set up the Arduino IDE for ESP32 development.
- How to adapt the existing code to run on the ESP32 platform.
- The benefits of using an integrated SoC for IoT projects.

---
### Hardware
A kit with a Freenove® ESP32-S3 WROOM (FNK0085) Board for learning programming and electronics.

Official web site: https://store.freenove.com/products/fnk0085

User guide and tutorial download link: https://freenove.com/fnk0085

<img src = "./images/freenove_esp32s3.jpg">

<img src = "./images/ESP32S3_Pinout.png">

### Installing the ESP32-S3 Board in Arduino IDE

Follow these steps to set up your Arduino IDE for the Freenove ESP32-S3 board.

1.  **Add the ESP32 Boards Manager URL:**
    *   In the Arduino IDE, go to **File > Preferences**.
    *   In the "Additional boards manager URLs" field, paste the following link:
        ```
        https://espressif.github.io/arduino-esp32/package_esp32_index.json
        ```
    *   Click **OK**.

    <img src="./images/espressif_arduino_esp32_url.png">

2.  **Install the ESP32 Core:**
    *   Open the Boards Manager by clicking the icon on the left sidebar or by going to **Tools > Board > Boards Manager...**.
    *   Search for `esp32` and find the entry by **Espressif Systems**.
    *   Select a version later than `2.0.4` (this tutorial uses `3.0.7`) and click **Install**.

    <img src="./images/esp32_board_manager.png">

3.  **Install the USB Driver:**
    *   The ESP32-S3 board uses a CH343 chip for USB communication. You will need to install a driver for it.
    *   Download the driver **CH343SER.ZIP** from [this link (CH343SER.ZIP)](https://learn.adafruit.com/how-to-install-drivers-for-wch-usb-to-serial-chips-ch9102f-ch9102/windows-driver-installation).
    *   Unzip the downloaded file, open the folder, and run the **SETUP.EXE** installer.

4.  **Connect the Board and Select the Port:**
    
    *   Connect the ESP32-S3 board to your computer using a USB Type-C cable. Use the USB socket located next to the "BOOT" button.
    *   In the Arduino IDE, go to **Tools > Board > esp32** and select **ESP32S3 Dev Module**.
    *   Go to **Tools > Port** and select the new COM port that appeared after connecting the board.
    
    <img src="./images/tools_esp32_esp32s3_dev.png">

You will see a new COM port under ESP32S3 Dev Module.

<img src = "./images/ch343_new_com.png">

### Wiring up the sensor and actuator

Now that we are familiar with the ESP32-S3 board, let's connect the soil moisture sensor and the water pump.

#### Soil Moisture Sensor

The soil moisture sensor has three pins: VCC, GND, and AOUT.

-   Connect **VCC** to the **3.3V** pin on the ESP32-S3.
-   Connect **GND** to any **GND** pin on the ESP32-S3.
-   Connect **AOUT** (Analog Out) to a GPIO pin that can be used as an Analog-to-Digital Converter (ADC) input. For this project, we will use **GPIO 1**.

<img src = "./images/esp32s3_moisture_sensor_wiring.png">

*Note: The image above is a simplified representation. Ensure your connections are secure, preferably on a breadboard.*

#### Water Pump

The water pump is controlled by a relay module. The relay helps us control a higher voltage device (like the pump, which may require 5V or more) using a low-voltage signal from the ESP32-S3 (3.3V).

The relay module has pins for control (VCC, GND, IN) and for the load (NO, COM, NC).

-   Connect the relay's **VCC** to **3.3V** on the ESP32-S3.
-   Connect the relay's **GND** to a **GND** pin on the ESP32-S3.
-   Connect the relay's **IN** (Input) pin to a digital GPIO pin on the ESP32-S3. We will use **GPIO 38**.

For the pump itself:
-   Connect the **COM** (Common) terminal of the relay to the positive terminal of an external power supply (e.g., 5V).
-   Connect the **NO** (Normally Open) terminal of the relay to the positive (red) wire of the water pump.
-   Connect the negative (black) wire of the water pump to the negative terminal of the external power supply.
-   Make sure the ground of the external power supply is also connected to a GND pin on the ESP32-S3 to create a common ground.

<img src = "./images/esp32s3_water_pump_wiring.png">

*Warning: Always be careful when working with external power sources. Double-check your wiring before powering on the circuit.*

### Porting the software to ESP32S3

With the hardware wired up, the next step is to adapt the Arduino code to run on the ESP32-S3. The core logic remains the same, but we need to account for differences in pin numbers and how the ESP32 handles analog inputs.

#### Key Code Modifications

1.  **Update Pin Definitions:**
    The first and most obvious change is to update the pin numbers in your code to match the new wiring.

    -   The moisture sensor is now on `GPIO 1`.
    -   The water pump relay is on `GPIO 38`.

    Your pin definition section should look like this:

    ```cpp
    const int sensorPin = 1;  // ESP32-S3 GPIO 1 for the moisture sensor
    const int pumpPin = 38;   // ESP32-S3 GPIO 38 for the water pump relay
    ```

2.  **Analog-to-Digital Converter (ADC) Resolution:**
    The Arduino Mega uses a 10-bit ADC, which gives analog readings from 0 to 1023. The ESP32-S3 has a more precise 12-bit ADC, resulting in a range from 0 to 4095.

    This means the values you get from `analogRead(sensorPin)` will be different. You will need to **re-calibrate your sensor** using the `1_Calibrate_Moisture_Sensor.ino` sketch to find the new "wet" and "dry" values for the ESP32-S3.

    Your `map()` function to convert the raw reading to a percentage will need to use these new calibrated values. For example:

    ```cpp
    // Example values - you MUST find your own by calibrating!
    int wetValue = 1300; // Example: Raw ADC value when sensor is in water
    int dryValue = 4095; // Example: Raw ADC value when sensor is dry
    
    // Convert the raw sensor reading to a percentage
    int moisturePercentage = map(sensorValue, dryValue, wetValue, 0, 100);
    ```

3.  **Wi-Fi and ThingSpeak Code:**
    The code for connecting to Wi-Fi and uploading data to ThingSpeak will need to use the ESP32's built-in Wi-Fi library (`WiFi.h`) instead of the ESP8266-specific libraries.

    The good news is that the `ThingSpeak.h` library works seamlessly with the ESP32. You will primarily need to adjust the Wi-Fi connection part of the code.

    Here is a snippet from `3_ThingSpeak_Upload_Moisture_v2.ino` adapted for the ESP32:

    ```cpp
    #include <WiFi.h>
    #include "ThingSpeak.h"
    
    char ssid[] = "YourNetworkName";   // your network SSID (name)
    char pass[] = "YourPassword";      // your network password
    
    WiFiClient client;
    
    void setup() {
      Serial.begin(115200);
      WiFi.mode(WIFI_STA);
      ThingSpeak.begin(client);  // Initialize ThingSpeak
      // ... rest of the setup
    }
    
    void connectWifi() {
      // Connect or reconnect to WiFi
      if (WiFi.status() != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        while (WiFi.status() != WL_CONNECTED) {
          WiFi.begin(ssid, pass);
          Serial.print(".");
          delay(5000);
        }
        Serial.println("\nConnected.");
      }
    }
    ```
    The `connectWifi()` function handles the connection logic, and the main loop will call this before attempting to update ThingSpeak. The rest of the ThingSpeak logic for setting fields and writing data remains the same.
```

### Hardware Setup for the Camera

The Freenove ESP32-S3 board comes with a camera module. To get it working, you need to connect it correctly. The camera connects to a specific set of pins on the board.

1.  **Identify the Camera Connector:** Locate the camera connector on the ESP32-S3 board. It's a small, flat flex cable connector.
2.  **Connect the Camera:** Carefully insert the camera's ribbon cable into the connector. Ensure the blue tab on the cable is facing away from the board and the contacts are facing down. Secure the latch on the connector.

### Software for the Camera Web Server

The goal is to run a web server on the ESP32-S3 that streams video from the camera, which you can view in a web browser. We will use the `Sketch_07.1_CameraWebServer` example as a reference.

1.  **Select the Correct Camera Model:**
    In the `Sketch_07.1_CameraWebServer.ino` file, you need to uncomment the line that defines the camera model for your board. For the Freenove ESP32-S3 board, the correct model is `CAMERA_MODEL_ESP32S3_CAM_LCD`.

    Make sure this line is active in your `.ino` file:
    ```cpp
    #define CAMERA_MODEL_ESP32S3_CAM_LCD
```
    And ensure all other camera model definitions are commented out. This definition tells the compiler to use the correct pin configuration from the `camera_pins.h` file.

2.  **Enter Wi-Fi Credentials:**
    Just like with the ThingSpeak project, you need to provide your Wi-Fi network's SSID and password. Find these lines in `Sketch_07.1_CameraWebServer.ino` and update them:

    ```cpp
    const char* ssid = "REPLACE_WITH_YOUR_SSID";
    const char* password = "REPLACE_WITH_YOUR_PASSWORD";
    ```

3.  **Upload and Run:**
    -   In the Arduino IDE, make sure you have selected the **ESP32S3 Dev Module** board and the correct COM port.
    -   Upload the sketch to your board.
    -   Open the Serial Monitor and set the baud rate to **115200**.
    -   When the board connects to your Wi-Fi, it will print its IP address to the Serial Monitor.

    ```
    Camera Ready! Use 'http://192.168.1.123' to connect
    ```

4.  **View the Video Stream:**
    -   Open a web browser on a device connected to the same Wi-Fi network.
    -   Enter the IP address shown in the Serial Monitor.
    -   You should see a web page with controls for the camera and a live video stream.

This completes the second challenge, giving your Smart Flowerpot the ability to visually monitor your plant.

## Challenge 3: Storing Photos on an SD Card

Before uploading to the cloud, it's a good practice to save the captured image to a local storage medium like an SD card. This provides a backup and decouples the image capture process from the upload process.

1.  **Hardware: SD Card Module:**
    You will need an SD card module to connect to your ESP32-S3. These modules typically communicate over the SPI protocol.
    -   Connect the SD card module's VCC to **3.3V**.
    -   Connect GND to **GND**.
    -   Connect the SPI pins (MISO, MOSI, SCK, CS) to the corresponding SPI pins on the ESP32-S3. On the Freenove board, the default SPI pins are:
        -   **MOSI:** GPIO 35
        -   **MISO:** GPIO 37
        -   **SCK:** GPIO 36
        -   **CS (Chip Select):** GPIO 34 (this can often be changed in the code)

2.  **Software: SD Library:**
    The `SD.h` library for Arduino works with the ESP32. The code in `sd_read_write.cpp` from the example project shows how to initialize the SD card and save a file. The camera capture logic will provide a buffer (`fb`) containing the image data, which can then be written to a file on the card.

### Uploading to Google Drive

This is the most complex part of the challenge, as it involves authenticating with Google's services. The `ESP32_Camera_SDcard_GoogleDrive` project uses a Google Apps Script to act as an intermediary, which simplifies the process on the microcontroller.

1.  **Set up Google Apps Script:**
    -   You need to create a Google Apps Script that is deployed as a web app. This script will receive the image data from your ESP32 via an HTTP POST request and then use Google's native APIs to save the file to your Google Drive.
    -   The script handles the OAuth2 authentication on Google's servers, so your ESP32 doesn't need to manage complex tokens.
    -   The `google_drive.cpp` file contains the logic to send the photo to the URL of your deployed Apps Script. You will need to replace the placeholder URL with your own.

2.  **ESP32 Code for Uploading:**
    -   The code will first take a picture and save it to the SD card.
    -   Then, it reads the image file from the SD card.
    -   It creates an HTTP client to send a `multipart/form-data` POST request to your Google Apps Script URL.
    -   The body of the request contains the image data.
    -   The Apps Script receives this data, creates a file in a specific folder in your Google Drive, and returns the URL or ID of the newly created file.

### Integrating with ThingSpeak

Once the image is in Google Drive and you have its public link, the final step is to post that link to your ThingSpeak channel.

1.  **Add a Field to ThingSpeak:**
    In your ThingSpeak channel, you might want to use one of the fields to store the URL of the image.

2.  **Update the ESP32 Code:**
    -   After the `google_drive.cpp` code successfully uploads the file and gets back a link, you will use the `ThingSpeak.h` library to update your channel.
    -   Use `ThingSpeak.setField()` to set the image URL to the appropriate field.
    -   Use `ThingSpeak.writeFields()` to send the update.

This creates a powerful link between your sensor data and visual evidence, allowing you to see a snapshot of your plant's condition at the exact moment the moisture data was recorded.

## Challenge 4: Using MQTT for Remote Control

Before we dive into the code, let's understand the MQTT protocol.

### Understanding MQTT

MQTT (Message Queuing Telemetry Transport) is a lightweight messaging protocol ideal for IoT devices. It works on a publish/subscribe model, which is different from the request/response model of HTTP that ThingSpeak uses.

-   **Broker:** A central server that receives all messages and routes them to the appropriate clients.
-   **Client:** Any device that connects to the broker. A client can be a publisher, a subscriber, or both.
-   **Topic:** A "channel" or "label" for messages. Clients publish messages to topics, and they subscribe to topics to receive messages.
-   **Publish:** A client sends a message to the broker on a specific topic.
-   **Subscribe:** A client tells the broker it wants to receive all messages published to a specific topic.

This model decouples the sender and receiver. The ESP32 doesn't need to know the IP address of your phone; it only needs to know the address of the MQTT broker.

### Setting up an MQTT Broker

You have several options for an MQTT broker:
1.  **Use a Public Broker:** Services like [HiveMQ](https://www.hivemq.com/public-mqtt-broker/) or [Eclipse Mosquitto](https://test.mosquitto.org/) offer free, public brokers for testing and development. This is the easiest way to get started.
2.  **Run Your Own Broker:** For more control and security, you can run your own broker on a Raspberry Pi or a cloud server using software like Mosquitto.

For this challenge, we'll assume the use of a public broker.

### Software Implementation

You will need an MQTT client library for your ESP32. The `PubSubClient` library by Nick O'Leary is a popular and reliable choice.

1.  **Install the Library:**
    In the Arduino IDE, go to **Tools > Manage Libraries...** and search for `PubSubClient`. Install the library.

2.  **Code Modifications:**
    You'll need to add logic to connect to the MQTT broker, subscribe to a command topic, and handle incoming messages.

    **a. Configuration:**
    Define your broker, topics, and a unique client ID.

    ```cpp
    #include <PubSubClient.h>
    
    const char* mqtt_server = "broker.hivemq.com"; // Public MQTT Broker
    const char* command_topic = "smartflowerpot/command";
    const char* status_topic = "smartflowerpot/status";
    const char* client_id = "esp32-flowerpot-123"; // Must be unique
    
    WiFiClient espClient;
    PubSubClient client(espClient);
    ```

    **b. Callback Function:**
    This function is executed whenever a message is received on a topic you are subscribed to.

    ```cpp
    void callback(char* topic, byte* payload, unsigned int length) {
      Serial.print("Message arrived [");
      Serial.print(topic);
      Serial.print("] ");
      String message;
      for (int i = 0; i < length; i++) {
        message += (char)payload[i];
      }
      Serial.println(message);
    
      // Control the pump based on the message
      if (String(topic) == command_topic) {
        if (message == "WATER") {
          Serial.println("Received command to water the plant.");
          // Code to run the water pump for a few seconds
          digitalWrite(pumpPin, HIGH);
          delay(2000); // Run pump for 2 seconds
          digitalWrite(pumpPin, LOW);
          client.publish(status_topic, "Watered the plant");
        }
      }
    }
    ```

    **c. Reconnect Logic:**
    Create a function to connect to the broker and subscribe to your command topic. This function should be called in your `loop()` to ensure the connection is maintained.

    ```cpp
    void reconnect() {
      // Loop until we're reconnected
      while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect(client_id)) {
          Serial.println("connected");
          // Subscribe to the command topic
          client.subscribe(command_topic);
        } else {
          Serial.print("failed, rc=");
          Serial.print(client.state());
          Serial.println(" try again in 5 seconds");
          // Wait 5 seconds before retrying
          delay(5000);
        }
      }
    }
    ```

    **d. Setup and Loop:**
    In `setup()`, set the MQTT server and callback. In `loop()`, make sure the client is connected and call `client.loop()` to process incoming messages.

    ```cpp
    void setup() {
      // ... other setup code ...
      client.setServer(mqtt_server, 1883); // 1883 is the standard MQTT port
      client.setCallback(callback);
    }
    
    void loop() {
      if (!client.connected()) {
        reconnect();
      }
      client.loop(); // This is essential for the client to process messages
    
      // You can also publish status updates periodically
      // For example, publish the moisture level every minute
      // client.publish(status_topic, String(moisturePercentage).c_str());
    }
    ```

### Testing with an MQTT Client

To test your setup, you can use a desktop or mobile MQTT client (like [MQTT Explorer](http://mqtt-explorer.com/)).

1.  Connect the client to the same broker (`broker.hivemq.com`).
2.  Subscribe to the `smartflowerpot/status` topic. You should see any messages your ESP32 publishes.
3.  Publish a message to the `smartflowerpot/command` topic with the payload `WATER`.
4.  You should see your ESP32 receive the message in the Serial Monitor and activate the water pump. A status message should then appear in your MQTT client.

By completing this challenge, you will have a fully-featured IoT device that not only monitors and logs data but can also be controlled remotely in real-time.
