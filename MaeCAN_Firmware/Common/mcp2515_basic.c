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
 * [2021-01-27.1]
 */

#include "mcp2515_basic.h"

static canFrame can_buffer[CANBUFFERSIZE];
static uint8_t can_buffer_read;
static uint8_t can_buffer_write;

/************************************************************************/
/* SPI                                                                  */
/************************************************************************/

void spi_init(void) {
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


uint8_t spi_putc(uint8_t data) {
	//Send a byte:
	SPDR = data;
	
	//Wait until byte is sent:
	while (!(SPSR & (1 << SPIF))) {};
	
	return SPDR;
}


/************************************************************************/
/* MCP2515                                                              */
/************************************************************************/

/* Basic */

void mcp1525_write_register(uint8_t adress, uint8_t data) {
	set_low(CS);
	
	spi_putc(SPI_WRITE);
	spi_putc(adress);
	spi_putc(data);
	
	set_high(CS);
}

uint8_t mcp2515_read_register(uint8_t adress) {
	uint8_t data;
	
	set_low(CS);
	
	spi_putc(SPI_READ);
	spi_putc(adress);
	
	data = spi_putc(0xff);
	
	set_high(CS);
	
	return data;
}

void mcp2515_bit_modify(uint8_t adress, uint8_t mask, uint8_t data) {
	set_low(CS);
	
	spi_putc(SPI_BIT_MODIFY);
	spi_putc(adress);
	spi_putc(mask);
	spi_putc(data);
	
	set_high(CS);
}

uint8_t mcp2515_read_rx_status(void) {
	uint8_t data;
	
	set_low(CS);
	
	spi_putc(SPI_RX_STATUS);
	data = spi_putc(0xff);
	spi_putc(0xff);
	
	set_high(CS);
	
	return data;
}

void mcp2515_init(void) {
	
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

uint8_t getCanFrame(canFrame *frame) {
	
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

void mcp2515_reset_int(uint8_t status) {
	
}

uint8_t CanBufferIn(void) {
	uint8_t next = (can_buffer_write + 1) & CANBUFFERMASK;
	
	if (can_buffer_read == next)
	return BUFFER_FAIL; // Full
	
	getCanFrame(&can_buffer[can_buffer_write & CANBUFFERMASK]);
	
	can_buffer_write = next;
	
	return BUFFER_SUCCESS;
}

uint8_t CanBufferOut(canFrame *pFrame) {
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

ISR(INTCAN_vect) {
	CanBufferIn();
}