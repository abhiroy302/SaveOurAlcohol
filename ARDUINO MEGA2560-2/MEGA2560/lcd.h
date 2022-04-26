#ifndef _LCD_H_
#define _LCD_H_

// LCD interface (should agree with the diagram above)
//   make sure that the LCD RW pin is connected to GND
#define lcd_D7_port     PORTB                   // lcd D7 connection
#define lcd_D7_bit      PB7
#define lcd_D7_ddr      DDRB

#define lcd_D6_port     PORTB                   // lcd D6 connection
#define lcd_D6_bit      PB6
#define lcd_D6_ddr      DDRB

#define lcd_D5_port     PORTB                   // lcd D5 connection
#define lcd_D5_bit      PB5
#define lcd_D5_ddr      DDRB

#define lcd_D4_port     PORTB                   // lcd D4 connection
#define lcd_D4_bit      PB4
#define lcd_D4_ddr      DDRB

#define lcd_E_port      PORTH                   // lcd Enable pin
#define lcd_E_bit       PH6
#define lcd_E_ddr       DDRH

#define lcd_RS_port     PORTH                   // lcd Register Select pin
#define lcd_RS_bit      PH5
#define lcd_RS_ddr      DDRH

// LCD module information
#define lcd_LineOne     0x80                    // start of line 1
#define lcd_LineTwo     0xC0                    // start of line 2

// LCD instructions
#define lcd_Clear           0b00000001          // replace all characters with ASCII 'space'
#define lcd_Home            0b00000010          // return cursor to first position on first line
#define lcd_EntryMode       0b00000110          // shift cursor from left to right on read/write
#define lcd_DisplayOff      0b00001000          // turn display off
#define lcd_DisplayOn       0b00001100          // display on, cursor off, don't blink character
#define lcd_FunctionReset   0b00110000          // reset the LCD
#define lcd_FunctionSet4bit 0b00101000          // 4-bit data, 2-line display, 5 x 7 font
#define lcd_SetCursor       0b10000000          // set cursor position

// Function Prototypes
void lcd_init(void);
void lcd_write_4(uint8_t);
void lcd_write_instruction_4d(uint8_t);
void lcd_write_character_4d(uint8_t);
void lcd_write_string_4d(uint8_t *);
void lcd_msg (uint8_t *,uint8_t line);
void lcd_init_4d(void);

#endif

