#ifndef HC_06_H
#define HC_06_H

// Includes
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/sync.h"
#include "hardware/irq.h"

// Defines
#define BUF_SIZE 250

// Struct Definitions
typedef struct {
    char buffer[BUF_SIZE];
    volatile uint8_t head;
    volatile uint8_t tail;
} circular_buffer_t;

typedef struct {
    // UART defines
    uart_inst_t * uart_id;
    uint32_t baud_rate;
    uint8_t uart_tx_pin;
    uint8_t uart_rx_pin;
    uint8_t uart_irq;

    // Store rx data in circular buffer
    circular_buffer_t rx_buffer;
} hc06_t;

// Function declarations
void hc06_init(hc06_t *hc06, uart_inst_t *uart_id, uint32_t baud_rate, uint8_t uart_tx_pin, uint8_t uart_rx_pin, uint8_t uart_irq);
bool hc06_rx_buffer_empty(hc06_t *hc06);
void hc06_uart_rx(void);
uint8_t hc06_transfer_rx_buffer(hc06_t *hc06, char buf[], uint8_t buf_len);

// Transfer data from RX buffer to a user-provided buffer
// Returns number of bytes transferred
uint8_t hc06_transfer_rx_buffer(hc06_t *hc06, char buf[], uint8_t buf_len);

#endif // HC_06_H
