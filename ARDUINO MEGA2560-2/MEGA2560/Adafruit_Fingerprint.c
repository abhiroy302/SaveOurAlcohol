#define F_CPU 16000000UL

#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "Adafruit_Fingerprint.h"
#include "fp_uart.h"
#include "millis.h"
#include "lcd.h"

/// The matching location that is set by fingerFastSearch()
uint16_t fingerID;
/// The confidence of the fingerFastSearch() match, higher numbers are more confidents
uint16_t confidence;
/// The number of stored templates in the sensor, set by getTemplateCount()
uint16_t templateCount;

uint16_t status_reg = 0x0;		// The status register (set by getParameters)
uint16_t system_id = 0x0;		// The system identifier (set by getParameters)
uint16_t capacity = 64;			// The fingerprint capacity (set by getParameters)
uint16_t security_level = 0;	// The security level (set by getParameters)
uint32_t device_addr =	0xFFFFFFFF;	// The device address (set by getParameters)
uint16_t packet_len = 64;		// The max packet length (set by getParameters)
uint16_t baud_rate = 57600;		// The UART baud rate (set by getParameters)
uint32_t thePassword = 0;

// Helper class to craft UART packets
typedef struct
{
	uint16_t start_code;	//< "Wakeup" code for packet detection
	uint8_t address[4];		//< 32-bit Fingerprint sensor address
	uint8_t type;			//< Type of packet
	uint16_t length;		//< Length of packet
	uint8_t data[64];		//< The raw buffer for packet payload
} FP;

FP packet = {0};

uint8_t deleteFingerprint(uint8_t id) 
{
	uint8_t p = -1;

	printf_P(PSTR("Deleting ID #"));
	printf_P(PSTR("%i\r\n"),id);

	p = FP_deleteModel(id);

	if (p == FINGERPRINT_OK) {
		printf_P(PSTR("Deleted!\r\n"));
		} else if (p == FINGERPRINT_PACKETRECIEVEERR) {
		printf_P(PSTR("Communication error\r\n"));
		} else if (p == FINGERPRINT_BADLOCATION) {
		printf_P(PSTR("Could not delete in that location\r\n"));
		} else if (p == FINGERPRINT_FLASHERR) {
		printf_P(PSTR("Error writing to flash\r\n)"));
		} else {
		printf_P(PSTR("Unknown error: 0x")); printf_P(PSTR("%x\r\n"),p);
	}

	return p;
}

int16_t getFingerprintID(void) 
{
	uint8_t p = FP_getImage();
	switch (p) {
		case FINGERPRINT_OK:
		printf_P(PSTR("Image taken\r\n"));
		break;
		case FINGERPRINT_NOFINGER:
		printf_P(PSTR("No finger detected\r\n"));
		return -1;
		case FINGERPRINT_PACKETRECIEVEERR:
		printf_P(PSTR("Communication error\r\n"));
		return -1;
		case FINGERPRINT_IMAGEFAIL:
		printf_P(PSTR("Imaging error\r\n"));
		return -1;
		default:
		printf_P(PSTR("Unknown error\r\n"));
		return -1;
	}

	// OK success!
	p = FP_image2Tz(1);
	switch (p) {
		case FINGERPRINT_OK:
		printf_P(PSTR("Image converted\r\n"));
		break;
		case FINGERPRINT_IMAGEMESS:
		printf_P(PSTR("Image too messy\r\n"));
		return -1;
		case FINGERPRINT_PACKETRECIEVEERR:
		printf_P(PSTR("Communication error\r\n"));
		return -1;
		case FINGERPRINT_FEATUREFAIL:
		printf_P(PSTR("Could not find fingerprint features\r\n"));
		return -1;
		case FINGERPRINT_INVALIDIMAGE:
		printf_P(PSTR("Could not find fingerprint features\r\n"));
		return -1;
		default:
		printf_P(PSTR("Unknown error\r\n"));
		return -1;
	}

	// OK converted!
	p = FP_fingerSearch(1);
	if (p == FINGERPRINT_OK) {
		printf_P(PSTR("Found a print match!\r\n"));
		} else if (p == FINGERPRINT_PACKETRECIEVEERR) {
		printf_P(PSTR("Communication error\r\n"));
		return -1;
		} else if (p == FINGERPRINT_NOTFOUND) {
		printf_P(PSTR("Did not find a match\r\n"));
		return -1;
		} else {
		printf_P(PSTR("Unknown error\r\n"));
		return -1;
	}

	// found a match!
	printf_P(PSTR("Found ID # ")); printf_P(PSTR("%i\r\n"),fingerID);
	printf_P(PSTR(" with confidence of ")); printf_P(PSTR("%i\r\n"),confidence);

	return fingerID;
}

// returns -1 if failed, otherwise returns ID #
int16_t getFingerprintIDez(void) 
{
	uint8_t p = FP_getImage();
	if (p != FINGERPRINT_OK)  return -1;

	p = FP_image2Tz(1);
	if (p != FINGERPRINT_OK)  return -1;

	p = FP_fingerFastSearch();
	if (p != FINGERPRINT_OK)  return -1;

	// found a match!
	printf_P(PSTR("Found ID # ")); printf_P(PSTR("%i\r\n"),fingerID);
	printf_P(PSTR(" with confidence of ")); printf_P(PSTR("%i\r\n"),confidence);
	return fingerID;
}


bool getFingerprintEnroll(void) 
{
	char lcd_buffer[20];
	int16_t p = -1;
	uint8_t id = getTemplateCount() + 1;
	uint32_t now = millis();
	// place finger in 10 seconds
	uint32_t timeout = 10000;
	printf_P(PSTR("Waiting for valid finger to enroll as #")); printf_P(PSTR("%x\r\n"),id);
	lcd_msg((uint8_t *)" Place a Finger ",lcd_LineOne);

	snprintf(lcd_buffer,sizeof(lcd_buffer)," in %02lu Seconds    ",(timeout/1000));
	lcd_msg((uint8_t *)lcd_buffer,lcd_LineTwo);

	while (p != FINGERPRINT_OK) {
		p = FP_getImage();
		// timeout implementation
		if((millis() - now) > timeout) 
		{
			lcd_msg((uint8_t *)" TimeOut Occur ",lcd_LineOne);
			return false;
		}
		switch (p) 
		{
			case FINGERPRINT_OK:
			printf_P(PSTR("Image taken\r\n"));
			break;
			case FINGERPRINT_NOFINGER:
			printf_P(PSTR(".\r\n"));
			break;
			case FINGERPRINT_PACKETRECIEVEERR:
			printf_P(PSTR("Communication error\r\n"));
			break;
			case FINGERPRINT_IMAGEFAIL:
			printf_P(PSTR("Imaging error\r\n"));
			break;
			default:
			printf_P(PSTR("Unknown error\r\n"));
			break;
		}
	}

	// OK success!
	p = FP_image2Tz(1);
	switch (p) {
		case FINGERPRINT_OK:
		printf_P(PSTR("Image converted\r\n"));
		break;
		case FINGERPRINT_IMAGEMESS:
		printf_P(PSTR("Image too messy\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
		case FINGERPRINT_PACKETRECIEVEERR:
		printf_P(PSTR("Communication error\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
		case FINGERPRINT_FEATUREFAIL:
		printf_P(PSTR("Could not find fingerprint features\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
		case FINGERPRINT_INVALIDIMAGE:
		printf_P(PSTR("Could not find fingerprint features\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
		default:
		printf_P(PSTR("Unknown error\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
	}

	printf_P(PSTR("Remove finger\r\n"));
	lcd_msg((uint8_t *)" Remove Finger  ",lcd_LineOne);
	_delay_ms(2000);
	p = 0;
	while (p != FINGERPRINT_NOFINGER) {
		p = FP_getImage();
	}
	printf_P(PSTR("ID ")); printf_P(PSTR("%i\r\n"),id);
	p = -1;
	printf_P(PSTR("Place same finger again\r\n"));
	lcd_msg((uint8_t *)"  Place Same    ",lcd_LineOne);
	lcd_msg((uint8_t *)"  Finger Again  ",lcd_LineTwo);
	while (p != FINGERPRINT_OK) {
		p = FP_getImage();
		switch (p) {
			case FINGERPRINT_OK:
			printf_P(PSTR("Image taken\r\n"));
			break;
			case FINGERPRINT_NOFINGER:
			printf_P(PSTR(".\r\n"));
			break;
			case FINGERPRINT_PACKETRECIEVEERR:
			printf_P(PSTR("Communication error\r\n"));
			break;
			case FINGERPRINT_IMAGEFAIL:
			printf_P(PSTR("Imaging error\r\n"));
			break;
			default:
			printf_P(PSTR("Unknown error\r\n"));
			break;
		}
	}

	// OK success!
	p = FP_image2Tz(2);
	switch (p) {
		case FINGERPRINT_OK:
		printf_P(PSTR("Image converted\r\n"));
		break;
		case FINGERPRINT_IMAGEMESS:
		printf_P(PSTR("Image too messy\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
		case FINGERPRINT_PACKETRECIEVEERR:
		printf_P(PSTR("Communication error\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
		case FINGERPRINT_FEATUREFAIL:
		printf_P(PSTR("Could not find fingerprint features\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
		case FINGERPRINT_INVALIDIMAGE:
		printf_P(PSTR("Could not find fingerprint features\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
		default:
		printf_P(PSTR("Unknown error\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
	}

	// OK converted!
	printf_P(PSTR("Creating model for # "));  printf_P(PSTR("%i\r\n"),id);

	p = FP_createModel();
	if (p == FINGERPRINT_OK) {
		printf_P(PSTR("Prints matched!\r\n"));
		} else if (p == FINGERPRINT_PACKETRECIEVEERR) {
		printf_P(PSTR("Communication error\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
		} else if (p == FINGERPRINT_ENROLLMISMATCH) {
		printf_P(PSTR("Fingerprints did not match\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
		} else {
		printf_P(PSTR("Unknown error\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
	}

	printf_P(PSTR("ID ")); printf_P(PSTR("%i\r\n"),id);
	p = FP_storeModel(id);
	if (p == FINGERPRINT_OK) {
		printf_P(PSTR("Stored!\r\n"));
		} else if (p == FINGERPRINT_PACKETRECIEVEERR) {
		printf_P(PSTR("Communication error\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
		} else if (p == FINGERPRINT_BADLOCATION) {
		printf_P(PSTR("Could not store in that location\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
		} else if (p == FINGERPRINT_FLASHERR) {
		printf_P(PSTR("Error writing to flash\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
		} else {
		printf_P(PSTR("Unknown error\r\n"));
		lcd_msg((uint8_t *)"  Error Occur  ",lcd_LineOne);
		return false;
	}

	lcd_msg((uint8_t *)"   Successfully  ",lcd_LineOne);
	lcd_msg((uint8_t *)"    Registered   ",lcd_LineTwo);
	_delay_ms(1000);
	return true;
}


bool verifyPassword(void)
{
	if (FP_verifyPassword())
	{
		printf_P(PSTR("Found fingerprint sensor!\r\n"));
		return true;
	}
	else
	{
		printf_P(PSTR("Did not find fingerprint sensor :(\r\n"));
		return false;
	}
}

uint16_t getTemplateCount(void)
{
	FP_getTemplateCount();	
	#ifdef FINGERPRINT_DEBUG
		printf_P(PSTR("Template Count: ")); printf_P(PSTR("%i\r\n"),templateCount);
	#endif
	return templateCount;		
}

void read_config(void)
{
	#ifdef FINGERPRINT_DEBUG	  
		printf_P(PSTR("Reading sensor parameters\r\n"));
		FP_getParameters();
		printf_P(PSTR("Status: 0x")); printf_P(PSTR("%x\r\n"),status_reg);
		printf_P(PSTR("Sys ID: 0x")); printf_P(PSTR("%x\r\n"),system_id);
		printf_P(PSTR("Capacity: ")); printf_P(PSTR("%i\r\n"),capacity);
		printf_P(PSTR("Security level: ")); printf_P(PSTR("%u\r\n"),security_level);
		printf_P(PSTR("Device address: ")); printf_P(PSTR("%lx\r\n"),device_addr);
		printf_P(PSTR("Packet len: ")); printf_P(PSTR("%i\r\n"),packet_len);
		printf_P(PSTR("Baud rate: ")); printf_P(PSTR("%u\r\n"),baud_rate);
	#endif
}

	
void Make_Packet(uint8_t type, uint16_t length, uint8_t *data) 
{
	packet.start_code = FINGERPRINT_STARTCODE;
	packet.type = type;
	packet.length = length;
	packet.address[0] = 0xFF;
	packet.address[1] = 0xFF;
	packet.address[2] = 0xFF;
	packet.address[3] = 0xFF;
	
	if (length < 64)
		memcpy(packet.data, data, length);
	else
		memcpy(packet.data, data, 64);
}

/*!
 * @brief Gets the command packet
 */
#define GET_CMD_PACKET(...)											\
  uint8_t data[] = {__VA_ARGS__};									\
  Make_Packet(FINGERPRINT_COMMANDPACKET, sizeof(data),data);		\
  _delay_ms(100);													\
  FP_writeStructuredPacket();										\
  if (FP_getStructuredPacket(DEFAULTTIMEOUT) != FINGERPRINT_OK)		\
    return FINGERPRINT_PACKETRECIEVEERR;							\
  if (packet.type != FINGERPRINT_ACKPACKET)							\
    return FINGERPRINT_PACKETRECIEVEERR;

/*!
 * @brief Sends the command packet
 */
#define SEND_CMD_PACKET(...)                                        \
  GET_CMD_PACKET(__VA_ARGS__);                                      \
  return packet.data[0];


/**************************************************************************/
/*!
    @brief  Verifies the sensors' access password (default password is
   0x0000000). A good way to also check if the sensors is active and responding
    @returns True if password is correct
*/
/**************************************************************************/
bool FP_verifyPassword(void) 
{
  return FP_checkPassword() == FINGERPRINT_OK;
}

uint8_t FP_checkPassword(void) 
{
  GET_CMD_PACKET(FINGERPRINT_VERIFYPASSWORD, (uint8_t)(thePassword >> 24),
                 (uint8_t)(thePassword >> 16), (uint8_t)(thePassword >> 8),
                 (uint8_t)(thePassword & 0xFF));
  if (packet.data[0] == FINGERPRINT_OK)
    return FINGERPRINT_OK;
  else
    return FINGERPRINT_PACKETRECIEVEERR;
}

/**************************************************************************/
/*!
    @brief  Get the sensors parameters, fills in the member variables
    status_reg, system_id, capacity, security_level, device_addr, packet_len
    and baud_rate
    @returns True if password is correct
*/
/**************************************************************************/
uint8_t FP_getParameters(void) 
{
  GET_CMD_PACKET(FINGERPRINT_READSYSPARAM);

  status_reg = ((uint16_t)packet.data[1] << 8) | packet.data[2];
  system_id = ((uint16_t)packet.data[3] << 8) | packet.data[4];
  capacity = ((uint16_t)packet.data[5] << 8) | packet.data[6];
  security_level = ((uint16_t)packet.data[7] << 8) | packet.data[8];
  device_addr = ((uint32_t)packet.data[9] << 24) |
                ((uint32_t)packet.data[10] << 16) |
                ((uint32_t)packet.data[11] << 8) | (uint32_t)packet.data[12];
  packet_len = ((uint16_t)packet.data[13] << 8) | packet.data[14];
  if (packet_len == 0) {
    packet_len = 32;
  } else if (packet_len == 1) {
    packet_len = 64;
  } else if (packet_len == 2) {
    packet_len = 128;
  } else if (packet_len == 3) {
    packet_len = 256;
  }
  baud_rate = (((uint16_t)packet.data[15] << 8) | packet.data[16]) * 9600;

  return packet.data[0];
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to take an image of the finger pressed on surface
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_NOFINGER</code> if no finger detected
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
    @returns <code>FINGERPRINT_IMAGEFAIL</code> on imaging error
*/
/**************************************************************************/
uint8_t FP_getImage(void) 
{
  SEND_CMD_PACKET(FINGERPRINT_GETIMAGE);
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to convert image to feature template
    @param slot Location to place feature template (put one in 1 and another in
   2 for verification to create model)
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_IMAGEMESS</code> if image is too messy
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
    @returns <code>FINGERPRINT_FEATUREFAIL</code> on failure to identify
   fingerprint features
    @returns <code>FINGERPRINT_INVALIDIMAGE</code> on failure to identify
   fingerprint features
*/
uint8_t FP_image2Tz(uint8_t slot) 
{
  SEND_CMD_PACKET(FINGERPRINT_IMAGE2TZ, slot);
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to take two print feature template and create a
   model
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
    @returns <code>FINGERPRINT_ENROLLMISMATCH</code> on mismatch of fingerprints
*/
uint8_t FP_createModel(void) 
{
  SEND_CMD_PACKET(FINGERPRINT_REGMODEL);
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to store the calculated model for later matching
    @param   location The model location #
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_BADLOCATION</code> if the location is invalid
    @returns <code>FINGERPRINT_FLASHERR</code> if the model couldn't be written
   to flash memory
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
uint8_t FP_storeModel(uint16_t location) 
{
  SEND_CMD_PACKET(FINGERPRINT_STORE, 0x01, (uint8_t)(location >> 8),
                  (uint8_t)(location & 0xFF));
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to load a fingerprint model from flash into buffer 1
    @param   location The model location #
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_BADLOCATION</code> if the location is invalid
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
uint8_t FP_loadModel(uint16_t location) 
{
  SEND_CMD_PACKET(FINGERPRINT_LOAD, 0x01, (uint8_t)(location >> 8),
                  (uint8_t)(location & 0xFF));
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to transfer 256-byte fingerprint template from the
   buffer to the UART
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
uint8_t FP_getModel(void) 
{
  SEND_CMD_PACKET(FINGERPRINT_UPLOAD, 0x01);
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to delete a model in memory
    @param   location The model location #
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_BADLOCATION</code> if the location is invalid
    @returns <code>FINGERPRINT_FLASHERR</code> if the model couldn't be written
   to flash memory
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
uint8_t FP_deleteModel(uint16_t location) 
{
  SEND_CMD_PACKET(FINGERPRINT_DELETE, (uint8_t)(location >> 8),
                  (uint8_t)(location & 0xFF), 0x00, 0x01);
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to delete ALL models in memory
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_BADLOCATION</code> if the location is invalid
    @returns <code>FINGERPRINT_FLASHERR</code> if the model couldn't be written
   to flash memory
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
uint8_t FP_emptyDatabase(void) 
{
  SEND_CMD_PACKET(FINGERPRINT_EMPTY);
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to search the current slot 1 fingerprint features to
   match saved templates. The matching location is stored in <b>fingerID</b> and
   the matching confidence in <b>confidence</b>
    @returns <code>FINGERPRINT_OK</code> on fingerprint match success
    @returns <code>FINGERPRINT_NOTFOUND</code> no match made
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
/**************************************************************************/
uint8_t FP_fingerFastSearch(void) 
{
  // high speed search of slot #1 starting at page 0x0000 and page #0x00A3
  GET_CMD_PACKET(FINGERPRINT_HISPEEDSEARCH, 0x01, 0x00, 0x00, 0x00, 0xA3);
  fingerID = 0xFFFF;
  confidence = 0xFFFF;

  fingerID = packet.data[1];
  fingerID <<= 8;
  fingerID |= packet.data[2];

  confidence = packet.data[3];
  confidence <<= 8;
  confidence |= packet.data[4];

  return packet.data[0];
}

/**************************************************************************/
/*!
    @brief   Control the built in LED
    @param on True if you want LED on, False to turn LED off
    @returns <code>FINGERPRINT_OK</code> on success
*/
/**************************************************************************/
uint8_t FP_LEDONOFF(bool on) 
{
  if (on) {
    SEND_CMD_PACKET(FINGERPRINT_LEDON);
  } else {
    SEND_CMD_PACKET(FINGERPRINT_LEDOFF);
  }
}

/**************************************************************************/
/*!
    @brief   Control the built in Aura LED (if exists). Check datasheet/manual
    for different colors and control codes available
    @param control The control code (e.g. breathing, full on)
    @param speed How fast to go through the breathing/blinking cycles
    @param coloridx What color to light the indicator
    @param count How many repeats of blinks/breathing cycles
    @returns <code>FINGERPRINT_OK</code> on fingerprint match success
    @returns <code>FINGERPRINT_NOTFOUND</code> no match made
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
/**************************************************************************/
uint8_t FP_LEDcontrol(uint8_t control, uint8_t speed,
                                         uint8_t coloridx, uint8_t count) {
  SEND_CMD_PACKET(FINGERPRINT_AURALEDCONFIG, control, speed, coloridx, count);
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to search the current slot fingerprint features to
   match saved templates. The matching location is stored in <b>fingerID</b> and
   the matching confidence in <b>confidence</b>
   @param slot The slot to use for the print search, defaults to 1
    @returns <code>FINGERPRINT_OK</code> on fingerprint match success
    @returns <code>FINGERPRINT_NOTFOUND</code> no match made
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
/**************************************************************************/
uint8_t FP_fingerSearch(uint8_t slot) {
  // search of slot starting thru the capacity
  GET_CMD_PACKET(FINGERPRINT_SEARCH, slot, 0x00, 0x00, (uint8_t)(capacity >> 8),
                 (uint8_t)(capacity & 0xFF));

  fingerID = 0xFFFF;
  confidence = 0xFFFF;

  fingerID = packet.data[1];
  fingerID <<= 8;
  fingerID |= packet.data[2];

  confidence = packet.data[3];
  confidence <<= 8;
  confidence |= packet.data[4];

  return packet.data[0];
}

/**************************************************************************/
/*!
    @brief   Ask the sensor for the number of templates stored in memory. The
   number is stored in <b>templateCount</b> on success.
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
/**************************************************************************/
uint8_t FP_getTemplateCount(void) {
  GET_CMD_PACKET(FINGERPRINT_TEMPLATECOUNT);

  templateCount = packet.data[1];
  templateCount <<= 8;
  templateCount |= packet.data[2];

  return packet.data[0];
}

/**************************************************************************/
/*!
    @brief   Set the password on the sensor (future communication will require
   password verification so don't forget it!!!)
    @param   password 32-bit password code
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
/**************************************************************************/
uint8_t FP_setPassword(uint32_t password) {
  SEND_CMD_PACKET(FINGERPRINT_SETPASSWORD, (uint8_t)(password >> 24),
                  (uint8_t)(password >> 16), (uint8_t)(password >> 8),
                  (uint8_t)(password & 0xFF));
}

/**************************************************************************/
/*!
    @brief   Helper function to process a packet and send it over UART to the
   sensor
    @param   packet A structure containing the bytes to transmit
*/
/**************************************************************************/

void FP_writeStructuredPacket(void) 
{
  fp_putch((uint8_t)(packet.start_code >> 8));
  fp_putch((uint8_t)(packet.start_code & 0xFF));
  fp_putch(packet.address[0]);
  fp_putch(packet.address[1]);
  fp_putch(packet.address[2]);
  fp_putch(packet.address[3]);
  fp_putch(packet.type);

  uint16_t wire_length = packet.length + 2;
  fp_putch((uint8_t)(wire_length >> 8));
  fp_putch((uint8_t)(wire_length & 0xFF));

#ifdef FINGERPRINT_DEBUG
  printf_P(PSTR("-> 0x"));
  printf_P(PSTR("%x"),(uint8_t)(packet.start_code >> 8));
  printf_P(PSTR(", 0x"));
  printf_P(PSTR("%x"), (uint8_t)(packet.start_code & 0xFF));
  printf_P(PSTR(", 0x"));
  printf_P(PSTR("%x"), packet.address[0]);
  printf_P(PSTR(", 0x"));
  printf_P(PSTR("%x"), packet.address[1]);
  printf_P(PSTR(", 0x"));
  printf_P(PSTR("%x"), packet.address[2]);
  printf_P(PSTR(", 0x"));
  printf_P(PSTR("%x"), packet.address[3]);
  printf_P(PSTR(", 0x"));
  printf_P(PSTR("%x"), packet.type);
  printf_P(PSTR(", 0x"));
  printf_P(PSTR("%x"),(uint8_t)(wire_length >> 8));
  printf_P(PSTR(", 0x"));
  printf_P(PSTR("%x"),(uint8_t)(wire_length & 0xFF));
#endif

  uint16_t sum = ((wire_length) >> 8) + ((wire_length)&0xFF) + packet.type;
  for (uint8_t i = 0; i < packet.length; i++) {
    fp_putch(packet.data[i]);
    sum += packet.data[i];
#ifdef FINGERPRINT_DEBUG
    printf_P(PSTR(", 0x"));
	printf_P(PSTR("%x"),packet.data[i]);
#endif
  }

  fp_putch((uint8_t)(sum >> 8));
  fp_putch((uint8_t)(sum & 0xFF));

#ifdef FINGERPRINT_DEBUG
  printf_P(PSTR(", 0x"));
  printf_P(PSTR("%x"),(uint8_t)(sum >> 8));
  printf_P(PSTR(", 0x"));
  printf_P(PSTR("%x\r\n"),(uint8_t)(sum & 0xFF));
#endif
}

/**************************************************************************/
/*!
    @brief   Helper function to receive data over UART from the sensor and
   process it into a packet
    @param   packet A structure containing the bytes received
    @param   timeout how many milliseconds we're willing to wait
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_TIMEOUT</code> or
   <code>FINGERPRINT_BADPACKET</code> on failure
*/
/**************************************************************************/
uint8_t FP_getStructuredPacket(uint16_t timeout) {
  uint8_t byte;
  uint16_t idx = 0, timer = 0;

#ifdef FINGERPRINT_DEBUG
  printf_P(PSTR("<- "));
#endif

  while (true) {
    while (!fp_available()) {
      _delay_ms(1);
      timer++;
      if (timer >= timeout) {
#ifdef FINGERPRINT_DEBUG
        printf_P(PSTR("Timed out\r\n"));
#endif
        return FINGERPRINT_TIMEOUT;
      }
    }
    byte = fp_read();
#ifdef FINGERPRINT_DEBUG
    printf_P(PSTR("0x"));
	printf_P(PSTR("%x"),byte);
    printf_P(PSTR(", "));
#endif
    switch (idx) {
    case 0:
      if (byte != (FINGERPRINT_STARTCODE >> 8))
        continue;
      packet.start_code = (uint16_t)byte << 8;
      break;
    case 1:
      packet.start_code |= byte;
      if (packet.start_code != FINGERPRINT_STARTCODE)
        return FINGERPRINT_BADPACKET;
      break;
    case 2:
    case 3:
    case 4:
    case 5:
      packet.address[idx - 2] = byte;
      break;
    case 6:
      packet.type = byte;
      break;
    case 7:
      packet.length = (uint16_t)byte << 8;
      break;
    case 8:
      packet.length |= byte;
      break;
    default:
      packet.data[idx - 9] = byte;
      if ((idx - 8) == packet.length) {
#ifdef FINGERPRINT_DEBUG
        printf_P(PSTR(" OK \r\n"));
#endif
        return FINGERPRINT_OK;
      }
      break;
    }
    idx++;
  }
  // Shouldn't get here so...
  return FINGERPRINT_BADPACKET;
}
