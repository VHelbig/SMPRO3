/*
 * SMPRO3.c
 *
 * Created: 25.10.2021 10:54:38
 * Author : vince
 */ 


#define F_CPU 16000000
#define ULTRA_NUMBER 4	//number if ultrasonic sensors

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "i2cmaster.h"	//i2c
#include "usart.h"		//usart library from alin also works on arduino mega
#include "PCA9685.h"	//motorcontrol board

//STRUCTURES
typedef struct{
	int timer_value;
	char updated;		//replace this with timestamp later
}timer_value_t;			//struct for keeping track of all timer values and if they are up to date

//GLOBAL VARIABLES

//ultrasonic data array
volatile timer_value_t timer_values[5];
//index to currently trigger ultra sonic s.
volatile int ultra_index=0;

// optocoupler counters
volatile int optocounter1=0;
volatile int optocounter2=0;

// Timer values for TIMER 1
char start=(1<<CS12);
char stop=0;


//FUNCTION PROTOTYPES
void reset_timer(void);		//just reset timer1
int get_timer_value(void);	//just get timer1 value 
int get_and_stop_timer(void); // get timer value and stop timer1
void trigger_ultra(int index);	//triggers ultrasonic sensor not recommended, as it blocks the execution for 10us
void print_all_timer_values(void);	//prints all ultrasonic sensor values and their update status
int read_timer_value(int index);	//function for getting ultra sonic sensor value from array, wont be needed later


int main(void)
{
	//Initializing Timer Value array
	for(int i=0;i<ULTRA_NUMBER;i++){
		timer_values[i].timer_value=0;
		timer_values[i].updated=0;
	}
	
	//Triger ultra sonic pins
    DDRF=0xFF;
	PORTF=0x00;
	
	DDRD=0x00; //Echo all is connected here
	PORTD=0x00; // No pull ups
	
	// use timer 3 as pwm generator for ultrasonic sensors
	//the timer will count up and reset at TOP
	// no pin output just interreupts
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
	
	
	//External interrupts
	//echo all
	EICRA|=(1<<ISC20); // Enable external interrupt 0 on any edge
	EIMSK|=(1<<INT2);
	//optocaupler	
	EICRB|=(1<<ISC41)|(1<<ISC40);//rising edge on int4
	EIMSK|=(1<<INT4);
	EICRB|=(1<<ISC51)|(1<<ISC50);//rising edge on int5
	EIMSK|=(1<<INT5);
	
	
	sei();  // enable global interrupts
	
	
	//usart
	uart_init();
	io_redirect();
	
	//i2c 
	i2c_init();
	motor_init_pwm(PWM_FREQUENCY_50);
	
    while (1) 
    {
		//generate Trig signal
		//trigger_ultra(ultra_index);
		//print_all_timer_values();
		
		//printf("%d\n",optocounter);
		_delay_ms(500);
		
		
    }
}

//Triggered on any edge on ECHO
ISR(INT2_vect){
	if((PIND&(1<<PD2))>0){ //rising edge
		reset_timer();
	}
	if((PIND&(1<<PD2))==0){ //falling edge
		timer_values[ultra_index].timer_value=get_and_stop_timer();
		timer_values[ultra_index].updated=1;
		ultra_index+=1;
		if(ultra_index>=ULTRA_NUMBER){
			ultra_index=0;
		}
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

void trigger_ultra(int index){
	PORTF=(0x01<<index);
	_delay_us(10);
	PORTF=0;
}

void print_all_timer_values(void){
	//Ultrasonic 0:		1424	updated
	for(int i=0;i<ULTRA_NUMBER;i++){
		printf("Ultrasonic %d: \t%d \t",i,timer_values[i].timer_value);
		if(timer_values[i].updated>0){
			printf("updated\n");
			timer_values[i].updated=0;
		}else{
			printf("not updated\n");
		}
	}
	printf("Index: %d\n",ultra_index);
	
}

//PWM interrupts
volatile char index_buffer=0; //buffer index, so it is not change during interrupt

ISR(TIMER3_COMPB_vect){
	//Trigger pin
	index_buffer=ultra_index;
	PORTF=(1<<index_buffer);
	
	ultra_index++;
	if(ultra_index>=ULTRA_NUMBER){
		ultra_index=0;
	}
}

ISR(TIMER3_OVF_vect){
	//release pin
	PORTF=0x00;
	
}


int read_timer_value(int index){
	timer_values[index].updated=0;
	return timer_values[index].timer_value;
}

//optocoupler:

ISR(INT4_vect){
	optocounter1++;
	
}

ISR(INT5_vect){
	optocounter2++;
}