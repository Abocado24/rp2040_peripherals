#include "vector_lib.h"

// Set all values in a vec_double_t to 0
void clear_double_vector(vec_double_t *vec) {
    vec->x = 0.0;
    vec->y = 0.0;
    vec->z = 0.0;
}

// Set values in a vec_double_t to desired values
void set_double_vector(vec_double_t *vec, double x, double y, double z) {
    vec->x = x;
    vec->y = y;
    vec->z = z;
}

// Clear all values in a vec_int16_t
void clear_int16_vector(vec_int16_t *vec) {
    vec->x = 0;
    vec->y = 0;
    vec->z = 0;
}

// Set values in a vec_int16_t to desired values
void set_int16_vector(vec_int16_t *vec, int16_t x, int16_t y, int16_t z) {
    vec->x = x;
    vec->y = y;
    vec->z = z;
}
