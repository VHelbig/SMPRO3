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
#define OLD_VALUE_THRESHOLD 200
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
float nose_deg=0;	//degrees from starting position
char followCornerMode=0;

//define in UltraSonic.c:
/*//ultrasonic data array
volatile timer_value_t timer_values[ULTRA_NUMBER];
//next ultrra sonic sensor which relly needs to be read next
volatile int ultra_error=0;
volatile int ultra_timer_error=0;
*/

// optocoupler counters
volatile int optocounterL=0;
volatile int optocounterR=0;

//Mot pwm values
int motor_left=0;
int motor_right=0;

//current sensor
volatile float lastmeasure=0;
volatile float voltage=0;
volatile float current=0;
//volatile float total_energy=0;
volatile float amphours=0;
//volatile char adc_index=5;

//PROTOTYPES
void turn_robot(float degrees);					//turns robot a certain angle and updates nose vector
void AlignRobotToPoint(vector_t  *point);		//Alligns robot to point in coordinate system
void MoveABit(vector_t *point);					//Moves robot ca. 7 cm straightly forward 
void MoveToPoint(vector_t *point);				//Allginrobot and 
void follow_corner(void);

void motpwm_setLeft(int speed);
void motpwm_setRight(int speed);
vector_t SetInterAim(vector_t aim, float seed);
void led_setall(char l1,char l2,char l3,char l4,char rl5);
char CloseTo(vector_t aim);
char isFrontClear(void);

uint16_t adc_read(uint8_t adc_channel);



int main(void)
{
	//ultrasonic
	init_ultra_trigger();
	init_ultra_read();
	
	DDRE&=~((1<<PE5)|(1<<PE4));
	PORTE&=~((1<<PE5)|(1<<PE4));
	
	DDRC=0x1F;
	PORTC=0x00;
	
	
	//optocaupler	
	EICRB|=(1<<ISC41)|(1<<ISC40);//rising edge on int4
	EIMSK|=(1<<INT4);
	EICRB|=(1<<ISC51)|(1<<ISC50);//rising edge on int5
	EIMSK|=(1<<INT5);
	
	//Current sensor
	DDRF = 0b00011111;// inputs of the ADC PF7..5
	PORTF = 0x00; //NO PULL-UPS
	DDRC = 0xFF;//LEDs Outputs
	PORTC = (1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3)|(1<<PC4);//All LEDs ON
	
	uint16_t adc_result;
	
	//select Vref = Aref and AVcc turn off
	ADMUX = (0<<REFS0)|(0<<REFS1);
	ADCSRB = (0<<MUX5);
	//set prescaler to 128 and turn on the ADC module
	ADCSRA = (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN)|(1<<ADIE)|(1<<ADSC);
	//leave ADCSRB as it is
	DIDR0|=(1<<ADC5D)|(1<<ADC6D); //disable digital input on A5 and A6
	
	
	sei();  // enable global interrupts
	
	//usart
	uart_init();
	io_redirect();
	printf("test");
	
	//i2c 
	i2c_init();
	motor_init_pwm(PWM_FREQUENCY_200);
	//Main algorithm
	robot.x=0;
	robot.y=0;
	aim.x=-100;
	aim.y=200;
	nose.x=0;
	nose.y=1;
	nose_deg=0;
	//straight:
	//motpwm_setLeft(1000);
	//motpwm_setRight(1065);
	
	//turn
	//motpwm_setLeft(1000);
	//motpwm_setRight(-1000);
	
	_delay_ms(500);
	
	//followCornerMode=1;
    while (1) 
    {
		
		//MoveToPoint(&aim);
		
		//generate Trig signal
		//trigger_ultra(ultra_index);
		
		//printf("%d\n",optocounter);
		//print_all_timer_values();
		
		//AlignRobotToPoint(&aim);
		
		//_delay_ms(100);
		//300 gut 
		//printf("v:%d\n",read_timer_value(0));
		
		if(followCornerMode){
			follow_corner();
		}else{
			if(CloseTo(aim)==0){
				if(CloseTo(inter)){
					inter.x=0;
					inter.y=0;
				}
				if(inter.x==0 && inter.y==0){
					led_setall(0,0,0,0,1);
					AlignRobotToPoint(&aim);
					if(isFrontClear()){
						MoveToPoint(&aim);
					}else{
						turn_robot(75);
						followCornerMode=1;
						follow_corner();
					}
				}else{
					led_setall(0,0,0,1,1);
					AlignRobotToPoint(&inter);
					if(isFrontClear()){
						MoveToPoint(&inter);
					}else{
						turn_robot(75);
						followCornerMode=1;
						follow_corner();
					}
				}
			}
		}
		
		/*//VOLTAGE SENSOR
		printf("Voltage VS: %0.1f mV \n", voltage);//VS=voltage sensor
		
		//CURRENT SENSOR
		printf("Current: %0.2f A \n", current);
		printf("Total Energy: %0.4f AH\n",amphours);*/
		
		if (amphours > 1.8 ){
			PORTC = 0b11111110;//PC0
		}
		if (amphours > 3.6 ){
			PORTC = 0b11111100;//PC1
		}
		if (amphours > 5.4 ){
			PORTC = 0b11111000;//PC2
		}
		if (amphours > 7.2 ){
			PORTC = 0b11110000;//PC3
		}
		if (amphours > 9 ){
			PORTC = 0b11100000;//PC4
		}

		//_delay_ms (1000);
		
		//_delay_ms(2000);
		//turn_robot(90);
		
		//zu nah 22
		//gut 40
		//zu weit 70
		
    }
}

//IMPORTANT FUNCTIONS

void turn_robot(float degrees){
	optocounterL=0;
	optocounterR=0;
	char stop=1;
	signed char mult=1;
	if(degrees<0){
		mult=-1;
	}
	//found through testing robot to turn 360°  baundary needs to be 82
	int baundary=0.2475*degrees*mult-1;
	motpwm_setLeft(1000*mult);
	motpwm_setRight(-1000*mult);
	while(stop){
		if(optocounterR>=baundary){
			motpwm_setRight(0);
			motpwm_setLeft(0);
			stop=0;
			printf("L:%d R:%d\n",optocounterL,optocounterR);
		}
	}
	printf("max; L:%d R:%d\n",optocounterL,optocounterR);
	//robot turned roughly 400° when odo =82
	nose_deg=nose_deg+optocounterR*4.2857*mult;
	if(nose_deg>360){
		nose_deg=nose_deg-360;
	}
	if(nose_deg<0){
		nose_deg=360+nose_deg;
	}
	float angle=-nose_deg/180.0*M_PI+M_PI_2;
	nose.x=robot.x+cos(angle);
	nose.y=robot.y+sin(angle);
	//printf("x:%f ,y:%f\n",nose.x,nose.y);
	printf("deg:%f\n",nose_deg);
}

void AlignRobotToPoint(vector_t  *point){
	//aling robot to direction
	signed char mult=1;
	float tmp=nose.x*point->y-nose.y*point->x;  // normal vector up or down
	if(tmp>0){
		mult=-1;
	}
	float dot_product=nose.x*point->x+nose.y*point->y;
	float length=sqrt(nose.x*nose.x+nose.y*nose.y)*sqrt(point->x*point->x+point->y*point->y);
	dot_product=dot_product/length;
	
	/*while(dot_product<0.99){
		dot_product=nose.x*point->x+nose.y*point->y;
		length=sqrt(nose.x*nose.x+nose.y*nose.y)*sqrt(point->x*point->x+point->y*point->y);
		dot_product=dot_product/length;
		turn_robot(mult*5);
	}*/
	
	if(dot_product<0.99){
		float angle=acos(dot_product);
		angle=angle/M_PI*180;
		turn_robot(mult*angle);
	}
}
int blockcounter=0;

void MoveABit(vector_t *point){
	//move robot a bit
	optocounterR=0;
	optocounterL=0;
	motpwm_setLeft(1000);
	motpwm_setRight(1000);
	char stop=1;
	float diffx=point->x-robot.x;
	float diffy=point->y-robot.y;
	float distance=sqrt(diffx*diffx+diffy*diffy);
	//int baundary=0.7067*distance;
	int baundary=5;
	
	while(stop){
		if(optocounterL>optocounterR && blockcounter<=0){
			if(motor_left<1000){
				motpwm_setRight(motor_right+1);	
			}else{
				motpwm_setLeft(motor_left-1);
			}
			blockcounter=200;
		}
		if(optocounterL<optocounterR && blockcounter<=0){
			if(motor_right<1000){
				motpwm_setLeft(motor_left+1);
			}else{
				motpwm_setRight(motor_right-1);
			}
			blockcounter=200;
		}
		if(optocounterR>baundary){
			motpwm_setRight(0);
		}
		if(optocounterL>baundary){
			motpwm_setLeft(0);
		}
		if(motor_right==0 && motor_left==0){
			stop=0;
		}
		blockcounter--;
	}
	//the robot moved 283cm when the optocoupler were 201
	distance=1.4079*optocounterR;

	float x=nose.x*distance;
	float y=nose.y*distance;
	if(point!=&aim){
		aim.x=aim.x-x;
		aim.y=aim.y-y;
		point->x=point->x-x;
		point->y=point->y-y;
		}else{
		aim.x=aim.x-x;
		aim.y=aim.y-y;
	}
}

void MoveToPoint(vector_t *point){
	AlignRobotToPoint(point);
	MoveABit(point);
	
}

//FOLLOW CORNER
void follow_corner(void) {
	//get values
	int fll=read_timer_value(4);
	int fl=read_timer_value(0);
	//printf("v:%d\n",fl);
	//move to the right // wall to the left
	
	/*if(fl<50){
		led_setall(0,0,1,0,0);
		turn_robot(30);
	}else*/
	if(fll>22 && fll<40){	//in perfect range
		led_setall(1,1,0,0,0);
		vector_t tmp=nose;
		MoveToPoint(&tmp);
		
	/*}else if(fll<10){	//corner right bend
		led_setall()
		turn_robot(45);
		while(fll>130){
			fll=analog_getValueExt(ANALOG_FLL,2);
			led_set(1,1);
			led_set(2,1);
			led_set(3,1);
			vector_t tmp=SetInterAim(nose,-0.15);
			MoveToPoint(&tmp);
			
		}*/
	}else if(fl<30){
		turn_robot(45);		
	}else if(fll>100){ //corner left bend
		led_setall(1,1,1,1,0);
		//Move a bit forward, then path should be free
		inter=nose;
		inter.x=20*inter.x;
		inter.y=20*inter.y;
		
		followCornerMode=0;
	}else if(fll<22){ 	//too close
		led_setall(1,0,0,0,0);
		//move a bit right
		vector_t tmp=SetInterAim(nose,0.15);
		MoveToPoint(&tmp);
	}else if(fll>40){		//too far
		led_setall(0,1,0,0,0);
		vector_t tmp=SetInterAim(nose,-0.15);
		MoveToPoint(&tmp);
	}
	/*if(isFrontClear()==0){//front not free
		led_setall(1,0,0,1,0);
		/*vector_t tmp=nose;
		MoveToPoint(&tmp);*
		turn_robot(75);
		/*while(fll>50){
			fll=read_timer_value(4);
			led_setall(1,1,1,0,0);
			vector_t tmp=SetInterAim(nose,-0.15);
			MoveToPoint(&tmp);
		}*
	}*/
}



//UTILITY
void motpwm_setLeft(int speed){
	if(speed<0){
		if (speed < -4095) {
			motor_left=-4095;
			motor_set_pwm(M2, 4096, 0);
		} else{
			motor_left=speed;
			speed=-speed;
			motor_set_pwm(M2, 0, speed);
		}
		motor_set_state(M2,CCW);
	}else{
		if (speed > 4095) {
			motor_left=4095;
			motor_set_pwm(M2, 4096, 0);
		} else{
			motor_left=speed;
			motor_set_pwm(M2, 0, speed);
		}
		motor_set_state(M2,CW);
	}
}

void motpwm_setRight(int speed){
	if(speed<0){
		if (speed < -4095) {
			motor_right=-4095;
			motor_set_pwm(M4, 4096, 0);
		} else{
			motor_right=speed;
			speed=-speed;
			motor_set_pwm(M4, 0, speed);
		}
		motor_set_state(M4,CCW);
		}else{
		if (speed > 4095) {
			motor_right=4095;
			motor_set_pwm(M4, 4096, 0);
		} else{
			motor_right=speed;
			motor_set_pwm(M4, 0, speed);
		}
		motor_set_state(M4,CW);
	}
}

vector_t SetInterAim(vector_t aim, float seed){
	//simple vector math
	//normal vector = (b-a) x (0,0,1)
	float x=aim.y;
	float y=-aim.x;
	float length=sqrt(x*x+y*y);
	x=aim.x+x/length*seed;
	y=aim.y+y/length*seed;
	vector_t result;
	result.x=x;
	result.y=y;
	return result;
}

void led_setall(char l1,char l2,char l3,char l4,char l5) {
	PORTC=(l1<<0)|(l2<<1)|(l3<<2)|(l4<<3)|(l5<<4);
}

#define CLOSE_THRESHOLD (5)
char CloseTo(vector_t aim){
	if(aim.x>-CLOSE_THRESHOLD && aim.x<CLOSE_THRESHOLD){
		if(aim.y>-CLOSE_THRESHOLD && aim.y<CLOSE_THRESHOLD){
			return 1;
		}
	}
	return 0;
}

char isFrontClear(void){
	return (read_timer_value(3)>80) && (read_timer_value(2)>20) && (read_timer_value(0)>20);
}

uint16_t adc_read(uint8_t adc_channel)
{
	ADMUX &= 0xF0; //clear any previously used channel
	ADMUX |= adc_channel; //set desired channel
	//start conversion
	ADCSRA |= (1<<ADSC);
	//now wait for the conversion to be complete
	while ( (ADCSRA & (1<<ADSC)) );
	/*at this point we have the result of the conversion and
    we return it to the function as a 16 bit unsigned int*/
	return ADC;
}	

ISR(ADC_vect){
	/*if(adc_index==5){
		//VOLTAGE SENSOR
		volatile uint16_t adc_result = ADC;
		voltage = (5000.0/1024) * adc_result;
		adc_index=6;
	}else{*/
		//CURRENT SENSOR
		volatile uint16_t adc_result = ADC;
		volatile float difftime=get_time_ms()-lastmeasure;
		lastmeasure=get_time_ms();
		volatile float voltage2 = (5000.0/1024) * adc_result;
		current = ((voltage2/1000) + 0.073)/0.452;
		
		
		//total_energy+=current*voltage/1000.0f*difftime/1000.0f;
		amphours+=current*difftime/1000.0f/60.0f/60.0f;
		//adc_index=5;
		
	//}
	ADMUX &= 0xF0; //clear any previously used channel
//	ADMUX |= adc_index; //set desired channel
	ADMUX |= 6; //set desired channel
	//start conversion

	ADCSRA |= (1<<ADSC);
	
}

//optocoupler:
ISR(INT4_vect){
	optocounterR++;
	
}

ISR(INT5_vect){
	optocounterL++;
}