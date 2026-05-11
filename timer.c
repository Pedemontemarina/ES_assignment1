#include "timer.h"

// Initializes timers and enables global interrupts

void timer_init(void)
{
    INTCON2bits.GIE = 1; // Enable global interrupts
    
    IFS0bits.T1IF = 0;   // Clear Timer1 interrupt flag
    IEC0bits.T1IE = 0;   // Disable Timer1 interrupt 

    IFS0bits.T2IF = 0;   // Clear Timer2 interrupt flag
    IEC0bits.T2IE = 0;   // Disable Timer2 interrupt 

    IFS0bits.T3IF = 0;   // Clear Timer3 interrupt flag
    IEC0bits.T3IE = 0;   // Disable Timer3 interrupt 

}

// tmr_setup_period
// Configures and starts the given timer to fire after `ms`
// milliseconds, using a 1:256 prescaler on a 72 MHz clock.
//
// Formula: PR = (ms * (FCY / prescaler)) / 1000
//          with FCY = 72,000,000 and prescaler = 256
//
// Parameters:
//   timer - TIMER1, TIMER2, or TIMER3
//   ms    - desired period in milliseconds

void tmr_setup_period(int timer, int ms){ 

    switch(timer){

        case TIMER1:
            TMR1=0; // Reset timer counter
            PR1 = (ms * (72000000LL/256)) / 1000; // Set period register
            T1CONbits.TCKPS = 3;   // prescaler 1:256
            IFS0bits.T1IF = 0;     // reset flag
            T1CONbits.TON = 1; //starts the timer
            break;

        case TIMER2:
            TMR2 = 0;
            PR2 = (ms * (72000000LL/256)) / 1000;
            T2CONbits.TCKPS = 3;
            IFS0bits.T2IF = 0;
            T2CONbits.TON = 1;
            break;  
        
        case TIMER3:
            TMR3 = 0;
            PR3 = (ms * (72000000LL/256)) / 1000;
            T3CONbits.TCKPS = 3;
            IFS0bits.T3IF = 0;
            T3CONbits.TON = 1;
            break;  
            
    }
}

// tmr_wait_period
// Blocks until the current timer period expires (flag set).
// If the flag was ALREADY set on entry, it means the previous
// period was missed (deadline overrun) — returns 1 in that case.
// Returns 0 if the period completed normally (no overrun).
//
// Parameters:
//   timer - TIMER1, TIMER2, or TIMER3
// Returns:
//   1 if deadline was missed (timer already expired on entry)
//   0 if timing was met correctly

int tmr_wait_period(int timer){
    int ret = 0;
    switch(timer){

        case TIMER1:
            if (IFS0bits.T1IF == 1){ // Check if flag was already set = deadline missed
                ret = 1;
                IFS0bits.T1IF = 0;} // Clear flag before re-waiting
            while(!IFS0bits.T1IF); // Busy-wait until the period expires
                IFS0bits.T1IF = 0; // Clear the interrupt flag
            break;

        case TIMER2:
            if (IFS0bits.T2IF == 1){
                ret = 1;
                IFS0bits.T2IF = 0;}
            while(!IFS0bits.T2IF); 
                IFS0bits.T2IF = 0;
            break;
            
        case TIMER3:
            if (IFS0bits.T3IF == 1){
                ret = 1;
                IFS0bits.T3IF = 0;}
            while(!IFS0bits.T3IF); 
                IFS0bits.T3IF = 0;
            break;

    }
   return ret;
}


// tmr_wait_ms
// Blocks for exactly `ms` milliseconds using the given timer.
// Since PR registers are 16-bit (max ~233 ms at 72 MHz/256),
// long delays are split into 200 ms chunks to avoid overflow.
//
// Parameters:
//   timer - TIMER1, TIMER2, or TIMER3
//   ms    - total delay in milliseconds

void tmr_wait_ms(int timer, int ms){
    
    switch(timer){
        
        case TIMER1:
            tmr_setup_period(TIMER1,200); // Start with a 200 ms period
            
            // Wait in 200 ms chunks as long as remaining time exceeds 200 ms
            while(ms > 200){
                while(!IFS0bits.T1IF); // Wait for 200 ms to elapse
                IFS0bits.T1IF = 0; // Clear flag
                ms = ms-200; // Decrease remaining time by 200 ms
            }
            // Wait for the remaining sub-200 ms portion (if any)
            if(ms>0){
                tmr_setup_period(TIMER1,ms);
                        
                while(!IFS0bits.T1IF); 
                IFS0bits.T1IF = 0;   
            }
            break;
             
        case TIMER2:
            tmr_setup_period(TIMER2,200);
            while(ms > 200){
                   
                while(!IFS0bits.T2IF); 
                IFS0bits.T2IF = 0;  
                
                ms = ms-200;}
            
            if(ms>0){
                tmr_setup_period(TIMER2,ms);
                        
                while(!IFS0bits.T2IF); 
                IFS0bits.T2IF = 0;      
            }  
            break;
            
        case TIMER3:
            tmr_setup_period(TIMER3,200);
            while(ms > 200){
                   
                while(!IFS0bits.T3IF); 
                IFS0bits.T3IF = 0;  
                
                ms = ms-200;}
            
            if(ms>0){
                tmr_setup_period(TIMER3,ms);
                        
                while(!IFS0bits.T3IF); 
                IFS0bits.T3IF = 0;      
            }  
            break;
            
    }
    

}
