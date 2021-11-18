/*
 * lcdlib.c
 *
 * Created: 11/10/2021 9:21:24 PM
 *  Author: Nischal
 */ 
#include "lcdlib.h"


void LCD_Command( unsigned char cmnd )
{
	LCD_Port = (LCD_Port & 0xC3) | (cmnd  >> 2); /* sending upper nibble */
	LCD_Port &= ~ (1<<RS);		/* RS=0, command reg. */
	LCD_Port |= (1<<EN);		/* Enable pulse */
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);

	_delay_us(200);

	LCD_Port = (LCD_Port & 0xC3) | (cmnd << 2);  /* sending lower nibble */
	LCD_Port |= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);
	_delay_ms(2);
}


void LCD_Char( unsigned char data )
{
	LCD_Port = (LCD_Port & 0xC3) | (data >> 2 ); /* sending upper nibble */
	LCD_Port |= (1<<RS);		/* RS=1, data reg. */
	LCD_Port|= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);

	_delay_us(200);

	LCD_Port = (LCD_Port & 0xC3) | (data << 2); /* sending lower nibble */
	LCD_Port |= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);
	_delay_ms(2);
}

void LCD_Init (void)			/* LCD Initialize function */
{
	LCD_Dir = 0x3F;			/* Make LCD port direction as o/p */
	_delay_ms(20);			/* LCD Power ON delay always >15ms */
	
	LCD_Command(0x02);		/* send for 4 bit initialization of LCD  */
	LCD_Command(0x28);              /* 2 line, 5*7 matrix in 4-bit mode */
	LCD_Command(0x0c);              /* Display on cursor off*/
	LCD_Command(0x06);              /* Increment cursor (shift cursor to right)*/
	LCD_Command(0x01);              /* Clear display screen*/
	_delay_ms(2);
}


void LCD_String(char *str)
{
	
	int i;
	for(i=0; str[i]!=0; i++)
	{
		
		LCD_Char(str[i]);
	}
}

void LCD_String_xy(char row, char pos, char *str){
	if( row == 0  && pos < 16)
	LCD_Command((pos & 0x0F)| 0x80);
	else if (row == 1 && pos <16)
	LCD_Command((pos & 0x0F)|0xC0);
	LCD_String(str);
}

void LCD_Clear()
{
	
	LCD_Command(0x01); /*clear display */
	_delay_ms(2);
	LCD_Command(0x80); /* cursor at home position */
}