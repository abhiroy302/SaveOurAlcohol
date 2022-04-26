#ifndef _UART_H_
#define _UART_H_

#include <stdio.h>
#include <stdbool.h>

#define F_CPU 16000000UL

// hardware serial port for debugging purpose
#define USART_BAUDRATE 9600
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

void USART0Init(void);
int USART0SendByte(char u8Data, FILE *stream);
int USART0ReceiveByte(FILE *stream);
bool getch(void);
bool kbhit (void);
void Stream_Test(void);

#endif
