/*
 * time.h
 *
 * Created: 07.12.2021 09:40:14
 *  Author: vince
 * Library to measure Time since system start
 * The Runtime is assumed to be smaller than 255h
 * Uses Timer 4 for measurement
 */ 


#ifndef TIME_H_
#define TIME_H_

float time_ms;		//max	921499999		Type max:	3.4E+38
float  time_s;		//max:	921499			Type max:	3.4E+38
float time_min;		//max:	15359			Type max:	3.4E+38
float time_h;		//max:	255				Type max:	3.4E+38

void time_init(void);
float get_time_ms(void);
float get_time_s(void);
float get_time_min(void);
float get_time_h(void);


#endif /* TIME_H_ */