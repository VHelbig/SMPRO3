

#define F_CPU 16000000UL

#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <util/delay.h>
#include "lcd.h"
#include "usart.h"
#include <avr/interrupt.h>
 
uint16_t adc_result;
uint16_t adc_read(uint8_t adc_channel);

int current_total = 0;

int main(void)
{
	uart_init();
	io_redirect();
	
	DDRF = 0b00011111;// inputs of the ADC PF7..5
	PORTF = 0x00; //NO PULL-UPS
	DDRC = 0xFF;//LEDs Outputs
	PORTC = (1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3)|(1<<PC4);//All LEDs ON
	
	uint16_t adc_result;
	
	//select Vref = Aref and AVcc turn off
	ADMUX = (0<<REFS0)|(0<<REFS0);
	ADCSRB = (1<<MUX5);
	//set prescaler to 128 and turn on the ADC module
	ADCSRA = (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN);
	
	while(1)
	{
		//VOLTAGE SENSOR
			adc_result = adc_read(PF5);
			float voltage1 = (5000.0/1024) * adc_result;
			
			printf("Voltage VS: %f0.1f mV \n", voltage1);//VS=voltage sensor
			
		//CURRENT SENSOR
			adc_result = adc_read(PF6);
			float voltage2 = (5000.0/1024) * adc_result;
			float current = ((voltage2/1000) + 0.073)/0.452;
			current_total += current;
			
			printf("Voltage CS: %0.1f mV \n", voltage2);//CS=current sensor
			printf("Current: %0.2f A \n", current);
			
		if (current_total == 2.4 ){PORTC = 0b11111110;}//PC0
		if (current_total == 4.8 ){PORTC = 0b11111100;}//PC1
		if (current_total == 7.2 ){PORTC = 0b11111000;}//PC2
		if (current_total == 9.6 ){PORTC = 0b11110000;}//PC3
		if (current_total == 12.0 ){PORTC = 0b11100000;}//PC4
			
		_delay_ms (1000);
	}
}

uint16_t adc_read(uint8_t adc_channel)
{
	ADMUX &= 0x00; //clear any previously used channel
	ADMUX |= adc_channel; //set desired channel
	//start conversion
	ADCSRA |= (1<<ADSC);
	//now wait for the conversion to be complete
	while ( (ADCSRA & (1<<ADSC)) );
	/*at this point we have the result of the conversion and
    we return it to the function as a 16 bit unsigned int*/
	return ADC;
}	
