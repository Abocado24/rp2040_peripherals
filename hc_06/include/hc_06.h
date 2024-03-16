#ifndef HC_06_H
#define HC_06_H

// Includes
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/sync.h"
#include "hardware/irq.h"

// Defines
#define HC06_MSG_SIZE               250
#define HC06_DEFAULT_TIMEOUT_MS     2000

// Struct Definitions
typedef struct {
    char buffer[HC06_MSG_SIZE];
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

    // Store current tx data in circular buffer for non-blocking send
    circular_buffer_t tx_buffer;
    // Store current rx data in circular buffer for non-blocking receive
    circular_buffer_t rx_buffer;

    // Flag to indicate that a complete message has been sent
    volatile bool message_sent;
    // Flag to indicate that a complete message has been received
    volatile bool message_received;
} hc06_t;

// Function declarations
void hc06_init(hc06_t *hc06, uart_inst_t *uart_id, uint32_t baud_rate, uint8_t uart_tx_pin, uint8_t uart_rx_pin, uint8_t uart_irq);
bool hc06_send_message_blocking(hc06_t* hc06, const char* message, uint32_t timeout_ms);
uint8_t hc06_send_message_non_blocking(hc06_t* hc06, const char* msg, uint8_t len);
uint8_t hc06_receive_message_blocking(hc06_t* hc06, char buf[], uint8_t len, uint32_t timeout_ms);
uint8_t hc06_receive_message_non_blocking(hc06_t *hc06, char buf[], uint8_t len);
bool hc06_message_sent(hc06_t *hc06);
bool hc06_message_received(hc06_t *hc06);

#endif // HC_06_H
