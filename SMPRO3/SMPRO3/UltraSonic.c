/*
 * UltraSonic.c
 *
 * Created: 14.12.2021 10:47:21
 *  Author: vince
 */ 

#ifndef F_CPU
#define F_CPU 16000000
#endif

#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "time.h"
#include "UltraSonic.h"


//global variables

// Timer values for TIMER 1
char timer1_start=(1<<CS12);
char timer1_stop=0;

//index to currently trigger ultra sonic s.
volatile int ultra_index=0;
//next ultrra sonic sensor which relly needs to be read next
volatile char request_ultra_index=-1;

//ultrasonic data array
volatile timer_value_t timer_values[ULTRA_NUMBER];
volatile char ultra_trigger_mask=0xFF;
volatile int ultra_error=0;
volatile int ultra_timer_error=0;

//INIT
void init_ultra_trigger(void){
	//Triger ultra sonic pins
	DDRF|=0b00011111;
	PORTF&=~(0b00011111);
	
	_delay_ms(100);
	
	// use timer 3 as pwm generator for ultrasonic sensors
	//the timer will count up and reset at TOP
	// no pin output just interrupts
	TCCR3A=(1<<WGM31)|(1<<WGM30);
	TCCR3B=(1<<WGM33)|(1<<WGM32)|(1<<CS30)|(1<<CS31);
	//Top value (25002) in OCR3A:
	OCR3AH=(uint8_t)(25002>>8);
	OCR3AL=(uint8_t)(25002);
	//Compare interrupt at OCRB=24999
	OCR3BH=(uint8_t)(24999>>8);
	OCR3BL=(uint8_t)(24999);
	//enable compare b interrupt and overflow interrupt
	TIMSK3=(1<<OCIE3B)|(1<<TOIE3);
	
	sei();
}

void init_ultra_read(void){
	DDRK&=~(0b0001111); //echo pins are connected here
	PORTK&=~(0b0001111); //No pull ups
	
	if(get_time_ms()==0){
		time_init();
	}
	
	//Initializing Timer Value array
	for(int i=0;i<ULTRA_NUMBER;i++){
		timer_values[i].timer_value=0;
		timer_values[i].time_stamp=0;
	}
	
	//ultrasonic get echo timer 1
	//leave TCCR1A as it is
	//TCCR1B is set through start and stop
	//timer1_start=(1<<CS12);
	//timer1_stop=0;
	TIMSK1=(1<<TOIE1);
	
	//External interrupts
	//echo pins for ultra sonic sensor
	PCMSK2=0b00011111; //PCINT16-20 pin change interrupt
	PCICR|=(1<<PCIE2);//enable pin change interrupt 2
	
	sei();  // enable global interrupts
	
	// interrupt control panel
	DDRL|=0x01;
	PORTL&=~(0x01);
	
	sei();
}

//INTERRUPTS
//it is where all the magic happens

//Triggered on any edge on ECHOs PORT
ISR(PCINT2_vect){
	PORTL|=0x01;//intterupt controll panel
	if(PINK==0){ // one pin must have had a falling edge
		timer_values[ultra_index].timer_value=get_and_stop_timer();
		timer_values[ultra_index].time_stamp=get_time_ms();			//TODO maybe this is too slow
		if(request_ultra_index<0 || request_ultra_index>=ULTRA_NUMBER){
			ultra_index++;
			if(ultra_index>=ULTRA_NUMBER){
				ultra_index=0;
			}
		}else{
			ultra_index=request_ultra_index;
			request_ultra_index=-1;
		}
	}else{//some pin must have had a rising edge
		if(TCCR1B!=timer1_stop){//Error detection
			ultra_timer_error++;
		}
		reset_timer();
		//error detection
		char index=0;
		switch(PINK){
			case 1:
			index=0;
			break;
			case 2:
			index=1;
			break;
			case 4:
			index=2;
			break;
			case 8:
			index=3;
			break;
			case 16:
			index=4;
			break;
			default:
			index=-1;
		}
		if(index!=ultra_index){
			ultra_error++;
		}
	}
	PORTL&=~(0x01); // interrupt controll panel
}


//To spot Errors
ISR(TIMER1_OVF_vect){
	ultra_timer_error++;
}

//PWM interrupts
volatile char index_buffer=0; //buffer index, so it is not change during interrupt

ISR(TIMER3_COMPB_vect){
	//Trigger pin
	index_buffer=ultra_index;
	PORTF=(1<<index_buffer);
}

ISR(TIMER3_OVF_vect){
	//release pin
	PORTF=0x00;
	
}

//FUNCTIONS

void reset_timer(void){
	cli();
	TCNT1H=0x00;
	TCNT1L=0x00;
	TCCR1B=timer1_start;
	sei();
}


int get_and_stop_timer(void){
	cli();
	TCCR1B=timer1_stop;
	/*int output=0;
	output=(TCNT1H<<8)|TCNT1L;*/
	//return output;
	int output=(uint16_t)TCNT1;
	sei();
	return output;
}

int get_timer_value(void){
	/*int output=0;
	output|=((uint8_t)TCNT1H)<<8;
	output|=(uint8_t)TCNT1L;
	return output;*/
	return (uint16_t)TCNT1;
}

int read_timer_value(int index){
	float diff_time=get_time_ms()-timer_values[index].time_stamp;
	if(diff_time>OLD_VALUE_THRESHOLD){
		int tmp=timer_values[index].timer_value;
		request_ultra_index=index;
		while(timer_values[index].timer_value==tmp){
			} //wait until value changes
		return timer_values[index].timer_value;
	}else{
		return timer_values[index].timer_value;
	}
}

void trigger_ultra(int index){
	PORTF=(0x01<<index);
	_delay_us(10);
	PORTF=0;
}

void print_all_timer_values(void){
	//Ultrasonic 0:		1424	updated
	for(int i=0;i<ULTRA_NUMBER;i++){
		printf("Ultrasonic %d: \t%d \tTime Stamp:%f\n",i,timer_values[i].timer_value,timer_values[i].time_stamp);
	}
	printf("Index: %d\n",ultra_index);
	printf("Errors: %d\n",ultra_error);
	printf("Timer Errors: %d\n",ultra_timer_error);
	
}