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

volatile float duration;
float distance;

void reset_timer();
int get_timer();

char start=(1<<CS12);
char stop=0;

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
	
	//timer
	TCCR1B=start;
	
	//usart
	uart_init();
	io_redirect();
	
    while (1) 
    {
		_delay_ms(1000);
		
		distance=duration*0.034/2.0;
		printf("%f\n",distance);
		PORTC=(1<<PC4);
		_delay_us(10);
		PORTC=0;
    }
}

ISR(INT0_vect){
	if(PIND&(1<<PD0)>0){ //rising edge
		reset_timer();
	}
	if((PIND&(1<<PD0))==0){ //falling edge
		duration=256.0/16*get_timer();
	}
}

void reset_timer(){
	TCNT1H=0;
	TCNT1L=0;
	TCCR1B=start;
	
}

int get_timer(){
	int output=0;
	output|=(uint8_t)TCNT1H<<8;
	output|=(uint8_t)TCNT1L;
	TCCR1B=stop;
	return output;
}