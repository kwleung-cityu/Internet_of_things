# Software Development

**Writing the Software: A Step-by-Step Approach**
A complete IoT system has many parts that need to work together. Instead of writing one giant program, the best practice is to build and test the system in small, manageable pieces. This makes it much easier to understand and debug.

Our first step is to get a reliable reading from the moisture sensor.

**Part 1: Calibrating the Moisture Sensor **

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

**Part 2: Converting Raw Values to a Percentage**

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
- **`map(rawValue, DRY_VALUE, WET_VALUE, 0, 100)`:** This is the core of our conversion. It tells the Arduino: "Take the `rawValue`. If it's equal to `DRY_VALUE` (680), the result is 0. If it's equal to `WET_VALUE` (250), the result is 100. For any value in between, calculate the proportional percentage." The function is smart enough to handle the fact that our "from" range is inverted (from a high number to a low one).
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

With this reliable percentage reading, we are now ready to connect our project to the internet in the next step.

---

**Part 3: Uploading moisture data to ThingSpeak**

**Part 4: Simple algorithm to react on abnormality** 

**Part 5: React - Closing the Loop with Automated Watering**

*   Safely connect a water pump to the Arduino using a relay module.
*   Modify the Arduino code to automatically activate the water pump when the moisture level drops below the safe range.