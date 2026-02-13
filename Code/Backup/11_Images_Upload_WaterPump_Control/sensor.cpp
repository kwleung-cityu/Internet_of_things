#include "sensor.h"

const int DRY_VALUE = 4095; // Upper calibration value (max ADC value) for 100% reading
const int WET_VALUE = 1300; // Lower calibration value (min ADC value) for 0% reading
const byte UPPER_MOISTURE_DEFAULT = 35; // Default upper moisture threshold (in percentage)
const byte LOWER_MOISTURE_DEFAULT = 30; // Default lower moisture threshold (in percentage)

Sensor::Sensor(int pin, unsigned long samplingPeriod) {
  _pin = pin;
  _samplingPeriod = samplingPeriod;
  _lastReadTime = 0;
  _lastMoistureValue = 100; //initialize to 100% to avoid unintended water pump action on power up.
  _upperCalibration = DRY_VALUE;
  _lowerCalibration = WET_VALUE;
  _upperMoisture = UPPER_MOISTURE_DEFAULT;
  _lowerMoisture = LOWER_MOISTURE_DEFAULT;
}

void Sensor::setSamplingPeriod(unsigned long period) {
  _samplingPeriod = period;
}   

void Sensor::setUpperMoisture(byte upper) {
  _upperMoisture = upper;
}

void Sensor::setLowerMoisture(byte lower) {
  _lowerMoisture = lower;
}

byte Sensor::getUpperMoisture() {
  return _upperMoisture;
}

byte Sensor::getLowerMoisture() {
  return _lowerMoisture;
}

byte Sensor::readMoisture() {
  unsigned long currentTime = millis();
  if (currentTime - _lastReadTime >= _samplingPeriod) {
    _lastReadTime = currentTime;
    int rawValue = analogRead(_pin);
    // Map the raw ADC value to a percentage based on calibration
    int moisturePercentage = map(rawValue, _upperCalibration, _lowerCalibration, 0, 100);
    _lastMoistureValue = constrain(moisturePercentage, 0, 100); // Ensure it's between 0 and 100

    Serial.printf("Raw ADC: %d, Mapped Moisture: %d%%\n", rawValue, _lastMoistureValue);
  }
  return _lastMoistureValue;
}

bool Sensor::isMoistureLow() {
  byte moisture = readMoisture();
  bool isLow = moisture < _lowerMoisture;

  return isLow;
}

bool Sensor::isMoistureHigh() {
  byte moisture = readMoisture();
  bool isHigh = moisture > _upperMoisture;

  return isHigh;
}