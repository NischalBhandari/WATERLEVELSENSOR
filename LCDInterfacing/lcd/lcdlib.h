/*
 * lcdlib.h
 *
 * Created: 11/10/2021 9:19:50 PM
 *  Author: Nischal
 */ 


#ifndef LCDLIB_H_
#define LCDLIB_H_


#define F_CPU 16000000UL			/* Define CPU Frequency e.g. here 8MHz */
#include <avr/io.h>			/* Include AVR std. library file */
#include <util/delay.h>			/* Include Delay header file */
#define LCD_Dir  DDRC			/* Define LCD data port direction */
#define LCD_Port PORTC			/* Define LCD data port */
#define RS PC0				/* Define Register Select pin */
#define EN PC1 				/* Define Enable signal pin */


void LCD_Command( unsigned char cmnd );
void LCD_Char( unsigned char data );

void LCD_Init (void);

void LCD_String(char *str);
void LCD_String_xy(char row, char pos, char *str);

void LCD_Clear();


#endif /* LCDLIB_H_ */