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

/*  SLCAN
 *
 *	If "SLCAN" is defined, the USB-Port creates a virtual COM-Port. This interface 
 *  transmits and receives CAN-Frames in the SLCAN format for extended Frames. The 
 *  device handles serial frames the same way as CAN-Frames and sends it's responses 
 *  on CAN and on Serial, while also converting the whole bus traffic.
 *
 * Tiiiiiiiildddddddddddddddd[CR]
 * 'T':		defines start of frame
 * i:		nibble of CAN ID
 * l:		CAN DLC
 * d:		nibble of CAN data
 * [CR]:	Frame termination
 *
 */

#include "mcp2515_defs.h"

#ifdef SLCAN
#include "slcan.h"
#ifndef BAUD_RATE
#define BAUD_RATE 500000
#endif // BAUD RATE
#endif // SLCAN


#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>

#ifndef MCP2515_BASIC_H
#define MCP2515_BASIC_H

#define CANBUFFERSIZE	16 // 2^n
#define CANBUFFERMASK	(CANBUFFERSIZE - 1)
#define BUFFER_FAIL		0
#define BUFFER_SUCCESS	1

// Pin-Deffinitionen:

#if defined(__AVR_ATmega328P__)

#define MISO		PB4
#define MISO_PORT	PORTB
#define MISO_DDR	DDRB
#define MISO_PIN	PINB

#define MOSI		PB3
#define MOSI_PORT	PORTB
#define MOSI_DDR	DDRB
#define MOSI_PIN	PINB

#define SCK			PB5
#define SCK_PORT	PORTB
#define SCK_DDR		DDRB
#define SCK_PIN		PINB

#define CS			PB2
#define CS_PORT		PORTB
#define CS_DDR		DDRB
#define CS_PIN		PINB

#define MCPINT		PD2
#define MCPINT_PORT	PORTD
#define MCPINT_DDR	DDRD
#define MCPINT_PIN	PIND

#define INTCAN_vect INT0_vect

#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)

#define MISO		PB3
#define MISO_PORT	PORTB
#define MISO_DDR	DDRB
#define MISO_PIN	PINB

#define MOSI		PB2
#define MOSI_PORT	PORTB
#define MOSI_DDR	DDRB
#define MOSI_PIN	PINB

#define SCK			PB1
#define SCK_PORT	PORTB
#define SCK_DDR		DDRB
#define SCK_PIN		PINB

#define CS			PB0
#define CS_PORT		PORTB
#define CS_DDR		DDRB
#define CS_PIN		PINB

#define MCPINT		PD2
#define MCPINT_PORT	PORTD
#define MCPINT_DDR	DDRD
#define MCPINT_PIN	PIND

#define INTCAN_vect INT2_vect

#endif

#ifndef IO_MAKROS
#define IO_MAKROS

/* Configure IO */
#define set_input(bit)             {bit ## _DDR &= ~(1 << bit);}
#define set_output(bit)            {bit ## _DDR |= (1 << bit);}

#define pullup_on(bit)             {bit ## _PORT |= (1 << bit);}
#define pullup_off(bit)            {bit ## _PORT &= ~(1 << bit);}

/* Manipulate Outputs */
#define set_high(bit)              {bit ## _PORT |= (1 << bit);}
#define set_low(bit)               {bit ## _PORT &= ~(1 << bit);}

/* Read Inputs */
#define is_high(bit)               (bit ## _PIN & (1 << bit))
#define is_low(bit)                (! (bit ## _PIN & (1 << bit)))

#endif

typedef struct {
	uint8_t cmd;
	uint8_t resp;
	uint16_t hash;
	uint8_t dlc;
	uint8_t data[8];
} canFrame;

void spi_init(void);

uint8_t spi_putc(uint8_t data);

void mcp1525_write_register(uint8_t adress, uint8_t data);

uint8_t mcp2515_read_register(uint8_t adress);

void mcp2515_bit_modify(uint8_t adress, uint8_t mask, uint8_t data);

uint8_t mcp2515_read_rx_status(void);

void mcp2515_init(void);

void sendCanFrameNoSLCAN(canFrame *frame);

void sendCanFrame(canFrame *frame);

uint8_t getCanFrame(canFrame *frame);

uint8_t CanBufferIn(void);

uint8_t CanBufferOut(canFrame *pFrame);

uint8_t readCanFrame(canFrame *frame);

ISR(INTCAN_vect);

#endif // MCP2515_BASIC_H