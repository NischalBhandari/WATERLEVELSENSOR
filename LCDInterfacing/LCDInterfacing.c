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
#include "SIMComm/TCPClientConnect.h"
#include "lcd/lcdlib.h"


#define min(a,b)				({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

#define MQTT_PROTOCOL_LEVEL		4

#define MQTT_CTRL_CONNECT		0x1
#define MQTT_CTRL_CONNECTACK	0x2
#define MQTT_CTRL_PUBLISH		0x3
#define MQTT_CTRL_PUBACK		0x4
#define MQTT_CTRL_PUBREC		0x5
#define MQTT_CTRL_PUBREL		0x6
#define MQTT_CTRL_PUBCOMP		0x7
#define MQTT_CTRL_SUBSCRIBE		0x8
#define MQTT_CTRL_SUBACK		0x9
#define MQTT_CTRL_UNSUBSCRIBE	0xA
#define MQTT_CTRL_UNSUBACK		0xB
#define MQTT_CTRL_PINGREQ		0xC
#define MQTT_CTRL_PINGRESP		0xD
#define MQTT_CTRL_DISCONNECT	0xE

#define MQTT_QOS_1				0x1
#define MQTT_QOS_0				0x0

/* Adjust as necessary, in seconds */
#define MQTT_CONN_KEEPALIVE		60

#define MQTT_CONN_USERNAMEFLAG	0x80
#define MQTT_CONN_PASSWORDFLAG	0x40
#define MQTT_CONN_WILLRETAIN	0x20
#define MQTT_CONN_WILLQOS_1		0x08
#define MQTT_CONN_WILLQOS_2		0x18
#define MQTT_CONN_WILLFLAG		0x04
#define MQTT_CONN_CLEANSESSION	0x02


/* Select Demo */
#define SUBSRCIBE_DEMO								/* Define SUBSRCIBE demo */
//#define PUBLISH_DEMO								/* Define PUBLISH demo */

#define AIO_SERVER			"io.adafruit.com"		/* Adafruit server */
#define AIO_SERVER_PORT		"1883"					/* Server port */
#define AIO_BASE_URL		"/api/v2"				/* Base URL for api */
#define AIO_USERNAME		"Enter Username"		/* Enter username here */
#define AIO_KEY				"Enter AIO key"			/* Enter AIO key here */
#define AIO_FEED			"Enter Feed Key"		/* Enter feed key */

int16_t packet_id_counter = 0;


char clientID[] ="";
char will_topic[] = "";
char will_payload[] ="";
uint8_t will_qos = 1;
uint8_t will_retain = 0;

uint16_t StringToUint16(uint8_t* _String)
{
	uint16_t value = 0;
	while ('0' == *_String || *_String == ' ' || *_String == '"')
	{
		_String++;
	}
	while ('0' <= *_String && *_String <= '9')
	{
		value *= 10;
		value += *_String - '0';
		_String++;
	}
	return value;
}

uint8_t* AddStringToBuf(uint8_t *_buf, const char *_string)
{
	uint16_t _length = strlen(_string);
	_buf[0] = _length >> 8;
	_buf[1] = _length & 0xFF;
	_buf+=2;
	strncpy((char *)_buf, _string, _length);
	return _buf + _length;
}

bool sendPacket(uint8_t *buffer, uint16_t len)
{
	if (TCPClient_connected())
	{
		uint16_t sendlen = min(len, 250);
		TCPClient_Send((char*)buffer, sendlen);
		return true;
	}
	else
	return false;
}

uint16_t readPacket(uint8_t *buffer, int16_t _timeout)
{
	uint16_t len = 0;
	while (TCPClient_DataAvailable() > 0 && _timeout >=0)
	{
		buffer[len++] = TCPClient_DataRead();
		_timeout-= 10;	_delay_ms(10);
	}
	buffer[len] = 0;
	return len;
}

uint8_t MQTT_ConnectToServer()
{
	return TCPClient_Start(1, AIO_SERVER, AIO_SERVER_PORT);
}

uint16_t MQTT_connectpacket(uint8_t* packet)
{
	uint8_t*_packet = packet;
	uint16_t _length;

	_packet[0] = (MQTT_CTRL_CONNECT << 4);
	_packet+=2;
	_packet = AddStringToBuf(_packet, "MQTT");
	_packet[0] = MQTT_PROTOCOL_LEVEL;
	_packet++;
	
	_packet[0] = MQTT_CONN_CLEANSESSION;
	if (will_topic && strlen(will_topic) != 0) {
		_packet[0] |= MQTT_CONN_WILLFLAG;
		if(will_qos == 1)
		_packet[0] |= MQTT_CONN_WILLQOS_1;
		else if(will_qos == 2)
		_packet[0] |= MQTT_CONN_WILLQOS_2;
		if(will_retain == 1)
		_packet[0] |= MQTT_CONN_WILLRETAIN;
	}
	if (strlen(AIO_USERNAME) != 0)
	_packet[0] |= MQTT_CONN_USERNAMEFLAG;
	if (strlen(AIO_KEY) != 0)
	_packet[0] |= MQTT_CONN_PASSWORDFLAG;
	_packet++;

	_packet[0] = MQTT_CONN_KEEPALIVE >> 8;
	_packet++;
	_packet[0] = MQTT_CONN_KEEPALIVE & 0xFF;
	_packet++;
	if (strlen(clientID) != 0) {
		_packet = AddStringToBuf(_packet, clientID);
		} else {
		_packet[0] = 0x0;
		_packet++;
		_packet[0] = 0x0;
		_packet++;
	}
	if (will_topic && strlen(will_topic) != 0) {
		_packet = AddStringToBuf(_packet, will_topic);
		_packet = AddStringToBuf(_packet, will_payload);
	}

	if (strlen(AIO_USERNAME) != 0) {
		_packet = AddStringToBuf(_packet, AIO_USERNAME);
	}
	if (strlen(AIO_KEY) != 0) {
		_packet = AddStringToBuf(_packet, AIO_KEY);
	}
	_length = _packet - packet;
	packet[1] = _length-2;

	return _length;
}

uint16_t MQTT_publishPacket(uint8_t *packet, const char *topic, char *data, uint8_t qos)
{
	uint8_t *_packet = packet;
	uint16_t _length = 0;
	uint16_t Datalen=strlen(data);

	_length += 2;
	_length += strlen(topic);
	if(qos > 0) {
		_length += 2;
	}
	_length += Datalen;
	_packet[0] = MQTT_CTRL_PUBLISH << 4 | qos << 1;
	_packet++;
	do {
		uint8_t encodedByte = _length % 128;
		_length /= 128;
		if ( _length > 0 ) {
			encodedByte |= 0x80;
		}
		_packet[0] = encodedByte;
		_packet++;
	} while ( _length > 0 );
	_packet = AddStringToBuf(_packet, topic);
	if(qos > 0) {
		_packet[0] = (packet_id_counter >> 8) & 0xFF;
		_packet[1] = packet_id_counter & 0xFF;
		_packet+=2;
		packet_id_counter++;
	}
	memmove(_packet, data, Datalen);
	_packet+= Datalen;
	_length = _packet - packet;

	return _length;
}

uint16_t MQTT_subscribePacket(uint8_t *packet, const char *topic, uint8_t qos)
{
	uint8_t *_packet = packet;
	uint16_t _length;

	_packet[0] = MQTT_CTRL_SUBSCRIBE << 4 | MQTT_QOS_1 << 1;
	_packet+=2;

	_packet[0] = (packet_id_counter >> 8) & 0xFF;
	_packet[1] = packet_id_counter & 0xFF;
	_packet+=2;
	packet_id_counter++;

	_packet = AddStringToBuf(_packet, topic);

	_packet[0] = qos;
	_packet++;

	_length = _packet - packet;
	packet[1] = _length-2;

	return _length;
}


int main()
{

	LCD_Init();
	SensorInit();
	sei();
	USART_Init(19200);
	LCD_String("Electron Solution");
	LCD_Command(0xC0); /* Go to 2nd line */
	_delay_ms(5000);
	LCD_Clear();
	//LCD_String(getSignalQuality(0));

	while(!SIM900_Start());
	//LCD_Clear();
	//LCD_String("Starting Connection");
	//TCPClient_Shut();
	//LCD_Clear();
	//LCD_String(getSignalQuality(0));
	//_delay_ms(10000);
	//TCPClient_ConnectionMode(0);			/* 0 = Single; 1 = Multi */
	//LCD_Clear();
	//LCD_String("single mode");
	//TCPClient_ApplicationMode(0);			/* 0 = Normal Mode; 1 = Transperant Mode */
	//AttachGPRS();
	//TCPClient_Connect(APN, USERNAME, PASSWORD);
	while(1)
	{
	
	_delay_ms(2000);
	SMS_Test();
	_delay_ms(2000);
		
	}
	
	return 0;
}


