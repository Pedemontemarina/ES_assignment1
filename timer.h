#ifndef TIMER_H
#define TIMER_H

#include <xc.h>
#include <stdint.h>

#define TIMER1 1
#define TIMER2 2
#define TIMER3 3

/* extern volatile int run_flag;
extern volatile int num_int; */

void timer_init(void);

void tmr_setup_period(int timer, int ms);
int tmr_wait_period(int timer);
void tmr_wait_ms(int timer, int ms);

#endif