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

### Porting the software to ESP32S3

[TODO...]




## Challenge 2: Sensing Augmented with Photo Taking from ESP32-S3

**Objective:** Add a camera to the project to visually monitor the plant's health. The ESP32-S3 is particularly well-suited for this task.

**Why do this?**
- **Richer Data:** A picture is worth a thousand data points. Visual inspection can reveal issues that a moisture sensor cannot, such as pests, discoloration, or physical damage.
- **Remote Monitoring:** Check on your plant's condition with a live image from anywhere in the world.
- **Project Example:** See the [Sketch_07.1_CameraWebServer](c:\Users\kwleung383\Documents\Personal\EE3070\SmartPetHouse\Freenove_ESP32_S3_WROOM_Board-main\C\Sketches\Sketch_07.1_CameraWebServer) for a practical implementation.

**What you will learn:**
- How to connect and configure an ESP32-CAM or a similar camera module.
- How to write code to capture an image.
- How to host a simple web server on the ESP32 to display the captured image in a web browser.

---

## Challenge 3: Uploading Photos to Google Drive and ThingSpeak

**Objective:** Instead of just viewing the photo on a local web server, this challenge involves automatically uploading the captured image to a cloud storage service like Google Drive and linking it to your ThingSpeak data.

**Why do this?**
- **Historical Record:** Create a time-lapse of your plant's growth by storing images in the cloud.
- **Data Integration:** Correlate moisture levels with visual appearance over time by associating each photo with a data entry in ThingSpeak.
- **Project Example:** The code in [ESP32_Camera_SDcard_GoogleDrive](c:\Users\kwleung383\Documents\Personal\EE3070\ESP32_Camera_SDcard_GoogleDrive) demonstrates how to achieve this.

**What you will learn:**
- How to use Google Drive API with an ESP32 to upload files.
- How to manage credentials and authentication for cloud services on a microcontroller.
- How to update a ThingSpeak channel with a link to the uploaded image.

---

## Challenge 4: Remote Control with MQTT

**Objective:** Implement the MQTT protocol to allow for real-time, two-way communication with your device. This enables you to send commands to your flowerpot instantly.

**Why do this?**
- **Instant Control:** While ThingSpeak is great for data logging, MQTT is a lightweight and efficient messaging protocol designed for instant command and control. You could trigger the water pump manually from your phone, for example.
- **Scalability:** MQTT is a standard for large-scale IoT systems, making it a valuable skill to learn.
- **Reliability:** It includes different Quality of Service (QoS) levels to ensure messages are delivered.

**What you will learn:**
- The basic principles of the MQTT publish/subscribe model.
- How to set up an MQTT broker (either a public one or your own).
- How to modify the ESP32 code to connect to an MQTT broker, subscribe to a "command" topic, and publish data to a "status" topic.
