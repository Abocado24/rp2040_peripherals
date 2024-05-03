/**
 * @file    circular_buffer.h
 * @brief   Defines an interface to use a byte array as a type-agnostic circular buffer.
 */

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef enum {
    CIRCULAR_BUFFER_RC_OK           = 0,
    CIRCULAR_BUFFER_RC_BAD_ARG      = 1,
    CIRCULAR_BUFFER_RC_OVERFLOW     = 2,
    CIRCULAR_BUFFER_RC_UNDERFLOW    = 3,
} circular_buffer_rc_t;

typedef struct {
    void * buffer;
    uint16_t buffer_capacity;
    uint8_t element_size;
    volatile uint16_t head;
    volatile uint16_t tail;
} circular_buffer_t;

/**
 * @brief   Initialize a circular buffer.
 * @details Uses void pointers to abstract away the type of data in the circular buffer.
 * @param   circular_buffer         The circular buffer struct.
 * @param   buffer                  The fixed-size byte array used to implement the circular buffer.
 * @param   buffer_capacity         The number of elements in the fixed-size byte array, needed for wraparound logic.
 * @param   element_size            The size of a single element in the circular buffer, used to navigate buffer.
 * @return  circular_buffer_rc_t    Return code indicating operation success/failure.
 *                                  - CIRCULAR_BUFFER_RC_OK:        Operation successful.
 *                                  - CIRCULAR_BUFFER_RC_BAD_ARG:   An invalid argument was provided.
 */
circular_buffer_rc_t circular_buffer_init(circular_buffer_t * circular_buffer, void * buffer, uint16_t buffer_capacity, uint16_t element_size);

/**
 * @brief   Get the current size of the circular buffer.
 * @details Is calculated from head and tail fields opposed to being stored in struct.
 *          Skips null arg checking for easier usage.
 * @param   circular_buffer     The circular buffer struct.
 */
uint16_t circular_buffer_get_size(circular_buffer_t * circular_buffer);

/**
 * @brief   Determine whether the circular buffer is empty.
 * @details Is calculated from head and tail fields opposed to being stored in struct.
 *          Skips null arg checking for easier usage.
 * @param   circular_buffer     The circular buffer struct.
 * @return  bool                Whether the circular buffer is empty.
 */
bool circular_buffer_is_empty(circular_buffer_t * circular_buffer);

/**
 * @brief   Determine whether the circular buffer is full.
 * @details Is calculated from head and tail fields opposed to being stored in struct.
 *          Skips null arg checking for easier usage.
 * @param   circular_buffer     The circular buffer struct.
 * @return  bool                Whether the circular buffer is full.
 */
bool circular_buffer_is_full(circular_buffer_t * circular_buffer);

/**
 * @brief   Push a new element onto the circular buffer.
 * @details Datatype size pushed must be consistent with existing data in the buffer.
 *          Overwrites the oldest buffer element if overflow occurs.
 * @param   circular_buffer         The circular buffer struct.
 * @param   txdata                  Pointer to data to push onto buffer.
 * @param   size                    Size of the data to push.
 * @return  circular_buffer_rc_t    Return code indicating operation success/failure.
 *                                  - CIRCULAR_BUFFER_RC_OK:        Operation successful.
 *                                  - CIRCULAR_BUFFER_RC_BAD_ARG:   An invalid argument was provided.
 *                                  - CIRCULAR_BUFFER_RC_OVERFLOW:  Overflow has occurred.
 */
circular_buffer_rc_t circular_buffer_push(circular_buffer_t * circular_buffer, void * txdata, uint8_t size);

/**
 * @brief   Pop an element from the circular buffer.
 * @details Datatype size popped to must be consistent with existing data in the buffer.
 *          Does nothing if underflow occurs.
 * @param   circular_buffer         The circular buffer struct.
 * @param   txdata                  Pointer to data to push onto buffer.
 * @param   size                    Size of the data to pop.
 * @return  circular_buffer_rc_t    Return code indicating operation success/failure.
 *                                  - CIRCULAR_BUFFER_RC_OK:        Operation successful.
 *                                  - CIRCULAR_BUFFER_RC_BAD_ARG:   An invalid argument was provided.
 *                                  - CIRCULAR_BUFFER_RC_UNDERFLOW: Underflow has occurred.
 */
circular_buffer_rc_t circular_buffer_pop(circular_buffer_t * circular_buffer, void * rxdata, uint8_t size);

#endif // CIRCULAR_BUFFER_H