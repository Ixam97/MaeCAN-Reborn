/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Maximilian Goldschmidt wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * 
 * ----------------------------------------------------------------------------
 * https://github.com/Ixam97
 * ----------------------------------------------------------------------------
 * [2021-03-13.1]
 */

#include "mcan.h"

static canFrame can_buffer[CANBUFFERSIZE];
static uint8_t can_buffer_read;
static uint8_t can_buffer_write;

uint32_t mcan_uid;
uint16_t mcan_hash;

#ifdef SLCAN
static const char asciilookup[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
static char slcan_buffer[25];
static uint8_t slcan_buffer_index;

uint8_t use_nl = 0;
#endif // SLCAN

/************************************************************************/
/* SPI                                                                  */
/************************************************************************/

static void spi_init(void) {
	//Activating SPI pins
	set_output(CS);
	set_output(MOSI);
	set_output(SCK);
	set_input(MISO);
	
	pullup_on(CS);
	pullup_on(MOSI);
	pullup_on(SCK);
	pullup_on(MISO);
	
	//Activating SPI master interface, fosc = fclk / 2
	SPCR = (1 << SPE) | (1 << MSTR);
	SPSR = (1 << SPI2X);
}


static uint8_t spi_putc(uint8_t data) {
	//Send a byte:
	SPDR = data;
	
	//Wait until byte is sent:
	while (!(SPSR & (1 << SPIF))) {};
	
	return SPDR;
}


/************************************************************************/
/* MCP2515                                                              */
/************************************************************************/

static void mcp1525_write_register(uint8_t adress, uint8_t data) {
	set_low(CS);
	
	spi_putc(SPI_WRITE);
	spi_putc(adress);
	spi_putc(data);
	
	set_high(CS);
}

/*
static uint8_t mcp2515_read_register(uint8_t adress) {
	uint8_t data;
	
	set_low(CS);
	
	spi_putc(SPI_READ);
	spi_putc(adress);
	
	data = spi_putc(0xff);
	
	set_high(CS);
	
	return data;
}
*/

static void mcp2515_bit_modify(uint8_t adress, uint8_t mask, uint8_t data) {
	set_low(CS);
	
	spi_putc(SPI_BIT_MODIFY);
	spi_putc(adress);
	spi_putc(mask);
	spi_putc(data);
	
	set_high(CS);
}

static uint8_t mcp2515_read_rx_status(void) {
	uint8_t data;
	
	set_low(CS);
	
	spi_putc(SPI_RX_STATUS);
	data = spi_putc(0xff);
	spi_putc(0xff);
	
	set_high(CS);
	
	return data;
}

void mcan_init(uint32_t _uid) {

	mcan_uid = _uid;
	mcan_hash = generateHash(_uid);
	
	spi_init();
	
	set_low(CS);
	spi_putc(SPI_RESET);
	_delay_us(100);
	set_high(CS);
	
	_delay_us(1000);
	
	mcp1525_write_register(CNF1, 0x41);
	mcp1525_write_register(CNF2, 0xf1);
	mcp1525_write_register(CNF3, 0x85);
	mcp1525_write_register(CANINTE, (1 << RX1IE) | (1 << RX0IE));
	mcp1525_write_register(RXB0CTRL, (1 << RXM1) | (1 << RXM0));
	mcp1525_write_register(RXB1CTRL, (1 << RXM1) | (1 << RXM0));
	
	mcp1525_write_register(RXM0SIDH, 0);
	mcp1525_write_register(RXM0SIDL, 0);
	mcp1525_write_register(RXM0EID8, 0);
	mcp1525_write_register(RXM0EID0, 0);
	
	mcp1525_write_register(RXM1SIDH, 0);
	mcp1525_write_register(RXM1SIDL, 0);
	mcp1525_write_register(RXM1EID8, 0);
	mcp1525_write_register(RXM1EID0, 0);
	
	mcp1525_write_register(BFPCTRL, 0);
	mcp1525_write_register(TXRTSCTRL, 0);
	mcp2515_bit_modify(CANCTRL, 0xe0, 0);
	
	// Setup ext. interrupt on falling edge:
	#if defined(__AVR_ATmega328P__)
	DDRD &= ~(1 << 2);
	PORTD |= (1 << 2);
	EICRA |= (1 << ISC00);
	EIMSK |= (1 << INT0);
	#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
	EICRA |= (1 << ISC21);
	EICRA &= ~(1 << ISC20);
	EIMSK |= (1 << INT2);
	DDRD &= ~(1 << 2);
	PORTD |= (1 << 2);
	#endif
	
	set_input(MCPINT);
	pullup_on(MCPINT);
	
	#ifdef SLCAN
	uart_init(UART_BAUD_SELECT(BAUD_RATE, F_CPU));
	#endif
	
}

/************************************************************************/
/* SLCAN                                                                */
/************************************************************************/
#ifdef SLCAN

static void sendSLCAN(canFrame *pFrame) {
	uint32_t _ID = (((uint32_t)pFrame->cmd << 17) | ((uint32_t)pFrame->resp << 16) | pFrame->hash);
		
	char message[] = {
		'T',
		/* i */ asciilookup[((_ID >> 28) & 0b1111)],
		/* i */ asciilookup[((_ID >> 24) & 0b1111)],
		/* i */ asciilookup[((_ID >> 20) & 0b1111)],
		/* i */ asciilookup[((_ID >> 16) & 0b1111)],
		/* i */ asciilookup[((_ID >> 12) & 0b1111)],
		/* i */ asciilookup[((_ID >> 8) & 0b1111)],
		/* i */ asciilookup[((_ID >> 4) & 0b1111)],
		/* i */ asciilookup[(_ID & 0b1111)],
		/* l */ asciilookup[pFrame->dlc],
		/* d */ asciilookup[((pFrame->data[0] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[0] & 0b1111)],
		/* d */ asciilookup[((pFrame->data[1] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[1] & 0b1111)],
		/* d */ asciilookup[((pFrame->data[2] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[2] & 0b1111)],
		/* d */ asciilookup[((pFrame->data[3] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[3] & 0b1111)],
		/* d */ asciilookup[((pFrame->data[4] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[4] & 0b1111)],
		/* d */ asciilookup[((pFrame->data[5] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[5] & 0b1111)],
		/* d */ asciilookup[((pFrame->data[6] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[6] & 0b1111)],
		/* d */ asciilookup[((pFrame->data[7] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[7] & 0b1111)]};
	// Tiiiiiiiildddddddddddddddd\r
	
	uart_putc(message[0]);
	uart_putc(message[1]);
	uart_putc(message[2]);
	uart_putc(message[3]);
	uart_putc(message[4]);
	uart_putc(message[5]);
	uart_putc(message[6]);
	uart_putc(message[7]);
	uart_putc(message[8]);
	uart_putc(message[9]);
	
	for (uint8_t i = 10; i < 10 + pFrame->dlc * 2; i++) {
		uart_putc(message[i]);
	}
	
	uart_putc(0x0d);
	if (use_nl == 1) uart_putc(0x0a);
	
	//uart_puts(message);
}

static uint8_t decodeAscii(char character) {
	switch (character) {
		case '1': return 0x01;
		case '2': return 0x02;
		case '3': return 0x03;
		case '4': return 0x04;
		case '5': return 0x05;
		case '6': return 0x06;
		case '7': return 0x07;
		case '8': return 0x08;
		case '9': return 0x09;
		case 'A': return 0x0a;
		case 'B': return 0x0b;
		case 'C': return 0x0c;
		case 'D': return 0x0d;
		case 'E': return 0x0e;
		case 'F': return 0x0f;
		case 'a': return 0x0a;
		case 'b': return 0x0b;
		case 'c': return 0x0c;
		case 'd': return 0x0d;
		case 'e': return 0x0e;
		case 'f': return 0x0f;
		case '0': return 0x00;
		default: return 0;
		
	}
}

static void getSLCAN(canFrame *frame) {
	uint32_t rxID = 0;
	
	rxID += (uint32_t)decodeAscii(slcan_buffer[0]) << 28;
	rxID += (uint32_t)decodeAscii(slcan_buffer[1]) << 24;
	rxID += (uint32_t)decodeAscii(slcan_buffer[2]) << 20;
	rxID += (uint32_t)decodeAscii(slcan_buffer[3]) << 16;
	rxID += (uint32_t)decodeAscii(slcan_buffer[4]) << 12;
	rxID += (uint32_t)decodeAscii(slcan_buffer[5]) << 8;
	rxID += (uint32_t)decodeAscii(slcan_buffer[6]) << 4;
	rxID += (uint32_t)decodeAscii(slcan_buffer[7]);
	frame->dlc = decodeAscii(slcan_buffer[8]);
	
	frame->cmd = (uint8_t)(rxID >> 17);
	frame->resp = (uint8_t)(rxID >> 16) & 0x01;
	frame->hash = (uint16_t)(rxID & 0xffff);
	
	for (uint8_t i = 0; i < frame->dlc; i++) {
		frame->data[i] = (decodeAscii(slcan_buffer[9+2*i]) << 4) + (decodeAscii(slcan_buffer[10+2*i]) & 0b1111);
	}
}

static uint8_t SLCANFrameReady(void) {
	uint16_t uart_in = uart_getc();
	
	if (uart_in >> 8 == 0 && (char)uart_in == 'T') {
		slcan_buffer_index = 0;
		} else if (uart_in >> 8 == 0 && ((char)uart_in == 0x0d || (char)uart_in == 0x0a) && slcan_buffer_index < 26) {
		if (slcan_buffer_index >= 9) {
			slcan_buffer_index = 25;
			} else {
			slcan_buffer_index = 26;
		}
		} else if (uart_in >> 8 == 0 && slcan_buffer_index < 25) {
		slcan_buffer[slcan_buffer_index] = (uint8_t)uart_in;
		slcan_buffer_index++;
		} else if (slcan_buffer_index == 25) {
		slcan_buffer_index++;
		return 1;
	}
	return 0;
}

#endif // SLCAN

void sendCanFrame(canFrame *frame) {
	sendCanFrameNoSLCAN(frame);
	
	#ifdef SLCAN
	sendSLCAN(frame);
	#endif // SLCAN
}


void sendCanFrameNoSLCAN(canFrame *frame) {
	uint32_t txID = (((uint32_t)frame->cmd << 17) | ((uint32_t)frame->resp << 16) | frame->hash);
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		
		set_low(CS);
		spi_putc(SPI_WRITE_TX);
	
		// Set frame ID
		spi_putc((uint8_t)((txID >> 16) >> 5));
		spi_putc(((uint8_t)((txID >> 16) & 0x03)) | 0x08 | (uint8_t)(((txID >> 16) & 0x1c) << 3));
		spi_putc((uint8_t)(txID >> 8));
		spi_putc((uint8_t)(txID & 0xff));
	
		// Set frame dlc
		spi_putc(frame->dlc);
	
		// Set frame data
		for (uint8_t i = 0; i < 8; i++) {
			spi_putc(frame->data[i]);
		}
		set_high(CS);
	
		//Send CAN Frame:
		set_low(CS);
		spi_putc(SPI_RTS | 0x01);
		set_high(CS);
	}
	_delay_us(1000);
}

static uint8_t getCanFrame(canFrame *frame) {
	
	uint32_t rxID;
	uint8_t status = mcp2515_read_rx_status();
	
	if(status & (1 << 6)) {
		set_low(CS);
		spi_putc(SPI_READ_RX);
		} else if (status & (1 << 7)) {
		set_low(CS);
		spi_putc(SPI_READ_RX | 0x04);
		} else {
		return 0xff;
	}
	
	rxID = (uint32_t) (spi_putc(0xff) << 3);
	uint8_t sidl = spi_putc(0xff);
	rxID |= sidl >> 5;
	rxID = (rxID << 2) | (sidl & 0x03);
	rxID = (rxID << 8) | spi_putc(0xff);
	rxID = (rxID << 8) | spi_putc(0xff);
	
	frame->cmd = (uint8_t)(rxID >> 17);
	frame->resp = (uint8_t)(rxID >> 16) & 0x01;
	frame->hash = (uint16_t)(rxID & 0xffff);
	
	frame->dlc = spi_putc(0xff) & 0x0f;
	
	for (uint8_t i = 0; i < 8; i++) {
		frame->data[i] = spi_putc(0xff);
	}
	
	set_high(CS);
	
	if (status & ( 1 << 6)) {
		mcp2515_bit_modify(CANINTF, (1 << RX0IF), 0);
		} else {
		mcp2515_bit_modify(CANINTF, (1 << RX1IF), 0);
	}
	return (status & 0x07);
}

static uint8_t CanBufferIn(void) {
	uint8_t next = (can_buffer_write + 1) & CANBUFFERMASK;
	
	if (can_buffer_read == next)
	return BUFFER_FAIL; // Full
	
	getCanFrame(&can_buffer[can_buffer_write & CANBUFFERMASK]);
	
	can_buffer_write = next;
	
	return BUFFER_SUCCESS;
}

static uint8_t CanBufferOut(canFrame *pFrame) {
	if (can_buffer_read == can_buffer_write)
	return BUFFER_FAIL; // Empty
	
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		*pFrame = can_buffer[can_buffer_read];
	}
		
	can_buffer_read++;
	if (can_buffer_read >= CANBUFFERSIZE)
	can_buffer_read = 0;
	
	return BUFFER_SUCCESS;
}

uint8_t readCanFrame(canFrame *pFrame) {
	
	if (CanBufferOut(pFrame) == 1) {
		
		#ifdef SLCAN
		sendSLCAN(pFrame);
		#endif // SLCAN
		
		return 1;
	}
	
	#if SLCAN
	if (SLCANFrameReady() == 1) {
		getSLCAN(pFrame);
		sendCanFrameNoSLCAN(pFrame);
		return 1;
	}
	#endif // SLCAN
	
	return 0;
	
}

/************************************************************************/
/* Märklin CAN                                                          */
/************************************************************************/

uint16_t generateHash(uint32_t _uid) {
	uint16_t uid_high = (uint16_t)(_uid >> 16);
	uint16_t uid_low  = (uint16_t)(_uid);
	
	uint16_t _hash = uid_high ^ uid_low;
	_hash = (_hash | 0x300) & ~0x80;
	
	return _hash;
}

uint8_t compareUID(uint8_t _data[8], uint32_t _uid) {

	if ((_data[0] == (uint8_t)(_uid >> 24)) && (_data[1] == (uint8_t)(_uid >> 16)) && (_data[2] == (uint8_t)(_uid >> 8)) && (_data[3] == (uint8_t)_uid)) {
		return 1;
		} else {
		return 0;
	}
}

/*
 * sendDeviceInfo: Sending basic device information to GUI
 */
void sendDeviceInfo(uint32_t serial_nbr, uint8_t measure_ch, uint8_t config_ch, char *art_nbr, char *name) {
	canFrame frame;
	
	frame.cmd = CMD_CONFIG;
	frame.resp = 1;
	frame.hash = 0x301;
	frame.dlc = 8;
	frame.data[0] = measure_ch; //number of measurement channels
	frame.data[1] = config_ch;   //number of configuration channels
	frame.data[2] = 0;
	frame.data[3] = 0;
	frame.data[4] = (uint8_t) (serial_nbr >> 24); //serial number
	frame.data[5] = (uint8_t) (serial_nbr >> 16);
	frame.data[6] = (uint8_t) (serial_nbr >> 8);
	frame.data[7] = (uint8_t) serial_nbr;
	
	sendCanFrame(&frame);
	
	
	uint8_t art_nbr_len = 0;
	
	while(1) {
		if (art_nbr[art_nbr_len] == 0) break;
		else art_nbr_len++;
	}
	
	for (uint8_t i = 0; i < 8; i++) {
		if (i < art_nbr_len) frame.data[i] = art_nbr[i];
		else frame.data[i] = 0;
	}
	frame.hash++;
	sendCanFrame(&frame);
	
	uint8_t name_len = 0;
	uint8_t needed_frames = 0;
	
	while(1) {
		if (name[name_len] == 0) break;
		else name_len++;
	}
	
	if ((name_len % 8) > 0) needed_frames = (name_len / 8) + 1;
	else needed_frames = name_len / 8;
	
	for (uint8_t i = 0; i < needed_frames; i++){
		for (uint8_t j = 0; j < 8; j++){
			if ((8 * i) + j < name_len) frame.data[j] = name[(8 * i) + j];
			else frame.data[j] = 0;
		}
		frame.hash++;
		sendCanFrame(&frame);
	}
	
	uint8_t frame_count = frame.hash - 0x300;
	
	frame.hash = mcan_hash;
	frame.dlc = 6;
	frame.data[0] = (uint8_t) (mcan_uid >> 24);
	frame.data[1] = (uint8_t) (mcan_uid >> 16);
	frame.data[2] = (uint8_t) (mcan_uid >> 8);
	frame.data[3] = (uint8_t) mcan_uid;
	frame.data[4] = 0;
	frame.data[5] = frame_count;
	
	sendCanFrame(&frame);
}

void sendPingFrame(uint16_t version, uint16_t type) {
	canFrame frame = {
		.cmd = CMD_PING,
		.resp = 1,
		.hash = mcan_hash,
		.dlc = 8,
		.data[0] = (uint8_t)(mcan_uid >> 24),
		.data[1] = (uint8_t)(mcan_uid >> 16),
		.data[2] = (uint8_t)(mcan_uid >> 8),
		.data[3] = (uint8_t)mcan_uid,
		.data[4] = (uint8_t)(version >> 8),
		.data[5] = (uint8_t)version,
		.data[6] = (uint8_t)(type >> 8),
		.data[7] = (uint8_t)type
	};
	
	sendCanFrame(&frame);
}

/*
 *
 */
void sendConfigInfoDropdown(uint8_t channel, uint8_t options, uint8_t default_option, char *string) {
	canFrame frame;
	
	frame.cmd = CMD_CONFIG;
	frame.resp = 1;
	frame.hash = 0x301;
	frame.dlc = 8;
	
	// Basic chanel info:
	frame.data[0] = channel;
	frame.data[1] = 1;
	frame.data[2] = options;
	frame.data[3] = default_option;
	frame.data[4] = 0;
	frame.data[5] = 0;
	frame.data[6] = 0;
	frame.data[7] = 0;
	
	sendCanFrame(&frame);
	
	// Chanel text:
	uint8_t string_len = 0;
	uint8_t needed_frames = 0;
	
	while (1) {
		if (string[string_len] == 0) break;
		else string_len++;
	}
	
	if ((string_len % 8) > 0) needed_frames = (string_len / 8) + 1;
	else needed_frames = string_len / 8;
	
	for (uint8_t i = 0; i < needed_frames; i++){
		for(uint8_t j = 0; j < 8; j++){
			if(((8*i)+j < string_len) && (string[(8*i)+j] != '_')) frame.data[j] = string[(8*i)+j];
			else frame.data[j] = 0;
		}
		frame.hash++;
		sendCanFrame(&frame);
	}
	
	// End Frame:
	uint8_t frame_count = frame.hash - 0x300;
	
	frame.hash = mcan_hash;
	frame.dlc = 6;
	frame.data[0] = (uint8_t) (mcan_uid >> 24);
	frame.data[1] = (uint8_t) (mcan_uid >> 16);
	frame.data[2] = (uint8_t) (mcan_uid >> 8);
	frame.data[3] = (uint8_t) mcan_uid;
	frame.data[4] = channel;
	frame.data[5] = frame_count;
	
	sendCanFrame(&frame);
}

void sendConfigInfoSlider(uint8_t channel, uint16_t min_value, uint16_t max_value, uint16_t default_value, char *string) {
	canFrame frame;
	
	frame.cmd = CMD_CONFIG;
	frame.resp = 1;
	frame.hash = 0x301;
	frame.dlc = 8;
	
	// Basic channel info:
	frame.data[0] = channel;
	frame.data[1] = 2;
	frame.data[2] = (uint8_t)(min_value >> 8);
	frame.data[3] = (uint8_t)min_value;
	frame.data[4] = (uint8_t)(max_value >> 8);
	frame.data[5] = (uint8_t)max_value;
	frame.data[6] = (uint8_t)(default_value >> 8);
	frame.data[7] = (uint8_t)default_value;
	
	sendCanFrame(&frame);
	
	// Chanel text:
	uint8_t string_len = 0;
	uint8_t needed_frames = 0;
	
	while (1) {
		if (string[string_len] == 0) break;
		else string_len++;
	}
	
	if (((string_len + 1) % 8) > 0) needed_frames = ((string_len + 1) / 8) + 1;
	else needed_frames = (string_len + 1) / 8;
	
	for(int i = 0; i < needed_frames; i++){
		for(int j = 0; j < 8; j++){
			if(((8*i)+j < string_len) && (string[(8*i)+j] != '_')) frame.data[j] = string[(8*i)+j];
			else frame.data[j] = 0;
		}
		
		frame.hash++;
		sendCanFrame(&frame);		
	}
	
	// End Frame:
	uint8_t frame_count = frame.hash - 0x300;

	frame.hash = mcan_hash;
	frame.dlc = 6;
	frame.data[0] = (uint8_t) (mcan_uid >> 24);
	frame.data[1] = (uint8_t) (mcan_uid >> 16);
	frame.data[2] = (uint8_t) (mcan_uid >> 8);
	frame.data[3] = (uint8_t) mcan_uid;
	frame.data[4] = channel;
	frame.data[5] = frame_count;

	sendCanFrame(&frame);	
}

void sendConfigConfirm(uint8_t channel, uint8_t valid) {
	canFrame frame;
	
	frame.hash = mcan_hash;
	frame.cmd = SYS_CMD;
	frame.resp = 1;
	frame.dlc = 7;
	
	frame.data[0] = (uint8_t) (mcan_uid >> 24);
	frame.data[1] = (uint8_t) (mcan_uid >> 16);
	frame.data[2] = (uint8_t) (mcan_uid >> 8);
	frame.data[3] = (uint8_t) mcan_uid;
	frame.data[4] = SYS_STAT;
	frame.data[5] = channel;
	frame.data[6] = valid;
	frame.data[7] = 0;
	
	sendCanFrame(&frame);
}

void sendS88Event(uint32_t portID, uint8_t old_value, uint8_t new_value) {
	canFrame frame;
	
	frame.hash = mcan_hash;
	frame.cmd = CMD_S88_EVENT;
	frame.resp = 1;
	frame.dlc = 8;
	frame.data[0] = (uint8_t) (portID >> 24);
	frame.data[1] = (uint8_t) (portID >> 16);
	frame.data[2] = (uint8_t) (portID >> 8);
	frame.data[3] = (uint8_t) portID;
	frame.data[4] = old_value;
	frame.data[5] = new_value;
	frame.data[6] = 0;
	frame.data[7] = 0;
	
	sendCanFrame(&frame);
}

void sendACCEvent(uint32_t accUID, uint8_t value, uint8_t power) {
	canFrame frame;
	
	frame.hash = mcan_hash;
	frame.cmd = CMD_SWITCH_ACC;
	frame.resp = 1;
	frame.dlc = 6;
	frame.data[0] = (uint8_t) (accUID >> 24);
	frame.data[1] = (uint8_t) (accUID >> 16);
	frame.data[2] = (uint8_t) (accUID >> 8);
	frame.data[3] = (uint8_t) accUID;
	frame.data[4] = value;
	frame.data[5] = power;
	frame.data[6] = 0;
	frame.data[7] = 0;
	
	sendCanFrame(&frame);
}

void sendStatus(uint8_t channel, uint16_t value) {
	canFrame frame;
	
	frame.hash = mcan_hash;
	frame.cmd = SYS_CMD;
	frame.resp = 1;
	frame.dlc = 8;
	frame.data[0] = (uint8_t) (mcan_uid >> 24);
	frame.data[1] = (uint8_t) (mcan_uid >> 16);
	frame.data[2] = (uint8_t) (mcan_uid >> 8);
	frame.data[3] = (uint8_t) mcan_uid;
	frame.data[4] = SYS_STAT;
	frame.data[5] = channel;
	frame.data[6] = (uint8_t) (value >> 8);
	frame.data[7] = (uint8_t) value;
	
	sendCanFrame(&frame);
	
}

ISR(INTCAN_vect) {
	CanBufferIn();
}