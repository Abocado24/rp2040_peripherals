#include "pico/stdlib.h"

#include "dc_motor.h"

#include <stdio.h>
#include <math.h>

// Motor control pins defined as constants
const uint MOTOR_FWD_PIN = 10;
const uint MOTOR_BWD_PIN = 11;
const uint MOTOR_PWM_PIN = 14;

int main() {
    stdio_init_all();

    // Create and initialize dc motor
    dc_motor_t dc_motor;
    uint pwm_slice = pwm_gpio_to_slice_num(MOTOR_PWM_PIN);
    uint8_t pwm_channel = PWM_CHAN_A;
    dc_motor_init(&dc_motor, MOTOR_FWD_PIN, MOTOR_BWD_PIN, MOTOR_PWM_PIN, pwm_slice, pwm_channel);

    // Main loop: cycle the motor speed
    while (true) {
        uint8_t steps = 200;
        // Motor will oscillate between maximum speeds of each direction using sinusoidal trajectory generation
        for(uint8_t i=0; i<steps; ++i) {
            double phase = i * M_PI/(steps/2);
            double percent_velocity = sin(phase);
            printf("percent velocity: %f\n", percent_velocity);
            dc_motor_set_percent_velocity(&dc_motor, percent_velocity);
            sleep_ms(500);
        }
    }

    return 0;
}
