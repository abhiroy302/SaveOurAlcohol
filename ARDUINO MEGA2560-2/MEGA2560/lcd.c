
#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h"

// Program ID
#define program_str1 (uint8_t *)" 16*2 LCD Demo  "
#define program_str2 (uint8_t *)" with FP Module "
#define program_str3 (uint8_t *)" April 20, 2022 "

/******************************* Main Program Code *************************/
void lcd_init(void)
{
	// configure the microprocessor pins for the data lines
    lcd_D7_ddr |= (1<<lcd_D7_bit);                  // 4 data lines - output
    lcd_D6_ddr |= (1<<lcd_D6_bit);
    lcd_D5_ddr |= (1<<lcd_D5_bit);
    lcd_D4_ddr |= (1<<lcd_D4_bit);

	// configure the microprocessor pins for the control lines
    lcd_E_ddr |= (1<<lcd_E_bit);                    // E line - output
    lcd_RS_ddr |= (1<<lcd_RS_bit);                  // RS line - output
	
	_delay_ms(500);
	// initialize the LCD controller as determined by the defines (LCD instructions)
    lcd_init_4d();                                  // initialize the LCD display for a 4-bit interface

	// set cursor to start of first line and display the information
	lcd_msg(program_str1,lcd_LineOne);

	// set cursor to start of second line and display the information
	lcd_msg(program_str2,lcd_LineTwo);
}

/******************************* End of Main Program Code ******************/

/*============================== 4-bit LCD Functions ======================*/
/*
  Name:     lcd_init_4d
  Purpose:  initialize the LCD module for a 4-bit data interface
  Entry:    equates (LCD instructions) set up for the desired operation
  Exit:     no parameters
  Notes:    uses time delays rather than checking the busy flag
*/
void lcd_init_4d(void)
{
	// Power-up delay
    _delay_ms(100);                                 // initial 40 mSec delay

	// Set up the RS and E lines for the 'lcd_write_4' subroutine.
    lcd_RS_port &= ~(1<<lcd_RS_bit);                // select the Instruction Register (RS low)
    lcd_E_port &= ~(1<<lcd_E_bit);                  // make sure E is initially low

	// Reset the LCD controller
    lcd_write_4(lcd_FunctionReset);                 // first part of reset sequence
    _delay_ms(10);                                  // 4.1 mS delay (min)

    lcd_write_4(lcd_FunctionReset);                 // second part of reset sequence
    _delay_us(200);                                 // 100uS delay (min)

    lcd_write_4(lcd_FunctionReset);                 // third part of reset sequence
    _delay_us(200);                                 // this delay is omitted in the data sheet

	// Preliminary Function Set instruction - used only to set the 4-bit mode.
	// The number of lines or the font cannot be set at this time since the controller is still in the
	//  8-bit mode, but the data transfer mode can be changed since this parameter is determined by one 
	//  of the upper four bits of the instruction.
 
    lcd_write_4(lcd_FunctionSet4bit);               // set 4-bit mode
    _delay_us(80);                                  // 40uS delay (min)

	// Function Set instruction
    lcd_write_instruction_4d(lcd_FunctionSet4bit);   // set mode, lines, and font
    _delay_us(80);                                   // 40uS delay (min)

	// The next three instructions are specified in the data sheet as part of the initialization routine, 
	//  so it is a good idea (but probably not necessary) to do them just as specified and then redo them 
	//  later if the application requires a different configuration.

	// Display On/Off Control instruction
    lcd_write_instruction_4d(lcd_DisplayOff);        // turn display OFF
    _delay_us(80);                                   // 40uS delay (min)

	// Clear Display instruction
    lcd_write_instruction_4d(lcd_Clear);             // clear display RAM
    _delay_ms(4);                                    // 1.64 mS delay (min)

	// ; Entry Mode Set instruction
    lcd_write_instruction_4d(lcd_EntryMode);         // set desired shift characteristics
    _delay_us(80);                                   // 40uS delay (min)

	// This is the end of the LCD controller initialization as specified in the data sheet, but the display
	//  has been left in the OFF condition.  This is a good time to turn the display back ON.
 
	// Display On/Off Control instruction
    lcd_write_instruction_4d(lcd_DisplayOn);        // turn the display ON
    _delay_us(80);                                  // 40uS delay (min)
}

/*...........................................................................
  Name:     lcd_write_string_4d
; Purpose:  display a string of characters on the LCD
  Entry:    (theString) is the string to be displayed
  Exit:     no parameters
  Notes:    uses time delays rather than checking the busy flag
*/
void lcd_write_string_4d(uint8_t theString[])
{
    uint8_t i = 0;                             // character counter*/
    while (theString[i] != 0)
    {
        lcd_write_character_4d(theString[i]);
        i++;
        _delay_us(80);                              // 40 uS delay (min)
    }
}

/*...........................................................................
  Name:     lcd_write_character_4d
  Purpose:  send a byte of information to the LCD data register
  Entry:    (theData) is the information to be sent to the data register
  Exit:     no parameters
  Notes:    does not deal with RW (busy flag is not implemented)
*/

void lcd_write_character_4d(uint8_t theData)
{
    lcd_RS_port |= (1<<lcd_RS_bit);                 // select the Data Register (RS high)
    lcd_E_port &= ~(1<<lcd_E_bit);                  // make sure E is initially low
    lcd_write_4(theData);                           // write the upper 4-bits of the data
    lcd_write_4(theData << 4);                      // write the lower 4-bits of the data
}

/*...........................................................................
  Name:     lcd_write_instruction_4d
  Purpose:  send a byte of information to the LCD instruction register
  Entry:    (theInstruction) is the information to be sent to the instruction register
  Exit:     no parameters
  Notes:    does not deal with RW (busy flag is not implemented)
*/
void lcd_write_instruction_4d(uint8_t theInstruction)
{
    lcd_RS_port &= ~(1<<lcd_RS_bit);                // select the Instruction Register (RS low)
    lcd_E_port &= ~(1<<lcd_E_bit);                  // make sure E is initially low
    lcd_write_4(theInstruction);                    // write the upper 4-bits of the data
    lcd_write_4(theInstruction << 4);               // write the lower 4-bits of the data
}

/*...........................................................................
  Name:     lcd_write_4
  Purpose:  send a byte of information to the LCD module
  Entry:    (theByte) is the information to be sent to the desired LCD register
            RS is configured for the desired LCD register
            E is low
            RW is low
  Exit:     no parameters
  Notes:    use either time delays or the busy flag
*/
void lcd_write_4(uint8_t theByte)
{
    lcd_D7_port &= ~(1<<lcd_D7_bit);                        // assume that data is '0'
    if (theByte & 1<<7) lcd_D7_port |= (1<<lcd_D7_bit);     // make data = '1' if necessary

    lcd_D6_port &= ~(1<<lcd_D6_bit);                        // repeat for each data bit
    if (theByte & 1<<6) lcd_D6_port |= (1<<lcd_D6_bit);

    lcd_D5_port &= ~(1<<lcd_D5_bit);
    if (theByte & 1<<5) lcd_D5_port |= (1<<lcd_D5_bit);

    lcd_D4_port &= ~(1<<lcd_D4_bit);
    if (theByte & 1<<4) lcd_D4_port |= (1<<lcd_D4_bit);

	// write the data
                                                    // 'Address set-up time' (40 nS)
    lcd_E_port |= (1<<lcd_E_bit);                   // Enable pin high
    _delay_us(10);                                   // implement 'Data set-up time' (80 nS) and 'Enable pulse width' (230 nS)
    lcd_E_port &= ~(1<<lcd_E_bit);                  // Enable pin low
    _delay_us(10);                                   // implement 'Data hold time' (10 nS) and 'Enable cycle time' (500 nS)
}


void lcd_msg (uint8_t str[],uint8_t line) 
{
	uint8_t line_no = lcd_LineTwo;
	if(line == lcd_LineOne)  
	{
		// Clear Display instruction
		lcd_write_instruction_4d(lcd_Clear);             // clear display RAM
		_delay_ms(4);                                    // 1.64 mS delay (min)
		line_no = lcd_LineOne;
	}
	lcd_write_instruction_4d(line_no);
	 _delay_us(80);										// 40 uS delay (min)
	lcd_write_string_4d(str);
}