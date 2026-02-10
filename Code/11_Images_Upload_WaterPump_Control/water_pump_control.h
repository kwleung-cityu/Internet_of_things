#ifndef _WATER_PUMP_CONTROL_H
#define _WATER_PUMP_CONTROL_H

#include <Arduino.h>

#define PUMP_RELAY_PIN    47    //ESP32-S3 GPIO 47 to control the water pump relay  

/**
 * @brief Initializes the water pump control by setting up the relay pin.
 *        The relay pin is configured as an output and set to LOW (pump off) by default.
 */
void InitWaterPump();


/**
 * @brief Starts the water pump cycle by setting the relay pin HIGH to turn on the pump and recording the time when the cycle started.
 *        This function will only start a new cycle if the pump is currently idle to prevent overlapping
 */
void startWaterPumpCycle();

/**
 * @brief Controls the water pump by managing the pump state on every main loop iteration to handle timing and state changes internally.
 *        The function will use predefined durations for watering (pumpOnTime = 1000) and soaking (pumpSoakTime = 20000) in water_pump_control.cpp to manage the pump cycle.
 */
void controlWaterPump();

#endif

