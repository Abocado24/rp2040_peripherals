#ifndef HCSR04_H
#define HCSR04_H

// Includes
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// Defines
#define HCSR04_MAX_GPIO_PINS        40      // Max number of GPIO pins on RP2040
#define HCSR04_SPEED_OF_SOUND       0.0343  // speed of sound in cm/us (34300 cm/s)
#define HCSR04_DEFAULT_TIMEOUT_US   25000   // 25 ms, for a max distance of ~4 meters
#define HCSR04_DIST_INIT            -1.0f

// Struct Definitions
typedef enum {
    HCSR04_IDLE,
    HCSR04_BUSY
} hcsr04_state_t;

typedef enum {
    HCSR04_RC_OK,
    HCSR04_RC_IN_USE,
    HCSR04_RC_START_TIMEOUT,
    HCSR04_RC_END_TIMEOUT
} hcsr04_rc_t;

typedef struct {
    // GPIO defines
    uint trigger_pin;
    uint echo_pin;
   
    // Sensor defines
    volatile float current_distance;
    volatile bool echo_received;
    absolute_time_t start_time;
    absolute_time_t end_time;
    volatile hcsr04_state_t state;
} hcsr04_t;

// Function declarations
void hcsr04_init(hcsr04_t *sensor, uint trigger_pin, uint echo_pin);
hcsr04_rc_t hcsr04_get_distance(hcsr04_t *sensor, uint32_t timeout_us);
hcsr04_rc_t hcsr04_start_measurement(hcsr04_t *sensor);
hcsr04_rc_t hcsr04_end_measurement(hcsr04_t *sensor);
void hcsr04_reset(hcsr04_t *sensor);

#endif // HCSR04_H
