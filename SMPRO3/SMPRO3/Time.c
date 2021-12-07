/*
 * time.c
 *
 * Created: 07.12.2021 10:00:30
 *  Author: vince
 */ 

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "time.h"

void time_init(){
	//leave TCCR4A as it is
	//the interrupt should trigger as little as possible while having a good enough accuracy
	//1024 -> 64us update rate -> below 1 ms -> good enough
	TCCR4B=(1<<CS42)|(1<<CS40);//1024 prescaler
	//leave TCCR4C as it is
	TIMSK4=(1<<TOIE4); // enable overflow interrupt
	sei();
}

ISR(TIMER4_OVF_vect){
	time_ms=time_ms+4194.24;
}


float get_time_ms(){
	return time_ms+((uint16_t)TCNT4)*(64E-6);
}

float get_time_s(){
	time_s=time_ms/1000+((uint16_t)TCNT4)*(64E-9);
	return time_s;
}

float get_time_min(){
	time_s=time_ms/1000+((uint16_t)TCNT4)*(64E-9);
	time_min=time_s/60.0;
	return time_min;
	
}
float get_time_h(){
	time_s=time_ms/1000+((uint16_t)TCNT4)*(64E-9);
	time_min=time_s/60.0;
	time_h=time_min/60.0;
	return time_h;
}