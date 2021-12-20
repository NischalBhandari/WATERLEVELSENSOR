/*
 * TCPClientConnect.c
 *
 * Created: 11/21/2021 8:55:42 AM
 *  Author: Nischal
 */ 

/************************************************************************/
/* Read response to find out if \r\n has been found and send a SIM900_RESPONSE_FINISHED IF necessary                                                                     */
/************************************************************************/

#include "TCPClientConnect.h"
#include "../lcd/lcdlib.h"

char My_Buffer[20];
volatile int16_t Flag =0, Count =0;
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
		/**
			If the timecount is greater than 10000(Default Timeout) + TimeOut then return nothing
		 **/
		if(TimeCount >= (DEFAULT_TIMEOUT+TimeOut))
		{
			CRLF_COUNT = 0; TimeOut = 0;
			Response_Status = SIM900_RESPONSE_TIMEOUT;
			return;
		}

		/************************************************************************/
		/* If the response status is SIM900_RESPONSE_STARTING
		CRLF_FOUND = 0
		set the CRLF_BUF to ZERO for 2 size
		Response_Stauts= SIM900_RESPONSE_WAITING                                                                     */
		/************************************************************************/
		if(Response_Status == SIM900_RESPONSE_STARTING)
		{
			CRLF_FOUND = 0;
			/* fill memory block with a certain value memset(void *ptr, int x , size_t n) */
			memset(CRLF_BUF, 0, 2);
			
			Response_Status = SIM900_RESPONSE_WAITING;
		}
		/************************************************************************/
		/* Calculate the length of the response buffer
		if the length is not zero then
		increase the timecount
		if ResponseBufferLength == Strlen(RESPONSE_BUFFER)
		for i =0 ; i<ReponseBufferLength; i++
		memmove(CRLF_BUF,CRLF_BUF+1,1);
		void *memmove(void *str1, const void *str2, size_t n)
		Add the response buffer value to the CRLF_BUF[1]
		Compare CRLF_BUF with \r\N 
		int strncmp(const char *str1, const char *str2, size_t n)

		                                                                     */
		/************************************************************************/
		ResponseBufferLength = strlen(RESPONSE_BUFFER);
		//LCD_String(RESPONSE_BUFFER);
		if (ResponseBufferLength)
		{
			_delay_ms(1);
			TimeCount++;
			if (ResponseBufferLength==strlen(RESPONSE_BUFFER))
			{
				for (uint16_t i=0;i<ResponseBufferLength;i++)
				{
					// move the CRLF_BUF[1] TO CRLF_BUF[0] this is like pushing the stack from back
					memmove(CRLF_BUF, CRLF_BUF + 1, 1);
					
					// now add the new data to the space . So hence this is a 2 character buffer to check the \r\n
					CRLF_BUF[1] = RESPONSE_BUFFER[i];
					
					// CHECK if the CRLF_BUF contains these characters
					if(!strncmp(CRLF_BUF, "\r\n", 2))
					{
						// if the Character finds these 2 times then
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

char* getSignalQuality(int Input)
{
	
	char buf[20];
	TCPClient_Clear();
	sprintf(buf,"AT+CSQ=%d\r\n",Input);
	LCD_Clear();
	LCD_String(buf);
	_delay_ms(4000);
	USART_SendString(buf);
	_delay_ms(1000);

	while((strstr(RESPONSE_BUFFER, "AT+CSQ") != NULL));
		return RESPONSE_BUFFER ;
	
}

bool WaitForExpectedResponse(char* ExpectedResponse)
{
	TCPClient_Clear();
	_delay_ms(1000);
	Start_Read_Response();						/* First read response 
	char *strstr(const char *haystack, const char *needle)
	*/
	if((Response_Status != SIM900_RESPONSE_TIMEOUT) && (strstr(RESPONSE_BUFFER, ExpectedResponse) != NULL))
	LCD_Clear();
	LCD_String(ExpectedResponse);
	_delay_ms(1000);
	return "true";							/* Return true for success */
	return "false";								/* Else return false */
}

bool SendATandExpectResponse(char* ATCommand, char* ExpectedResponse)
{
	USART_SendString(ATCommand);				/* Send AT command to SIM900 */
	USART_TxChar('\r');
	USART_TxChar('\n');
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

bool SIM900_Start()
{
	for (uint8_t i=0;i<10;i++)
	{	
		if(SendATandExpectResponse("ATE0","OK")||SendATandExpectResponse("AT","OK"))
		return true;
	}
	return false;
}

bool TCPClient_Shut()
{
	USART_SendString("AT+CIPSHUT\r");
	LCD_Clear();
	LCD_String("send shut command");
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

void SMS_Test(){
	SendATandExpectResponse("AT", "OK");
	
	LCD_Clear();
	SendATandExpectResponse("AT+CMGF=1", "OK");
	LCD_Clear();
	SendATandExpectResponse("AT+CMGS=\"+9779860086391\"","OK");
	_delay_ms(3000);
	SendATandExpectResponse("Test From Nischal","OK");
	_delay_ms(4000);
	USART_TxChar(26);
	LCD_String("sent");
	
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
