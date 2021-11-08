/*
 * SMPRO3.c
 *
 * Created: 25.10.2021 10:54:38
 * Author : vince
 */ 

//Trig is connected to PC4
//Echo is connected to PD0 

#define F_CPU 16000000

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "usart.h"

volatile int duration;
volatile char updated=0;
float distance;

void reset_timer(void);
int get_timer_value(void);
int get_and_stop_timer(void);

// Timer values
char start=(1<<CS12);
char stop=0;

float values[3]={0,0,0};

int main(void)
{
    DDRC|=(1<<PC4)|(1<<PC5);
	PORTC=0;
	DDRD=0; //all input
	PORTD=0x00;
	
	
	//interrupts
	EICRA|=(1<<ISC00);
	EIMSK|=(1<<INT0);
	sei();
	
	
	//usart
	uart_init();
	io_redirect();
	
	char counter=0;
    while (1) 
    {
		//generate Trig signal
		PORTC=(1<<PC4);
		_delay_us(10);
		PORTC=0;
		
		//read value
		if(updated==1){
			printf("%d\t",duration);
			// result = timervalue* 256/16000000 * 343m/s * 100cm *1/2
			float result=duration*0.2744;
			printf("%f\n",result);
			updated=0;
		}
		_delay_ms(500);
    }
}

ISR(INT0_vect){
	if((PIND&(1<<PD0))>0){ //rising edge
		reset_timer();
	}
	if((PIND&(1<<PD0))==0){ //falling edge
		duration=get_and_stop_timer();
		updated=1;
	}
}

void reset_timer(void){
	TCNT1H=0x00;
	TCNT1L=0x00;
	TCCR1B=start;
	
}


int get_and_stop_timer(void){
	TCCR1B=stop;
	int output=0;
	output|=((uint8_t)TCNT1H)<<8;
	output|=(uint8_t)TCNT1L;
	return output;
}
int get_timer_value(void){
	int output=0;
	output|=((uint8_t)TCNT1H)<<8;
	output|=(uint8_t)TCNT1L;
	return output;
}