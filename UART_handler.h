#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h> 

extern volatile int total_chars; 

void uart_init(void);

void uart_send_char(char c);
void uart_send_string(const char *s);

// Circular buffer to get user commands
#define UART_BUFFER_SIZE 64 // emptied by command buffer

int uart_available(void);
char uart_read_char(void);

#define UART_COMMAND_BUFFER_SZ 32 // contains only correctly formatted commands, emptied by command processor
// Max command length: $XX,dd* (7 chars) + null terminator.

#define TX_BUFFER_SIZE   64  // contains data from imu to UART, emptied by TX ISR
bool uart_command_buffer(void);
bool uart_validate_command(void);
int uart_get_hz(void);

#endif
