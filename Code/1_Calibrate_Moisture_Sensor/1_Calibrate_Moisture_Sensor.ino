// File: 1_Calibrate_Moisture_Sensor.ino

#define SENSOR_PIN A0 

void setup() {
  Serial.begin(115200);
}

void loop() {
  int rawValue = analogRead(SENSOR_PIN);
  
  Serial.print("Raw Analog Value: ");
  Serial.println(rawValue);

  delay(1000);
}