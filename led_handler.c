#include "config.h"
#include "led_handler.h"


//Initialize LD1 and LD2 as digital outputs, both off
// for definitions of LD1_TRIS, LD1_LAT, LD2_TRIS, and LD2_LAT, see config.h
void led_init(void) {
	
	LD1_TRIS = 0;   // output led1
    LD1_LAT = 0;    // LED turned off

    LD2_TRIS = 0;  // output led2
    LD2_LAT = 0;   // LED turned off
	
}

void led_toggle_ld1(void) {
    LD1_LAT = !LD1_LAT;   // change the state of LD1
}

void led_toggle_ld2(void) {
    LD2_LAT = !LD2_LAT;  // change the state of LD2
}

// Set LD2 to the specified state (0 for off, 1 for on)
void led_set_ld2(int state) {
    LD2_LAT = state;     
}

