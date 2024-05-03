#include "circular_buffer.h"

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
circular_buffer_rc_t circular_buffer_init(circular_buffer_t * circular_buffer, void * buffer, uint16_t buffer_capacity, uint16_t element_size) {
    if(circular_buffer == NULL || buffer == NULL || buffer_capacity == 0 || element_size == 0) {
        return CIRCULAR_BUFFER_RC_BAD_ARG;
    }

    // set fields in circular buffer struct
    circular_buffer->buffer = buffer;
    circular_buffer->buffer_capacity = buffer_capacity;
    circular_buffer->element_size = element_size;

    // initialize circular buffer to an empty state
    circular_buffer->head = 0;
    circular_buffer->tail = 0;

    return CIRCULAR_BUFFER_RC_OK;
}

/**
 * @brief   Get the current size of the circular buffer.
 * @details Is calculated from head and tail fields opposed to being stored in struct.
 *          Skips null arg checking for easier usage.
 * @param   circular_buffer     The circular buffer struct.
 */
uint16_t circular_buffer_get_size(circular_buffer_t * circular_buffer) {
    // calculation depends on whether the buffer is in a wraparound state
    if(circular_buffer->head >= circular_buffer->tail) {
        return circular_buffer->head - circular_buffer->tail;
    }
    else {
        return circular_buffer->buffer_capacity - circular_buffer->tail + circular_buffer->head;
    }
}

/**
 * @brief   Determine whether the circular buffer is empty.
 * @details Is calculated from head and tail fields opposed to being stored in struct.
 *          Skips null arg checking for easier usage.
 * @param   circular_buffer     The circular buffer struct.
 * @return  bool                Whether the circular buffer is empty.
 */
bool circular_buffer_is_empty(circular_buffer_t * circular_buffer) {
    return (circular_buffer->head == circular_buffer->tail);
}

/**
 * @brief   Determine whether the circular buffer is full.
 * @details Is calculated from head and tail fields opposed to being stored in struct.
 *          Skips null arg checking for easier usage.
 * @param   circular_buffer     The circular buffer struct.
 * @return  bool                Whether the circular buffer is full.
 */
bool circular_buffer_is_full(circular_buffer_t * circular_buffer) {
    return ((circular_buffer->head + 1) % circular_buffer->buffer_capacity) == circular_buffer->tail;
}

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
circular_buffer_rc_t circular_buffer_push(circular_buffer_t * circular_buffer, void * txdata, uint8_t size) {
    if(circular_buffer == NULL || txdata == NULL || size != circular_buffer->element_size) {
        return CIRCULAR_BUFFER_RC_BAD_ARG;
    }

    circular_buffer_rc_t rc = CIRCULAR_BUFFER_RC_OK;

    // Overwrite oldest buffer element if overflow occurs by incrementing tail position
    if(circular_buffer_is_full(circular_buffer)) {
        circular_buffer->tail = (circular_buffer->tail + 1) % circular_buffer->buffer_capacity;
        rc = CIRCULAR_BUFFER_RC_OVERFLOW;
    }

    // Push new element to head and increment head position
    uint16_t head_byte_index = circular_buffer->head * circular_buffer->element_size;
    memcpy((circular_buffer->buffer + head_byte_index), txdata, circular_buffer->element_size);
    circular_buffer->head = (circular_buffer->head + 1) % circular_buffer->buffer_capacity;

    return rc;
}

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
circular_buffer_rc_t circular_buffer_pop(circular_buffer_t * circular_buffer, void * rxdata, uint8_t size) {
    if(circular_buffer == NULL || rxdata == NULL || size != circular_buffer->element_size) {
        return CIRCULAR_BUFFER_RC_BAD_ARG;
    }

    // Catch popping from an empty buffer
    if(circular_buffer_is_empty(circular_buffer)) {
        return CIRCULAR_BUFFER_RC_UNDERFLOW;
    }

    // Pop element from circular buffer and increment tail position
    uint16_t tail_byte_index = circular_buffer->tail * circular_buffer->element_size;
    memcpy(rxdata, (circular_buffer->buffer + tail_byte_index), circular_buffer->element_size);
    circular_buffer->tail = (circular_buffer->tail + 1) % circular_buffer->buffer_capacity;

    return CIRCULAR_BUFFER_RC_OK;
}