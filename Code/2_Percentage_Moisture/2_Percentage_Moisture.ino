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