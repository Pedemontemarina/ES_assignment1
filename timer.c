#include "timer.h"

/* volatile int run_flag = 0; 
volatile int num_int = 0; */


void timer_init(void)
{
	// enable interrupts timer1---------------------------------
    INTCON2bits.GIE = 1; /// da verificare
    
    IFS0bits.T1IF = 0;   // azzera flag Timer1
    IEC0bits.T1IE = 0;

}


void tmr_setup_period(int timer, int ms){ 

    switch(timer){

        case TIMER1:
            TMR1=0;
            PR1 = (ms * (72000000LL/256)) / 1000;
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
            
    }
}

int tmr_wait_period(int timer){
    int ret = 0;
    switch(timer){

        case TIMER1:
            if (IFS0bits.T1IF == 1){
                ret = 1;
                IFS0bits.T1IF = 0;}
            while(!IFS0bits.T1IF); 
                IFS0bits.T1IF = 0;
            break;

        case TIMER2:
            if (IFS0bits.T2IF == 1){
                ret = 1;
                IFS0bits.T2IF = 0;}
            while(!IFS0bits.T2IF); 
                IFS0bits.T2IF = 0;
            break;

    }
   return ret;
}

void tmr_wait_ms(int timer, int ms){
    
    switch(timer){
        
        case TIMER1:
            tmr_setup_period(TIMER1,200);
            while(ms > 200){
                       
                while(!IFS0bits.T1IF); 
                IFS0bits.T1IF = 0;
                ms = ms-200;
            }
            
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
    }

}
