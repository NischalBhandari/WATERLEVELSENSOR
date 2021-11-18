/*
 * LCDInterfacing.c
 *
 * Created: 11/6/2021 8:12:27 AM
 *  Author: Nischal
 */ 

#define F_CPU 16000000UL			/* Define CPU Frequency e.g. here 8MHz */
#include <avr/io.h>			/* Include AVR std. library file */
#include <util/delay.h>			/* Include Delay header file */
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>					/* Include standard library */
#include "uart/USART_RS232_H_file.h"
#include "lcd/lcdlib.h"




#define APN					"internet"
#define USERNAME			""
#define PASSWORD			""
char My_Buffer[20];
volatile int16_t Flag =0, Count =0;

#define SREG    _SFR_IO8(0x3F)

#define DEFAULT_BUFFER_SIZE		200
#define DEFAULT_TIMEOUT			10000
#define DEFAULT_CRLF_COUNT		2

enum SIM900_RESPONSE_STATUS {
	SIM900_RESPONSE_WAITING,
	SIM900_RESPONSE_FINISHED,
	SIM900_RESPONSE_TIMEOUT,
	SIM900_RESPONSE_BUFFER_FULL,
	SIM900_RESPONSE_STARTING,
	SIM900_RESPONSE_ERROR
};

char RESPONSE_BUFFER[DEFAULT_BUFFER_SIZE];

int8_t Response_Status, CRLF_COUNT = 0;
volatile int16_t Counter = 0, pointer = 0;
uint32_t TimeOut = 0;

void Read_Response()
{
	char CRLF_BUF[2];
	char CRLF_FOUND;
	uint32_t TimeCount = 0, ResponseBufferLength;
	while(1)
	{
		if(TimeCount >= (DEFAULT_TIMEOUT+TimeOut))
		{
			CRLF_COUNT = 0; TimeOut = 0;
			Response_Status = SIM900_RESPONSE_TIMEOUT;
			return;
		}

		if(Response_Status == SIM900_RESPONSE_STARTING)
		{
			CRLF_FOUND = 0;
			/* fill memory block with a certain value memset(void *ptr, int x , size_t n) */
			memset(CRLF_BUF, 0, 2);
			
			Response_Status = SIM900_RESPONSE_WAITING;
		}
		ResponseBufferLength = strlen(RESPONSE_BUFFER);
		if (ResponseBufferLength)
		{
			_delay_ms(1);
			TimeCount++;
			if (ResponseBufferLength==strlen(RESPONSE_BUFFER))
			{
				for (uint16_t i=0;i<ResponseBufferLength;i++)
				{
					memmove(CRLF_BUF, CRLF_BUF + 1, 1);
					CRLF_BUF[1] = RESPONSE_BUFFER[i];
					if(!strncmp(CRLF_BUF, "\r\n", 2))
					{
						if(++CRLF_FOUND == (DEFAULT_CRLF_COUNT+CRLF_COUNT))
						{
							CRLF_COUNT = 0; TimeOut = 0;
							Response_Status = SIM900_RESPONSE_FINISHED;
							return;
						}
					}
				}
				CRLF_FOUND = 0;
			}
		}
		_delay_ms(1);
		TimeCount++;
	}
}

void TCPClient_Clear()
{
	memset(RESPONSE_BUFFER,0,DEFAULT_BUFFER_SIZE);
	Counter = 0;	pointer = 0;
}

void Start_Read_Response()
{
	Response_Status = SIM900_RESPONSE_STARTING;
	do {
		Read_Response();
	} while(Response_Status == SIM900_RESPONSE_WAITING);

}

void GetResponseBody(char* Response, uint16_t ResponseLength)
{

	uint16_t i = 12;
	char buffer[5];
	while(Response[i] != '\r')
	++i;

	strncpy(buffer, Response + 12, (i - 12));
	ResponseLength = atoi(buffer);

	i += 2;
	uint16_t tmp = strlen(Response) - i;
	memcpy(Response, Response + i, tmp);

	if(!strncmp(Response + tmp - 6, "\r\nOK\r\n", 6))
	memset(Response + tmp - 6, 0, i + 6);
}

bool WaitForExpectedResponse(char* ExpectedResponse)
{
	TCPClient_Clear();
	_delay_ms(1000);
	Start_Read_Response();						/* First read response */
	if((Response_Status != SIM900_RESPONSE_TIMEOUT) && (strstr(RESPONSE_BUFFER, ExpectedResponse) != NULL))
	return true;							/* Return true for success */
	return false;								/* Else return false */
}

bool SendATandExpectResponse(char* ATCommand, char* ExpectedResponse)
{
	USART_SendString(ATCommand);				/* Send AT command to SIM900 */
	USART_TxChar('\r');
	return WaitForExpectedResponse(ExpectedResponse);
}

bool TCPClient_ApplicationMode(uint8_t Mode)
{
	char _buffer[20];
	sprintf(_buffer, "AT+CIPMODE=%d\r", Mode);
	_buffer[19] = 0;
	USART_SendString(_buffer);
	return WaitForExpectedResponse("OK");
}

bool TCPClient_ConnectionMode(uint8_t Mode)
{
	char _buffer[20];
	sprintf(_buffer, "AT+CIPMUX=%d\r", Mode);
	_buffer[19] = 0;
	USART_SendString(_buffer);
	return WaitForExpectedResponse("OK");
}

bool AttachGPRS()
{
	USART_SendString("AT+CGATT=1\r");
	return WaitForExpectedResponse("OK");
}

char * SIM900_Start()
{
	for (uint8_t i=0;i<1;i++)
	{	
		if(SendATandExpectResponse("ATE0","OK")||SendATandExpectResponse("AT","OK"))
		return "true";
	}
	return "false";
}

bool TCPClient_Shut()
{
	USART_SendString("AT+CIPSHUT\r");
	return WaitForExpectedResponse("OK");
}

bool TCPClient_Close()
{
	USART_SendString("AT+CIPCLOSE=1\r");
	return WaitForExpectedResponse("OK");
}

bool TCPClient_Connect(char* _APN, char* _USERNAME, char* _PASSWORD)
{

	USART_SendString("AT+CREG?\r");
	if(!WaitForExpectedResponse("+CREG: 0,1"))
	return false;

	USART_SendString("AT+CGATT?\r");
	if(!WaitForExpectedResponse("+CGATT: 1"))
	return false;

	USART_SendString("AT+CSTT=\"");
	USART_SendString(_APN);
	USART_SendString("\",\"");
	USART_SendString(_USERNAME);
	USART_SendString("\",\"");
	USART_SendString(_PASSWORD);
	USART_SendString("\"\r");
	if(!WaitForExpectedResponse("OK"))
	return false;

	USART_SendString("AT+CIICR\r");
	if(!WaitForExpectedResponse("OK"))
	return false;

	USART_SendString("AT+CIFSR\r");
	if(!WaitForExpectedResponse("."))
	return false;

	USART_SendString("AT+CIPSPRT=1\r");
	return WaitForExpectedResponse("OK");
}

bool TCPClient_connected() {
	USART_SendString("AT+CIPSTATUS\r");
	CRLF_COUNT = 2;
	return WaitForExpectedResponse("CONNECT OK");
}

uint8_t TCPClient_Start(uint8_t _ConnectionNumber, char* Domain, char* Port)
{
	char _buffer[25];
	USART_SendString("AT+CIPMUX?\r");
	if(WaitForExpectedResponse("+CIPMUX: 0"))
	USART_SendString("AT+CIPSTART=\"TCP\",\"");
	else
	{
		sprintf(_buffer, "AT+CIPSTART=\"%d\",\"TCP\",\"", _ConnectionNumber);
		USART_SendString(_buffer);
	}
	
	USART_SendString(Domain);
	USART_SendString("\",\"");
	USART_SendString(Port);
	USART_SendString("\"\r");

	CRLF_COUNT = 2;
	if(!WaitForExpectedResponse("CONNECT OK"))
	{
		if(Response_Status == SIM900_RESPONSE_TIMEOUT)
		return SIM900_RESPONSE_TIMEOUT;
		return SIM900_RESPONSE_ERROR;
	}
	return SIM900_RESPONSE_FINISHED;
}

uint8_t TCPClient_Send(char* Data, uint16_t _length)
{
	USART_SendString("AT+CIPSEND\r");
	CRLF_COUNT = -1;
	WaitForExpectedResponse(">");
	
	for (uint16_t i = 0; i < _length; i++)
	USART_TxChar(Data[i]);
	USART_TxChar(0x1A);

	if(!WaitForExpectedResponse("SEND OK"))
	{
		if(Response_Status == SIM900_RESPONSE_TIMEOUT)
		return SIM900_RESPONSE_TIMEOUT;
		return SIM900_RESPONSE_ERROR;
	}
	return SIM900_RESPONSE_FINISHED;
}

int16_t TCPClient_DataAvailable()
{
	return (Counter - pointer);
}

uint8_t TCPClient_DataRead()
{
	if(pointer<Counter)
	return RESPONSE_BUFFER[pointer++];
	else{
		TCPClient_Clear();
		return 0;
	}
}


int main()
{
	//long  count;
	//char string[10];
	//double  distance;

	LCD_Init();
	//SensorInit();
	sei();
	USART_Init(9600);
	LCD_String("Electron Solution");
	LCD_Command(0xC0); /* Go to 2nd line */
	//while(!SIM900_Start());
	LCD_String(SIM900_Start());
//	TCPClient_Shut();
	//TCPClient_ConnectionMode(0);			/* 0 = Single; 1 = Multi */
	//TCPClient_ApplicationMode(0);			/* 0 = Normal Mode; 1 = Transperant Mode */
//	AttachGPRS();
	//TCPClient_Connect(APN, USERNAME, PASSWORD);
	while(1){
		if(Flag == 1)
		{
			LCD_Clear();
			LCD_String(RESPONSE_BUFFER);
		//	USART_SendString(RESPONSE_BUFFER);
		//	LCD_String(RESPONSE_BUFFER);
			Flag = 0;
		}
		//sensorTrigger();
		//calculateRisingEdge();
		//LCD_Clear();
		//LCD_String(calculateFallingEdge(&count, &distance, string));
	}
	
	return 0;
}


ISR (USART_RXC_vect)
{
	
	uint8_t oldsrg = SREG;
	Flag = 1;
	RESPONSE_BUFFER[Counter] = UDR;
	Counter++;
	if(Counter == DEFAULT_BUFFER_SIZE){
		Counter = 0; pointer = 0;
	}
	SREG = oldsrg;
}
