/**
 * @file main.c
 * @author facci
 * @date 2026-04-30
 * @brief Main function
 */
/* 
1/ Simulate an algorithm that needs 7 ms for its execution and needs to work at 100 Hz.
This is to emulate a real-world scenario.
2/ Make LD2 blink at 1 Hz (500 on, 500 off).
3/ Read characters from UART1. If a string $BW,xx* is received, the xx dictates the bandwidth of the accelerometer
 filter. Valid data for xx are 8 to 15. Invalid values (e.g., -1, 3.2, 18) should be discarded and a message 
 $ERR,1* should be sent on the UART. The initial bandwidth is 1000 Hz (value 15, see page 58). If a string $HZ,yy*
is received, the yy dictates the frequency of the data sent over the UART. Valid data for xx are 0, 1, 2, 5, 10.
A value of 0 disables the message. Invalid values should be discarded and a message $ERR,1* should be sent on the
UART. The initial frequency is 10 Hz.
4/ Acquire the three accelerometer axes at 50 Hz.
5/ Compute the roll and pitch angles
6/ Send it to the UART at yy Hz using the protocol $ACC,x,y,z*.
7/ Send the computed angles at 5 Hz using the message $ANG,x,y*, where x is the roll angle in degrees 
and y is the pitch angle in degrees.
 */


#include "config.h"
#include "timer.h"
#include "UART_handler.h"
#include "led_handler.h"
#include "SPI_handler.h"
#include "IMU_handler.h"

void setup() {
    // set all pins to digital
    ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000; 

	led_init();       // Configure LED pins (TRIS and LAT)
    uart_init();      // Configure baud rate, pins, and UART interrupts
    spi_init_pins();  // Configure MISO, MOSI, SCK, and CS pins
    imu_setup();      // Wake up IMU from sleep mode
    imu_init();       // Configure IMU bandwidth, range, etc.
    timer_init();     // Initial timer configurations
}


int main(void) {
    setup();

    int hz= uart_get_hz(); // Get initial frequency setting for UART transmission

    // Struct to hold raw accelerometer data (x, y, z as int16_t)
    accel_data_t accel = {0};

    // Struct to hold computed angles (roll and pitch as float)
    angle_data_t angles = {0.0f, 0.0f};

    // Cycle counter used to derive multiple frequencies from one loop
    int count = 0;
    
    // Counts how many times the loop exceeded the 10ms deadline
    int missed_deadlines = 0;

    //main loop with 100 Hz frequency
    tmr_setup_period(TIMER1, 10);  

    while (1) {
        // Simulate a 7ms algorithm using TIMER2
        tmr_wait_ms(TIMER3, 7);

        // Toggle LD2 every 500ms (50 cycles × 10ms) → blinks at 1 Hz
        if (count % 50 == 0) led_toggle_ld2();
        
        // Every 2 cycles × 10ms = 20ms → read accelerometer at 50 Hz
        if (count % 2 == 0) {
            imu_read_acc(&accel);             // Read X, Y, Z via SPI  
        }

         // Send $ACC at yy Hz, the user-configured frequency (0, 1, 2, 5, 10)
        if (hz > 0) {   // hz = 0 means transmission is disabled
            if (count % (100 / hz) == 0) {
                char msg[48];
                sprintf(msg, "$ACC,%d,%d,%d*", accel.x, accel.y, accel.z);
                uart_send_string(msg);
            }
        }

        // Send $ANG every 200ms (20 cycles × 10ms) → fixed at 5 Hz
        if (count % 20 == 1) {
            imu_roll_pitch(&accel, &angles);  // Compute roll and pitch from accel
            char msg[48];
            // %.2f → float with 2 decimal places, e.g. "$ANG,12.34,-5.67*"
            sprintf(msg, "$ANG,%.2f,%.2f*", angles.roll, angles.pitch);
            uart_send_string(msg);
        }

        // // Check if a complete command is in the UART buffer
        if (uart_command_buffer()) {
            // Parse command ($BW or $HZ), validate value, update settings or send $ERR,1* if invalid
            uart_validate_command();
            hz = uart_get_hz(); // Update hz in case it was changed by a valid $HZ command
        }
        // Increment cycle counter
        count++;

        // Wait for TIMER1 to expire (end of 10ms period)
        // If we missed the deadline, toggle LD1 as an alert
        
        if (tmr_wait_period(TIMER1)) {
            missed_deadlines++;
            led_toggle_ld1();  
}
    }
    return 0;
}

