#include "vector_lib.h"

// Set all values in a vec_double_t to 0
void clear_double_vector(vec_double_t *vec) {
    memset(vec, 0, sizeof(vec_double_t));
}

// Set values in a vec_double_t to desired values
void set_double_vector(vec_double_t *vec, double x, double y, double z) {
    vec->x = x;
    vec->y = y;
    vec->z = z;
}

// Copy values from a vec_double_t to another
void copy_double_vector(vec_double_t *src, vec_double_t *dst) {
    memcpy(dst, src, sizeof(vec_double_t));
}

// Clear all values in a vec_int16_t
void clear_int16_vector(vec_int16_t *vec) {
    memset(vec, 0, sizeof(vec_int16_t));
}

// Set values in a vec_int16_t to desired values
void set_int16_vector(vec_int16_t *vec, int16_t x, int16_t y, int16_t z) {
    vec->x = x;
    vec->y = y;
    vec->z = z;
}

// Copy values from a vec_int16_t to another
void copy_int16_vector(vec_int16_t *src, vec_int16_t *dst) {
    memcpy(dst, src, sizeof(vec_int16_t));
}