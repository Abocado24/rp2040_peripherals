#include "dc_motor.h"

// Internal values for PWM (keep constant until the need to dynamically adjust it comes up)
const float dc_motor_pwm_frequency = 64;
const uint16_t dc_motor_pwm_resolution = 4095;

// Initialize required pins for dc motor
dc_motor_rc_t dc_motor_init(dc_motor_t * dc_motor, uint8_t fwd_pin, uint8_t bwd_pin, uint8_t pwm_pin, uint8_t pwm_slice, uint8_t pwm_channel) {
    // Catch bad inputs
    if(!dc_motor) {
        return DC_MOTOR_NULL_INST;
    }
    if(pwm_slice != pwm_gpio_to_slice_num(pwm_pin)) {
        return DC_MOTOR_BAD_ARG;
    }
    
    // Initialize the direction control pins
    gpio_init(fwd_pin);
    gpio_set_dir(fwd_pin, GPIO_OUT);
    gpio_init(bwd_pin);
    gpio_set_dir(bwd_pin, GPIO_OUT);

    // Initialize PWM pin
    gpio_set_function(pwm_pin, GPIO_FUNC_PWM);
    pwm_set_clkdiv(pwm_slice, dc_motor_pwm_frequency);
    pwm_set_wrap(pwm_slice, dc_motor_pwm_resolution);
    pwm_set_enabled(pwm_slice, true);

    // Store values in struct for use in future functions
    dc_motor->fwd_pin = fwd_pin;
    dc_motor->bwd_pin = bwd_pin;
    dc_motor->pwm_pin = pwm_pin;
    dc_motor->pwm_slice = pwm_slice;
    dc_motor->pwm_channel = pwm_channel;

    // Set an initial state (stationary)
    dc_motor->pwm_enabled = true;
    dc_motor->direction = DC_MOTOR_FWD;
    gpio_put(dc_motor->fwd_pin, 1);
    gpio_put(dc_motor->bwd_pin, 0);
    pwm_set_chan_level(dc_motor->pwm_slice, dc_motor->pwm_channel, 0);
    dc_motor->speed = 0;

    return DC_MOTOR_OK;
}

// Start dc motor by turning on PWM
dc_motor_rc_t dc_motor_start(dc_motor_t * dc_motor) {
    // Catch bad inputs
    if(!dc_motor) {
        return DC_MOTOR_NULL_INST;
    }

    dc_motor->pwm_enabled = true;
    pwm_set_enabled(dc_motor->pwm_slice, true);

    return DC_MOTOR_OK;
}

// Stop dc motor by turning off PWM
dc_motor_rc_t dc_motor_stop(dc_motor_t * dc_motor) {
    // Catch bad inputs
    if(!dc_motor) {
        return DC_MOTOR_NULL_INST;
    }

    dc_motor->pwm_enabled = false;
    pwm_set_enabled(dc_motor->pwm_slice, false);

    return DC_MOTOR_OK;
}

// Set direction of dc motor by changing pin states
dc_motor_rc_t dc_motor_set_direction(dc_motor_t * dc_motor, dc_motor_direction_t direction) {
    // Catch bad inputs
    if(!dc_motor) {
        return DC_MOTOR_NULL_INST;
    }
    
    // Exit early if motor is already moving in desired direction
    if(direction == dc_motor->direction) {
        return DC_MOTOR_OK;
    }

    // Change direction and update state
    dc_motor->direction = direction;
    switch(direction) {
        case DC_MOTOR_FWD:
            gpio_put(dc_motor->fwd_pin, 1);
            gpio_put(dc_motor->bwd_pin, 0);
            break;
        case DC_MOTOR_BWD:
            gpio_put(dc_motor->fwd_pin, 0);
            gpio_put(dc_motor->bwd_pin, 1);
            break;
    }

    return DC_MOTOR_OK;
}

// Set speed of dc motor by adjusting pwm duty cycle
dc_motor_rc_t dc_motor_set_speed(dc_motor_t * dc_motor, uint16_t speed) {
    // Catch bad inputs
    if(!dc_motor) {
        return DC_MOTOR_NULL_INST;
    }

    // Wrap speeds that are too high down to the maximum
    if(speed > dc_motor_pwm_resolution) {
        speed = dc_motor_pwm_resolution;
    }

    // Set speed and update state
    pwm_set_chan_level(dc_motor->pwm_slice, dc_motor->pwm_channel, speed);
    dc_motor->speed = speed;

    return DC_MOTOR_OK;
}

// Wrapper for dc_motor_set_speed that instead sets relative to percent speed
dc_motor_rc_t dc_motor_set_percent_speed(dc_motor_t * dc_motor, double percent_speed) {
    // Wrap out of range percent_speed to minimum (0) or maximum (1.0)
    if(percent_speed < 0.0) {
        percent_speed = 0.0;
    }
    else if(percent_speed > 1.0) {
        percent_speed = 1.0;
    }

    // Map percent speed to appropriate duty cycle
    uint16_t speed = dc_motor_pwm_resolution * percent_speed;

    // Set motor to mapped speed
    return dc_motor_set_speed(dc_motor, speed);
}

// Set velocity (speed and direction) of dc motor via signed value
dc_motor_rc_t dc_motor_set_velocity(dc_motor_t * dc_motor, int16_t velocity) {
    // Catch bad inputs
    if(!dc_motor) {
        return DC_MOTOR_NULL_INST;
    }

    // Extract speed and direction from velocity
    uint16_t speed = abs(velocity);
    dc_motor_direction_t direction = velocity >= 0 ? DC_MOTOR_FWD : DC_MOTOR_BWD;

    // Set speed and direction
    dc_motor_set_speed(dc_motor, speed);
    dc_motor_set_direction(dc_motor, direction);

    return DC_MOTOR_OK;
}

// Wrapper for dc_motor_set_velocity that instead sets relative to percent speed
dc_motor_rc_t dc_motor_set_percent_velocity(dc_motor_t * dc_motor, double percent_velocity) {
    // Wrap out of range percent_velocity  to minimum (-1.0) or maximum (1.0)
    if(percent_velocity < -1.0) {
        percent_velocity = -1.0;
    }
    else if(percent_velocity > 1.0) {
        percent_velocity = 1.0;
    }

    // Map velocity to appropriate duty cycle
    int16_t velocity = dc_motor_pwm_resolution * percent_velocity;

    // Set motor to mapped velocity
    return dc_motor_set_velocity(dc_motor, velocity);
}