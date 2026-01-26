# A Time lapse microgreen

## Background

Take a snapshoot with ESP32 S3 microcontroller, store it to SD card, upload to Google Drive, and direct the image  to ThingSpeak

### Materials Needed
- Choice of microgreen seeds candidates: radish, broccoli, or mustard
- Shallow tray or container
- Growing medium (soil, coconut coir, or paper towels)
- Water spray bottle controlled by AD20P-0510A water pump with a mini shower head (mosfet control)
- ESP32S3 microcontroller with onboard CMOS camera (Freenove)
- LED grow light (mosfet control)
- Environmental sensing (temperature, humidity, and, luminance sensors)
- Wi-Fi access (NTP to sync time from the internet), RTC to gate in an hour interval
- Sensor data upload to Cloud in every hour
- Photo taking in every hour and save image to SD card
- Image upload to Google Drive but why not with ThingSpeak Image Channel? There is no image feature with free accounts. At least we need a student account (US$45/year) to use the image feature with max size quota 5MB (not a lot actually!)
  - Link: https://thingspeak.mathworks.com/prices/thingspeak_student

### Preparations
* Install esp32 core version of 3.0.7. This version guarantees that CameraWebServer example provided by Freenove can compile. If you don't care about Freenove example, you may use higher esp32 core versions. At time of writing the latest version is 3.3.4.
* Create a script for Google Drive to get the `webAppUrl`
* Create a MATLAB script

### Setting Up IoT Monitoring

1. Mount the ESP32S3 and camera above the tray for a clear top-down view
2. Connect the ESP32S3 to power and Wi-Fi
3. Program the ESP32S3 to:
   - Capture images at regular intervals (e.g., every hour)
   - Measure temperature, humidity, and light (add sensors if desired)
   - Upload images/data to a cloud service or local server for analysis
   - Augmented with charting with sensor data

### Growth and Data Collection

1. Remove the cover after germination; provide light (natural or LED)
2. Continue misting daily to keep the medium moist
3. Monitor growth via camera images and sensor data
4. Record observations and analyze growth patterns

### Smartphone app development

### Budgeting
