#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h> 

extern volatile int total_chars; // Total characters received (for debugging)

void uart_init(void);

void uart_send_char(char c);
void uart_send_string(const char *s);

int uart_available(void);
char uart_read_char(void);

bool uart_command_buffer(void);
bool uart_validate_command(void);
int uart_get_hz(void);

// Circular buffer to get user commands
#define RX_BUFFER_SIZE 32 // emptied by command buffer max ~10 char/ciclo for 9600 baud

#define UART_COMMAND_BUFFER_SZ 32 // contains only correctly formatted commands, emptied by command processor
// Max command length: $XX,dd* (7 chars) + null terminator.

#define TX_BUFFER_SIZE   64  // contains data from imu to UART, emptied by TX ISR

// Circular buffer struct — used for both RX and TX buffers.
// head: index where next character will be written by ISR (RX) or main code (TX)
// tail: index where next character will be read by main code (RX) or ISR (TX)
// size: actual buffer size

typedef struct {
    volatile char buf[64];  //max sixe
    volatile int  head;     
    volatile int  tail;     
    int           size;     // dimention used at runtime (RX_BUFFER_SIZE or TX_BUFFER_SIZE)
} circular_buffer_t;



#endif
