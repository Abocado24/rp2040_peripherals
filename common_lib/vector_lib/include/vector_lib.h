#ifndef VECTOR_LIB_H
#define VECTOR_LIB_H

#include <stdint.h>

// Definition for a vector of int16_t
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} vec_int16_t;

// Definition for a vector of double
typedef struct {
    double x;
    double y;
    double z;
} vec_double_t;

// Function prototypes for operations on vec_double_t
void clear_double_vector(vec_double_t *vec);
void set_double_vector(vec_double_t *vec, double x, double y, double z);
void copy_double_vector(vec_double_t *src, vec_double_t *dst);

// Function prototypes for operations on vec_int16_t
void clear_int16_vector(vec_int16_t *vec);
void set_int16_vector(vec_int16_t *vec, int16_t x, int16_t y, int16_t z);
void copy_int16_vector(vec_int16_t *src, vec_int16_t *dst);

#endif // VECTOR_LIB_H
