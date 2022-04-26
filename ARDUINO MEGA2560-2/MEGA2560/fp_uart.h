
#ifndef _FP_UART_H
#define _FP_UART_H


#define F_CPU 16000000UL

// hardware serial port for debugging purpose
#define FP_BAUDRATE 57600
#define UBRR1_VALUE (((F_CPU / (FP_BAUDRATE * 16UL))) - 1)

void fp_uartInit(void);					// Initialize UART
void fp_putch(uint8_t byte2transmit);	// Transmit a character
uint8_t fp_getch(void);							// Receive a character
void fp_string(const char* str);

// circular fifo
uint8_t fp_available(void);
uint8_t fp_fifo_length(void);
uint8_t fp_read(void);

#endif 


