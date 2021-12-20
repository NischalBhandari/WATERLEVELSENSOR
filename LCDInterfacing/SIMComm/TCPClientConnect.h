/*
 * TCPClientConnect.h
 *
 * Created: 11/21/2021 8:56:15 AM
 *  Author: Nischal
 */ 


#ifndef TCPCLIENTCONNECT_H_
#define TCPCLIENTCONNECT_H_


#define F_CPU 16000000UL			/* Define CPU Frequency e.g. here 8MHz */
#include <avr/io.h>			/* Include AVR std. library file */
#include <util/delay.h>			/* Include Delay header file */
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>					/* Include standard library */
#include "../uart/USART_RS232_H_file.h"


#define APN					"internet"
#define USERNAME			""
#define PASSWORD			""

#define SREG    _SFR_IO8(0x3F)

#define DEFAULT_BUFFER_SIZE		200
#define DEFAULT_TIMEOUT			10000
#define DEFAULT_CRLF_COUNT		1

enum SIM900_RESPONSE_STATUS {
	SIM900_RESPONSE_WAITING = 'a',
	SIM900_RESPONSE_FINISHED = 'b' ,
	SIM900_RESPONSE_TIMEOUT = 'c',
	SIM900_RESPONSE_BUFFER_FULL = 'd',
	SIM900_RESPONSE_STARTING= 'e',
	SIM900_RESPONSE_ERROR='f'
};

char RESPONSE_BUFFER[DEFAULT_BUFFER_SIZE];




void Read_Response();
void TCPClient_Clear();
void Start_Read_Response();
void GetResponseBody(char* Response, uint16_t ResponseLength);
bool WaitForExpectedResponse(char* ExpectedResponse);
bool SendATandExpectResponse(char* ATCommand, char* ExpectedResponse);
bool TCPClient_ApplicationMode(uint8_t Mode);
bool TCPClient_ConnectionMode(uint8_t Mode);
bool AttachGPRS();
bool SIM900_Start();
char* getSignalQuality(int Input);
bool TCPClient_Shut();
bool TCPClient_Close();
bool TCPClient_Connect(char* _APN, char* _USERNAME, char* _PASSWORD);
bool TCPClient_connected();
int16_t TCPClient_DataAvailable();
uint8_t TCPClient_DataRead();
uint8_t TCPClient_Start(uint8_t _ConnectionNumber, char* Domain, char* Port);
uint8_t TCPClient_Send(char* Data, uint16_t _length);
void SMS_Test();

#endif /* TCPCLIENTCONNECT_H_ */