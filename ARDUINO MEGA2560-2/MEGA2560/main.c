/*
 * MEGA2560.c
 *
 * Created: 20-Apr-22 10:06:29 AM
 * Author : MuhammadNaseem
*/ 

#define F_CPU 16000000UL

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sfr_defs.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "millis.h"
#include "uart.h"
#include "lcd.h"
#include "fp_uart.h"
#include "Adafruit_Fingerprint.h"

uint16_t template_cnt = 0;
int16_t finger_ID = -1;
uint32_t Home_Millis = 0;
bool clear = false;

// set pump on time 3000 means 3 second
#define pump_on_time  3000

#define key1_port     PORTE                   // key 1 pin
#define key1_bit      PE5
#define key1_ddr      DDRE

#define key2_port     PORTE                   // key 2 pin
#define key2_bit      PE4
#define key2_ddr      DDRE

#define key3_port     PORTE                   // key 3 pin
#define key3_bit      PE3
#define key3_ddr      DDRE

#define key4_port     PORTG                   // key 4 pin
#define key4_bit      PG5
#define key4_ddr      DDRG

// check for key press
#define is_key1_press	((PINE >> key1_bit) & 0x01)
#define is_key2_press	((PINE >> key2_bit) & 0x01)
#define is_key3_press	((PINE >> key3_bit) & 0x01)
#define is_key4_press	((PING >> key4_bit) & 0x01)

// relay / valves or pumps pins
#define pump1_port     PORTA                   // relay 1 pin
#define pump1_bit      PA1
#define pump1_ddr      DDRA

#define pump2_port     PORTA                   // relay 2 pin
#define pump2_bit      PA3
#define pump2_ddr      DDRA

#define pump3_port     PORTA                   // relay 3 pin
#define pump3_bit      PA5
#define pump3_ddr      DDRA

#define pump4_port     PORTA                   // relay 4 pin
#define pump4_bit      PA7
#define pump4_ddr      DDRA

void config_keypad(void);

void config_keypad(void)
{
	// configure the microprocessor pins for inputs
	key1_ddr &= ~(1<<key1_bit);
	key2_ddr &= ~(1<<key2_bit);
	key3_ddr &= ~(1<<key3_bit);
	key4_ddr &= ~(1<<key4_bit);
	// enable pull up resistors
	key1_port |= (1<<key1_bit);
	key2_port |= (1<<key2_bit);
	key3_port |= (1<<key3_bit);
	key4_port |= (1<<key4_bit);

	// writing high on relay port to make it off initially
	pump1_port |= (1<<pump1_bit);
	pump2_port |= (1<<pump2_bit);
	pump3_port |= (1<<pump3_bit);
	pump4_port |= (1<<pump4_bit);
	// configuring pins as a output
	pump1_ddr |= (1<<pump1_bit);
	pump2_ddr |= (1<<pump2_bit);
	pump3_ddr |= (1<<pump3_bit);
	pump4_ddr |= (1<<pump4_bit);
}

void manage_eeprom(uint8_t finger_ID)
{
	char lcd_buffer[20];
	uint8_t id = finger_ID - 1;
	uint16_t base_add = id * 2;

	// base address is address in eeprom
	// which contains the drink choice
	// and base + 1 contains remaining turns

	uint8_t choice = eeprom_read_byte((uint8_t *)(base_add));
	uint8_t turns  = eeprom_read_byte((uint8_t *)(base_add + 1));

	if(turns > 0) // turns are there
	{
		lcd_msg((uint8_t *)" Opening Valve ",lcd_LineOne);
		lcd_msg((uint8_t *)"w.r.t to choice",lcd_LineTwo);
		// open valve for set amount of time according to choice
		switch(choice)
		{
			case 1:
				pump1_port &= ~(1<<pump1_bit);	// on relay on low logic
				_delay_ms(pump_on_time);		// delay
				pump1_port |= (1<<pump1_bit);	// off the relay on high logic
			break;

			case 2:
				pump2_port &= ~(1<<pump2_bit);	// on relay on low logic
				_delay_ms(pump_on_time);		// delay
				pump2_port |= (1<<pump2_bit);	// off the relay on high logic
			break;

			case 3:
				pump3_port &= ~(1<<pump3_bit);	// on relay on low logic
				_delay_ms(pump_on_time);		// delay
				pump3_port |= (1<<pump3_bit);	// off the relay on high logic
			break;

			case 4:
				pump4_port &= ~(1<<pump4_bit);	// on relay on low logic
				_delay_ms(pump_on_time);		// delay
				pump4_port |= (1<<pump4_bit);	// off the relay on high logic
			break;

			default:
			break;
		}
		// update turn in eeprom
		eeprom_update_byte((uint8_t *)(base_add + 1),(turns - 1));
		lcd_msg((uint8_t *)"Remaining Tries",lcd_LineOne);
		snprintf(lcd_buffer,sizeof(lcd_buffer),"        %i       ",(turns - 1));
		lcd_msg((uint8_t *)lcd_buffer,lcd_LineTwo);
	}
	else // turns expires
	{
		printf_P(PSTR("All Tries Expired\r\n"));
		lcd_msg((uint8_t *)" Tries Expired ",lcd_LineOne);
	}
}

void manage_choices(void)
{
	uint8_t id = getTemplateCount();
	uint16_t base_add = id * 2;
	uint8_t turns = 5;
	uint8_t choice = 0;
	char lcd_buffer[20];

	lcd_msg((uint8_t *)"  Enter Options ",lcd_LineOne);
	lcd_msg((uint8_t *)"Press 1,2,3 or 4",lcd_LineTwo);
	// wait here to press for key to consider a choice
	while (is_key1_press && is_key2_press && is_key3_press && is_key4_press);
	// see which pin is pressed
	if(!is_key1_press) choice = 1;
	if(!is_key2_press) choice = 2;
	if(!is_key3_press) choice = 3;
	if(!is_key4_press) choice = 4;

	snprintf(lcd_buffer,sizeof(lcd_buffer),"        %i       ",choice);
	lcd_msg((uint8_t *)lcd_buffer,lcd_LineOne);
	lcd_msg((uint8_t *)"  is selected   ",lcd_LineTwo);

	// update values in eeprom
	eeprom_update_byte((uint8_t *)(base_add),choice);
	eeprom_update_byte((uint8_t *)(base_add + 1),turns);
}

int main(void)
{
	config_keypad();			// configure keypad pins
	init_millis(16000000UL);	// frequency at which atmega2560 is running
	USART0Init();				// hardware serial port for 9600 bps
	lcd_init();					// initialize the 16*2 lcd
	_delay_ms(1000);
	fp_uartInit();				// finger print module for 57600 bps
	_delay_ms(5);
	sei();

	// for verification that module is connected
	if(verifyPassword())
	{
		lcd_msg((uint8_t *)"  FingerPrint  ",lcd_LineOne);
		lcd_msg((uint8_t *)"  Sensor Found ",lcd_LineTwo);
	}
	else
	{
		lcd_msg((uint8_t *)"  FingerPrint  ",lcd_LineOne);
		lcd_msg((uint8_t *)"   Not Found   ",lcd_LineTwo);
	}
	// read fp configurations
	read_config();
	// get already saved template
	template_cnt = getTemplateCount();
	
	// any time if user want to clear the database just uncomment the below line which is FP_emptyDatabase()
	//FP_emptyDatabase();

	// if user want to delete any id then uncomment below function deleteFingerprint(id) 
	// id is a number > 0
	//deleteFingerprint(id);

	Home_Millis = millis();
	
    // Replace with your application code
    while (1) 
    {
		if(template_cnt > 0) // if there are already saved finger prints in database
		{
			finger_ID = getFingerprintIDez();
			// the below is alternate function that can also be used
			//finger_ID = getFingerprintID();
			// its found a valid finger print
			if(finger_ID > 0)
			{
				// now check condition either selected drink can be issued or not
				manage_eeprom(finger_ID);
				clear = false;
				Home_Millis = millis();
			}
			_delay_ms(50);
		}
		// if any key is press it is consider to register
		if(!(is_key1_press && is_key2_press && is_key3_press && is_key4_press))
		{
			// check either id already exist
			if(getFingerprintIDez() < 0)
			{			
				// it means user want to enroll finger
				if(getFingerprintEnroll() == true) // if successful
				{
					manage_choices();
					template_cnt = getTemplateCount();
				}
			}
			else // if already exist
			{				
				lcd_msg((uint8_t *)"  User Already  ",lcd_LineOne);
				lcd_msg((uint8_t *)"   Registered   ",lcd_LineTwo);		
			}
			clear = false;
			_delay_ms(2000);
		}
		
		if((millis() - Home_Millis) >= 2000) // update lcd after 2 seconds
		{
			if(!clear)
			{
				lcd_msg((uint8_t *)" 16*2 LCD Demo  ",lcd_LineOne);				
				clear = true;
			}
			lcd_msg((uint8_t *)" with FP Module ",lcd_LineTwo);
			Home_Millis = millis();
		}

		if(getch())
		{
			printf_P(PSTR("clearing all finger prints\r\n"));
			lcd_msg((uint8_t *)"Clearing Memory ",lcd_LineOne);
			Home_Millis = millis();
		}
		_delay_ms(10);		  
    }
}

