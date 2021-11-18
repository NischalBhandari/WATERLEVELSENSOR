/*
 * IncFile1.h
 *
 * Created: 11/9/2021 8:56:03 PM
 *  Author: Nischal
 */ 


#ifndef INCFILE1_H_
#define INCFILE1_H_
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#define TriggerPin PD4

void SensorInit();
void sensorTrigger();
void calculateRisingEdge();
char* calculateFallingEdge( long* count , double * distance, char * myString );

#endif /* INCFILE1_H_ */