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
 * [2020-08-27.1]
 */

//#define F_CPU 16000000UL

#include "mcp2515_defs.h"
#include <avr/io.h>
#include <util/delay.h>

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

#elif defined(__AVR_ATmega2560__)

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

void sendCanFrame(canFrame *frame);

uint8_t getCanFrame(canFrame *frame);
