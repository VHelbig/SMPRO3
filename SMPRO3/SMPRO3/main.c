/*
 * SMPRO3.c
 *
 * Created: 25.10.2021 10:54:38
 * Author : vince
 */ 

//DEFINES
#define F_CPU 16000000
//ULtrasonic 
#define ULTRA_NUMBER 5	//number of ultrasonic sensors
#define OLD_VALUE_THRESHOLD 500
//Main
#define IR_THRESHOLD (50)

//INCLUDES
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "i2cmaster.h"	//i2c
#include "usart.h"		//usart library from alin also works on arduino mega
#include "PCA9685.h"	//motorcontrol board
#include "time.h"
#include "UltraSonic.h"

//STRUCTS
typedef struct{
	float x;
	float y;
}vector_t;



//GLOBAL VARIABLES

vector_t robot;	//always 0
vector_t aim;	//ultimate Aim
vector_t inter; //intermediate aim
vector_t nose;	//points to the nose of the robot
float nose_deg;	//degrees from starting position
char followCornerMode=0;

//define in UltraSonic.c:
/*//ultrasonic data array
volatile timer_value_t timer_values[ULTRA_NUMBER];
//next ultrra sonic sensor which relly needs to be read next
volatile int ultra_error=0;
volatile int ultra_timer_error=0;
*/

// optocoupler counters
volatile int optocounter1=0;
volatile int optocounter2=0;



int main(void)
{
	//ultrasonic
	init_ultra_trigger();
	init_ultra_read();
	
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
	
	//Main algorithm
	robot.x=0;
	robot.y=0;
	aim.x=50;
	aim.y=20;
	nose.x=0;
	nose.y=1;
	nose_deg=0;
	
    while (1) 
    {
		//generate Trig signal
		//trigger_ultra(ultra_index);
		print_all_timer_values();
		//printf("%d\n",optocounter);
		_delay_ms(500);
		
		
    }
}


//optocoupler:
ISR(INT4_vect){
	optocounter1++;
	
}

ISR(INT5_vect){
	optocounter2++;
}