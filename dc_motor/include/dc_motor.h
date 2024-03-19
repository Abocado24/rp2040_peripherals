#ifndef DC_MOTOR_H
#define DC_MOTOR_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"

#include <stdlib.h>

typedef enum {
    DC_MOTOR_OK = 0,
    DC_MOTOR_NULL_INST = -1,
    DC_MOTOR_BAD_ARG = -2,
} dc_motor_rc_t;

typedef enum {
    DC_MOTOR_FWD = 0,
    DC_MOTOR_BWD = 1,
} dc_motor_direction_t;

typedef struct {
    uint8_t fwd_pin;
    uint8_t bwd_pin;
    uint8_t pwm_pin;
    uint pwm_slice;
    uint8_t pwm_channel;

    bool pwm_enabled;
    dc_motor_direction_t direction;
    uint16_t speed;
} dc_motor_t;

dc_motor_rc_t dc_motor_init(dc_motor_t * dc_motor, uint8_t fwd_pin, uint8_t bwd_pin, uint8_t pwm_pin, uint8_t pwm_slice, uint8_t pwm_channel);

dc_motor_rc_t dc_motor_start(dc_motor_t * dc_motor);
dc_motor_rc_t dc_motor_stop(dc_motor_t * dc_motor);
dc_motor_rc_t dc_motor_set_direction(dc_motor_t * dc_motor, dc_motor_direction_t direction);
dc_motor_rc_t dc_motor_set_speed(dc_motor_t * dc_motor, uint16_t speed);
dc_motor_rc_t dc_motor_set_percent_speed(dc_motor_t * dc_motor, double percent_speed);
dc_motor_rc_t dc_motor_set_velocity(dc_motor_t * dc_motor, int16_t velocity);
dc_motor_rc_t dc_motor_set_percent_velocity(dc_motor_t * dc_motor, double percent_velocity);

#endif // DC_MOTOR_H