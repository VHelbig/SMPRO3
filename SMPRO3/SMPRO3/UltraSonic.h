/*
 * UltraSonic.h
 *
 * Created: 14.12.2021 10:42:16
 *  Author: vince
 */ 


#ifndef ULTRASONIC_H_
#define ULTRASONIC_H_

#ifndef ULTRA_NUMBER
#define ULTRA_NUMBER 5	//number of ultrasonic sensors
#endif

#ifndef OLD_VALUE_THRESHOLD
#define OLD_VALUE_THRESHOLD 500
#endif

//STRUCTURES
typedef struct{
	int timer_value;
	float time_stamp;	//TODO change this to a smaller type later
}timer_value_t;			//struct for keeping track of all timer values and if they are up to date

//GLOBAL VARIABLES

//ultrasonic data array
extern volatile timer_value_t timer_values[ULTRA_NUMBER];
extern volatile int ultra_error;
extern volatile int ultra_timer_error;


//FUNCTION PROTOTYPES

void init_ultra_trigger(void);
void init_ultra_read(void);

//timer functions
void reset_timer(void);		//just reset and start timer1
int get_timer_value(void);	//just get timer1 value
int get_and_stop_timer(void); // get timer value and stop timer1

int read_timer_value(int index);	//function for getting ultra sonic sensor value from array, wont be needed later

//Utility but not used
void trigger_ultra(int index);	//triggers ultrasonic sensor not recommended, as it blocks the execution for 10us
void print_all_timer_values(void);	//prints all ultrasonic sensor values and their update status

#endif /* ULTRASONIC_H_ */