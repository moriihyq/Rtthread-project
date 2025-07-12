/* push_rod_control.c */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

// Define the GPIO pins for the push rod control (H-Bridge IN1 and IN2)
// This needs to be configured based on the actual hardware connection
#define PUSH_ROD_PIN_IN1    "P508"  // Renamed for clarity, controls direction 1
#define PUSH_ROD_PIN_IN2    "P509"  // Controls direction 2


void push_rod_init(void)
{
    // --- MODIFIED ---
    // Initialize BOTH pins as output
    rt_pin_mode(rt_pin_get(PUSH_ROD_PIN_IN1), PIN_MODE_OUTPUT);
    rt_pin_mode(rt_pin_get(PUSH_ROD_PIN_IN2), PIN_MODE_OUTPUT);

    // Set both pins to LOW initially. This puts the H-Bridge in a "stop" or "brake" state.
    // It's the safest state to be in at startup.
    rt_pin_write(rt_pin_get(PUSH_ROD_PIN_IN1), PIN_LOW);
    rt_pin_write(rt_pin_get(PUSH_ROD_PIN_IN2), PIN_LOW);

    rt_kprintf("Push rod control initialized.\n");
}

void push_rod_extend(void)
{
    rt_kprintf("Push rod extending...\n");

    // --- MODIFIED ---
    // To extend, set IN1 to HIGH and IN2 to LOW (example logic, might need to swap)
    rt_pin_write(rt_pin_get(PUSH_ROD_PIN_IN1), PIN_HIGH);
    rt_pin_write(rt_pin_get(PUSH_ROD_PIN_IN2), PIN_LOW);

    // This delay now represents how long the motor is powered to extend
    rt_thread_mdelay(500); // Simulate extension time (adjust as needed)

    // --- ADDED ---
    // After extending, stop the motor to prevent overheating and save power
    rt_pin_write(rt_pin_get(PUSH_ROD_PIN_IN1), PIN_LOW);
    rt_pin_write(rt_pin_get(PUSH_ROD_PIN_IN2), PIN_LOW);
}

void push_rod_retract(void)
{
    rt_kprintf("Push rod retracting...\n");

    // --- MODIFIED ---
    // To retract, set IN1 to LOW and IN2 to HIGH (the opposite of extend)
    rt_pin_write(rt_pin_get(PUSH_ROD_PIN_IN1), PIN_LOW);
    rt_pin_write(rt_pin_get(PUSH_ROD_PIN_IN2), PIN_HIGH);

    // This delay now represents how long the motor is powered to retract
    rt_thread_mdelay(500); // Simulate retraction time (adjust as needed)

    // --- ADDED ---
    // After retracting, stop the motor
    rt_pin_write(rt_pin_get(PUSH_ROD_PIN_IN1), PIN_LOW);
    rt_pin_write(rt_pin_get(PUSH_ROD_PIN_IN2), PIN_LOW);
}


