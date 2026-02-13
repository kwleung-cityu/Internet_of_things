#include "water_pump_control.h"

// --- Add these new global variables ---
enum PumpState { IDLE, WATERING, SOAKING };
PumpState currentPumpState = IDLE;
unsigned long pumpStateChangeMillis = 0; // Tracks time for the current state

// Pump turn-on time and soak time - need tuning for your own case
const unsigned int pumpOnTime = 1000; // Pump ON time in milliseconds (1 second)
const unsigned int pumpSoakTime = 20000; // Soak time in milliseconds (20 seconds)

/**
 * @brief Local function to manage the water pump cycle using a state machine. 
 * It checks the current state of the pump and transitions between states based on elapsed time.
 * @param onTime Duration for which the pump should be ON (in milliseconds).
 * @param soakTime Duration for which the soil should soak after watering (in milliseconds).
 */
void manageWaterPumpCycle(unsigned int onTime, unsigned int soakTime);

/**
 * @brief Initializes the water pump control by setting up the relay pin.
 *        The relay pin is configured as an output and set to LOW (pump off) by
 */
void InitWaterPump()
{
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, LOW); // Ensure pump is OFF by default
}

/**
 * @brief Controls the water pump by managing the pump state on every loop iteration to handle timing and state changes internally.
 *        The function will use predefined durations for watering and soaking to manage the pump cycle.
 */
void controlWaterPump() {

  // Manage the pump state on every single loop iteration
  // The function will handle the timing and state changes internally.
  // Use 1000ms for watering and 20000ms (20s) for soaking.
  manageWaterPumpCycle(pumpOnTime, pumpSoakTime);  
}

/**
 * @brief Starts the water pump cycle by setting the relay pin HIGH to turn on the pump and recording the time when the cycle started.
 *        This function will only start a new cycle if the pump is currently idle to prevent overlapping
 */
void startWaterPumpCycle() {
  // Only start a new cycle if the pump is currently idle
  if (currentPumpState == IDLE) {
    currentPumpState = WATERING;
    pumpStateChangeMillis = millis(); // Record the time we started watering
    digitalWrite(PUMP_RELAY_PIN, HIGH);
    Serial.println("Pump cycle started: WATERING");
  }
}

//-----------------------LOCAL FUNCTIONS--------------------------

/**
 * @brief Local function to manage the water pump cycle using a state machine.
 * @param onTime Duration for which the pump should be ON (in milliseconds).
 * @param soakTime Duration for which the soil should soak after watering (in milliseconds).
 */
void manageWaterPumpCycle(unsigned int onTime, unsigned int soakTime) {
  // This function is a state machine. It does nothing unless a state is active.
  
  // State 1: The pump is currently WATERING
  if (currentPumpState == WATERING) {
    // Check if the 'onTime' has elapsed
    if (millis() - pumpStateChangeMillis >= onTime) {
      // Time to switch to the SOAKING state
      currentPumpState = SOAKING;
      pumpStateChangeMillis = millis(); // Record the time we started soaking
      digitalWrite(PUMP_RELAY_PIN, LOW);
      Serial.println("Watering finished. Now SOAKING.");
    }
  }  // State 2: The soil is currently SOAKING
  else if (currentPumpState == SOAKING) { 
    // Check if the 'soakTime' has elapsed
    if (millis() - pumpStateChangeMillis >= soakTime) {
      // The cycle is complete, return to IDLE
      currentPumpState = IDLE;
      Serial.println("Soak time complete. Pump cycle finished.");
    }
  }
}