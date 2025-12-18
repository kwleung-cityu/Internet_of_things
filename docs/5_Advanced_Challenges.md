# Advanced Challenge: Precision Watering with PID Control

In the basic project, our water pump uses a simple on/off logic. This works, but it can sometimes lead to overwatering. For more precise control, we can implement a **Proportional-Integral-Derivative (PID)** control algorithm.

## What is PID?

PID is a control loop mechanism that is widely used in industrial control systems. Instead of just turning the pump on or off, it continuously calculates an "error" value (the difference between the current moisture and your target moisture) and adjusts the pump's run time to minimize this error over time.

*   **Proportional:** Reacts to the current error. A very dry plant gets more water than a slightly dry plant.
*   **Integral:** Looks at past errors. If the soil is consistently too dry, it will gradually increase the watering amount.
*   **Derivative:** Predicts future errors. It slows down the watering as it gets close to the target to avoid overshooting.

## Implementation Steps

1.  **Install the PID Library:** We will use a standard Arduino PID library. Go to `Tools > Manage Libraries...` and search for "PID" by Brett Beauregard. Install it.
2.  **The Code:** Here is how you would integrate the PID controller into your main loop...
    *   *(Provide the more advanced code here)*
3.  **Tuning the PID:** The hardest part of PID is "tuning" the Kp, Ki, and Kd constants...
    *   *(Explain what tuning is and give some starting values)*