#include "config.h"
#include "UART_handler.h"
#include "IMU_handler.h"

volatile int total_chars = 0; // Total characters received (for debugging)

// circular buffer for reception
static volatile char uart_buffer[UART_BUFFER_SIZE]; //RX ring buffer
static volatile int uart_head = 0; // Write index (updated by ISR)
static volatile int uart_tail = 0; // Read index (updated by main)

// circular buffer for transmission
static volatile char tx_buffer[TX_BUFFER_SIZE]; // TX ring buffer
static volatile int tx_head = 0;  // Write index (updated by main)
static volatile int tx_tail = 0;  // Read index  (updated by ISR)

// command buffer for parsing
static char command_buffer[UART_COMMAND_BUFFER_SZ]; //stores incoming command
static uint8_t i = 0;       // Index into command_buffer
static int current_hz = 10; // Current $ACC send frequency (default 10 Hz)
static int current_bw = 15; // Current accelerometer bandwidth (default 15 = 1000 Hz)

/* Parser states for incoming UART commands
Splits $BW and $HZ cases to avoid accepting invalid values
 */
typedef enum {
    STATE_WAIT_START,  // Waiting for '$'
    STATE_CMD_TYPE,    // Waiting for 'B' or 'H'
    STATE_B,           // Received 'B', waiting for 'W'
    STATE_H,           // Received 'H', waiting for 'Z'
    STATE_COMMA,       // Waiting for ','
    STATE_DATA1,       // Waiting for first digit
    STATE_DATA2,       // Waiting for second digit
    STATE_END          // Waiting for '*'
} parser_state_t;
static parser_state_t state = STATE_WAIT_START;
// nota: devo dividere i due casi per evitare di ottenere BZ o HW


// UART initialization function
void uart_init(void){
	
    TRISDbits.TRISD0 = 0;               // output D0 -> TX
    TRISDbits.TRISD11 = 1;              // input D11 -> RX
    
    RPINR18bits.U1RXR = UART1_RX_RPIN;  // Map UART1 RX to RPI75 (RD11)
    RPOR0bits.RP64R   = 1;              // Map UART1 TX to RP64  (RD0)
    
    U1STA = 0x00;                       // reset control and status register
    U1MODE = 0x00;                      // reset mode register
    U1BRG = 468 ;                       // Baud rate setting (72000000/16*9600)-1
   // U1BRG = 38; //BAUD - 115200
    
    U1MODEbits.UARTEN = 1;              // Enable UART
    U1STAbits.UTXEN = 1;                // Enable TX
    
    U1STAbits.URXISEL = 0;              //Interrupt is set on every received character
    IFS0bits.U1RXIF   = 0;              // Clear RX interrupt flag
    IEC0bits.U1RXIE   = 1;              // Enable RX interrupt
	
}
  
/* 
UART1 RX interrupt — called every received character, 
saves it in the circular buffer if there's space, otherwise discards it

*/

void __attribute__((interrupt, no_auto_psv))
_U1RXInterrupt(void) {
    IFS0bits.U1RXIF = 0;          // Clear interrupt flag
    char c = U1RXREG;             // Read received character from hardware register
    total_chars++;

    int next = (uart_head + 1) % UART_BUFFER_SIZE;  // Next write position
    if (next != uart_tail) {      // If buffer is not full
        uart_buffer[uart_head] = c;  // Store character
        uart_head = next;            // Advance write index
    }
    // If buffer is full: discard character silently
}


/*
  UART1 TX interrupt — called when U1TXREG is empty, sends next byte from TX circular buffer
  Disables itself when buffer is empty. 
  Replaces while(U1STAbits.UTXBF) that waited for the buffer to be empty,
  allowing non-blocking transmission.

 */

void __attribute__((interrupt, no_auto_psv))
_U1TXInterrupt(void) {
    IFS0bits.U1TXIF = 0;              // Clear interrupt flag

    if (tx_tail != tx_head) {         // If TX buffer has data
        U1TXREG = tx_buffer[tx_tail]; // Send next character
        tx_tail = (tx_tail + 1) % TX_BUFFER_SIZE;  // Advance read index
    } else {
        IEC0bits.U1TXIE = 0;  // Buffer empty: disable TX interrupt
    }
}


//// Send a null-terminated string over UART one character at a time
void uart_send_string(const char *s) {
    while (*s) {          // Loop until null terminator '\0'
        uart_send_char(*s);  // Send one character at a time
        s++;                 // Advance pointer to next character
    }
}



// chat
//void uart_send_string(const char *s) {
//    // copia tutto nel buffer TX senza toccare interrupt
//    while (*s) {
//        int next = (tx_head + 1) % TX_BUFFER_SIZE;
//        if (next == tx_tail) break;  // buffer pieno: scarta
//        tx_buffer[tx_head] = *s;
//        tx_head = next;
//        s++;
//    }
//    // kickstart UNA VOLTA SOLA alla fine
//    IEC0bits.U1TXIE = 0;
//    if (!U1STAbits.UTXBF) {
//        if (tx_tail != tx_head) {
//            U1TXREG = tx_buffer[tx_tail];
//            tx_tail = (tx_tail + 1) % TX_BUFFER_SIZE;
//        }
//    }
//    IEC0bits.U1TXIE = 1;
//}

// Check if RX buffer contains unread characters (returns 1 if available, 0 if empty)
int uart_available(void) {
    IEC0bits.U1RXIE = 0;   // Disable RX interrupt to safely read shared variable
    int head = uart_head;   // Local copy of head
    IEC0bits.U1RXIE = 1;   // Re-enable RX interrupt
    return (head != uart_tail);
}

// Read one character from RX buffer, returns 0 if buffer is empty
char uart_read_char(void) {
    IEC0bits.U1RXIE = 0;    // Disable RX interrupt to safely read shared variable (could use uart_available here )
    int head = uart_head;    // Local copy to avoid race conditions
    IEC0bits.U1RXIE = 1;

    if (head == uart_tail) return 0;  // Buffer empty

    char c = uart_buffer[uart_tail];  // Read oldest character
    
    uart_tail = (uart_tail + 1) % UART_BUFFER_SIZE; 
    // Advance read index 
    //and come back to 0 if we reach the end of the buffer 
    //(ex: uart_tail = 7 → (7+1) % 8 = 0 ) back to the start
    
    return c;
}

/*sends a single character over UART, non-blocking,
 using the TX circular buffer and enabling the TX interrupt*/  

void uart_send_char(char c) {
    int next = (tx_head + 1) % TX_BUFFER_SIZE; // Calculate next write position
    while (next == tx_tail);   // Wait if TX buffer is full 

    tx_buffer[tx_head] = c;    // Store character in TX buffer
    tx_head = next;            // Advance write index          
    
    IEC0bits.U1TXIE = 0;          // Disable TX interrupt to safely check if U1TXREG is empty
    if (!U1STAbits.UTXBF) {       // If hardware TX buffer is empty, 
        // kickstart transmission by writing first character
        
        if (tx_tail != tx_head) { // if buffer has data
            U1TXREG = tx_buffer[tx_tail];  //send first byte
            tx_tail = (tx_tail + 1) % TX_BUFFER_SIZE; // Advance read index
        }
    }
    IEC0bits.U1TXIE = 1;          // Re-enable TX interrupt — ISR will send the rest
}


//chat
//void uart_send_char(char c) {
//    int next = (tx_head + 1) % TX_BUFFER_SIZE;
//    if (next == tx_tail) return;  // buffer pieno: scarta
//    tx_buffer[tx_head] = c;
//    tx_head = next;
//}


/*
  State machine parser — reads RX buffer and detects complete commands
  Valid commands: $BW,xx* and $HZ,yy* (and also values accepted for each)
  return true if a complete valid-format command was received, false otherwise
 */

bool uart_command_buffer(void){  
    bool string_ready = false;

    while(uart_available()){  // While there are characters in the RX buffer
        char c = uart_read_char();

        switch (state){

            case STATE_WAIT_START:
                if (c == '$'){ // Command always starts with '$'
                    state = STATE_CMD_TYPE;
                    i = 0;
                    command_buffer[i] = c;
                    i++;
                }    
            break;
            
            case STATE_CMD_TYPE:
                if (c == 'B'){
                    state = STATE_B;
                    command_buffer[i] = c;
                    i++;
                } 
                else if (c == 'H'){
                    state = STATE_H;
                    command_buffer[i] = c;
                    i++;
                } 
                else {
                    state = STATE_WAIT_START;
                }
            break;

            case STATE_B:
                if (c == 'W') {
                    command_buffer[i] = c;
                    i++;
                    state = STATE_COMMA;
                } 
                else {
                    state = STATE_WAIT_START;
                }
            break;

            case STATE_H:
                if (c == 'Z') {
                    command_buffer[i] = c;
                    i++;
                    state = STATE_COMMA;
                } 
                else {
                    state = STATE_WAIT_START;
                }
            break;

            case STATE_COMMA: 
                if (c == ',') { // Separator between command and data
                    command_buffer[i] = c;
                    i++;
                    state = STATE_DATA1;
                } 
                else {
                    state = STATE_WAIT_START;
                }
            break;

            case STATE_DATA1: 
                if (c >= '0' && c <= '9') { //first digit
                    command_buffer[i] = c;
                    i++;
                    state = STATE_DATA2;
                } 
                else {
                    state = STATE_WAIT_START;
                }
            break;

            case STATE_DATA2:
                if (c >= '0' && c <= '9') { //second digit
                    command_buffer[i] = c;
                    i++;
                    state = STATE_END;
                } 
                else {
                    state = STATE_WAIT_START;
                }
            break;

            case STATE_END:
                if (c == '*') { // '*' marks end of command
                    command_buffer[i] = c;
                    i++;
                    command_buffer[i] = '\0'; // Null-terminate the string for easier processing
                    string_ready = true;
                }
                state = STATE_WAIT_START; // Reset for next command
            break;
        }

        // Safety: reset if buffer is about to overflow
        if (i >= UART_COMMAND_BUFFER_SZ - 1) {
            i = 0;
            state = STATE_WAIT_START;
        }

        if (string_ready) break; // Stop reading — process command first
    }

    return string_ready;
}

/*
  Validate and apply a parsed command from command_buffer
  Sends $ERR,1* on invalid values and return true if command was valid and applied, false otherwise.
  Validates $BW,xx* (8 ≤ xx ≤ 15) and $HZ,yy* (yy ∈ {0,1,2,5,10})
 */

bool uart_validate_command(void) { 

    // Convert ASCII digits to integer value
    // e.g. '1','5' → (1*10)+5 = 15
    // '0' = 48 in ASCII, so '5'-'0' = 5

    int tens  = command_buffer[4] - '0'; // tens digit
    int units = command_buffer[5] - '0'; // units digit

    uint8_t data = tens * 10 + units; 

    if (command_buffer[1] == 'B'){     // $BW command: set bandwidth
        if (data <8 || data > 15){     // Valid range: 8 to 15
            uart_send_string("$ERR,1*");
            return false;}

        current_bw = data;       // Update global variable with new bandwidth setting           
        imu_set_bandwidth(data); // Apply to IMU register
    }
    if (command_buffer[1] == 'H'){     // $HZ command: set frequency
        if (data != 0 && data != 1 && data != 2 && data != 5 && data != 10){
            uart_send_string("$ERR,2*"); // Valid values: 0,1,2,5,10 only
            return false;}
            current_hz = data;
    }
    return true;
}

/*
  Get the current $ACC send frequency
  return Current frequency in Hz (0 means disabled)
  It will be used in main loop to determine how often to send $ACC messages based on user command
 */
int uart_get_hz(void) {
    return current_hz;
}

