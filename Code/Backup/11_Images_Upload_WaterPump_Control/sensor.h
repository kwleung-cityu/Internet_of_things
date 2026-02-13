#ifndef _SENSOR_H
#define _SENSOR_H

#include <Arduino.h>

class Sensor {
  public:
    // Constructor
    // pin: GPIO pin number for the sensor
    // samplingPeriod: Minimum time interval (in milliseconds) between consecutive readings to prevent excessive reads
    Sensor(int pin, unsigned long samplingPeriod = 5000);
    void setSamplingPeriod(unsigned long period);

    // set upper and lower sensor values for calibration
    void setUpperMoisture(byte upper);
    void setLowerMoisture(byte lower);

    byte getUpperMoisture();
    byte getLowerMoisture();
    byte readMoisture();
    bool isMoistureLow();
    bool isMoistureHigh();

  private:
    int _pin;
    unsigned long _samplingPeriod;
    unsigned long _lastReadTime;
    byte _lastMoistureValue;
    int _upperCalibration;
    int _lowerCalibration;
    byte _upperMoisture;
    byte _lowerMoisture;
};

#endif
