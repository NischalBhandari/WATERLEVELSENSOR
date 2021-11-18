/*
 * CFile1.c
 *
 * Created: 11/9/2021 7:18:57 PM
 *  Author: Nischal
 */ 

#include "ultrasonic.h"

long count;
char string[10];
double distance;

int TimerOverflow = 0;

void SensorInit()
{
	DDRD = (1 << PD5);
	TIMSK  = (1 << TOIE1);
	TCCR1A = 0;
}

void sensorTrigger()
{
	PORTD  |= (1 << TriggerPin);
	_delay_us(10);
	PORTD &= (~(1 << TriggerPin));
	TCNT1 = 0;
	TCCR1B = 0X41;
	TIFR  = 1 << ICF1;
	TIFR = 1 << TOV1;
}


void calculateRisingEdge()
{
	
	while((TIFR &(1 << ICF1)) == 0); /* WAIT FOR RISING EDGE*/
	TCNT1 = 0; /* clear timer counter */
	TCCR1B = 0x01; /* capture on falling edge */
	TIFR  = 1 << ICF1; /* clear Input Capture Flag */
	TIFR =  1 << TOV1; /* clear timer overflow flag */
	TimerOverflow = 0; /* clear timer overflow count */
}

 char* calculateFallingEdge()
{
	
	while((TIFR & (1 << ICF1)) == 0 ) /* WAIT FOR FALLING EDGE */
	count  = ICR1 + (65535*TimerOverflow);
	distance = (double)count/466.47;
	dtostrf(distance,2,2,string);
	strcat(string, "cm");
	return string;
}


ISR(TIMER1_OVF_vect)
{
	TimerOverflow++;	/* Increment Timer Overflow count */
}
