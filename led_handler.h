#ifndef LED_HANDLER_H
#define LED_HANDLER_H

#include <xc.h>
#include <stdint.h>

void led_init(void);
void led_toggle_ld1(void);
void led_toggle_ld2(void);
void led_set_ld2(int state);

#endif
