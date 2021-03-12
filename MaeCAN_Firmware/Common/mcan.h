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
 * [2021-03-12.1]
 */

#ifndef MCAN_H
#define MCAN_H

#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>
#include "uart.h"
#include "mcp2515_defs.h"

//  Adressbereich der Local-IDs:
#define MM_ACC 			0x3000	// Magnetartikel Motorola
#define DCC_ACC 		0x3800	// Magbetartikel NRMA_DCC
#define MM_TRACK 		0x0000	// Gleissignal Motorola
#define DCC_TRACK 		0xC000	// Gleissignal NRMA_DCC

//  CAN-Befehle (Märklin)
#define SYS_CMD			0x00 	// Systembefehle
 	#define SYS_STOP 	0x00 	// System - Stopp
 	#define SYS_GO		0x01	// System - Go
 	#define SYS_HALT	0x02	// System - Halt
 	#define SYS_STAT	0x0b	// System - Status (sendet geanderte Konfiguration oder übermittelt Messwerte)
#define CMD_SWITCH_ACC 	0x0b	// Magnetartikel Schalten
#define CMD_S88_EVENT	0x11	// Rückmelde-Event
#define CMD_PING 		0x18	// CAN-Teilnehmer anpingen
#define CMD_CONFIG		0x1d	// Konfiguration
#define CMD_BOOTLOADER	0x1B

#ifndef BAUD_RATE
#define BAUD_RATE 500000
#endif // BAUD RATE

#define CANBUFFERSIZE	16 // 2^n
#define CANBUFFERMASK	(CANBUFFERSIZE - 1)
#define BUFFER_FAIL		0
#define BUFFER_SUCCESS	1

// Pin definitions depending on chip:

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
// IO makros

// Configure IO
#define set_input(bit)             {bit ## _DDR &= ~(1 << bit);}
#define set_output(bit)            {bit ## _DDR |= (1 << bit);}

#define pullup_on(bit)             {bit ## _PORT |= (1 << bit);}
#define pullup_off(bit)            {bit ## _PORT &= ~(1 << bit);}

// Manipulate Outputs
#define set_high(bit)              {bit ## _PORT |= (1 << bit);}
#define set_low(bit)               {bit ## _PORT &= ~(1 << bit);}

// Read Inputs
#define is_high(bit)               (bit ## _PIN & (1 << bit))
#define is_low(bit)                (! (bit ## _PIN & (1 << bit)))

#endif // IO_MAKROS

// Märklin CAN frame structure
typedef struct {
	uint8_t cmd;
	uint8_t resp;
	uint16_t hash;
	uint8_t dlc;
	uint8_t data[8];
} canFrame;

#ifdef SLCAN
// Use New Line char after every frame sent over UART
extern uint8_t use_nl;
#endif // SLCAN

#ifdef __cplusplus
extern "C" {
#endif

// Init CAN
void mcp2515_init(void);

// Send can frame on bus only
void sendCanFrameNoSLCAN(canFrame *frame);

// Send can frame on bus and UART
void sendCanFrame(canFrame *frame);

// Returns 1 if frame from UART or CAN bus is available in the respective Buffer
uint8_t readCanFrame(canFrame *frame);

// Generate hash from UID
uint16_t generateHash(uint32_t uid);

// Send specific Märklin CAN frames
void sendDeviceInfo(uint32_t uid, uint16_t hash, uint32_t serial_nbr, uint8_t meassure_ch, uint8_t config_ch, char *art_nbr, char *name);
void sendConfigInfoDropdown(uint32_t uid, uint16_t hash, uint8_t channel, uint8_t options, uint8_t default_option, char *string);
void sendConfigInfoSlider(uint32_t uid, uint16_t hash, uint8_t channel, uint16_t min_value, uint16_t max_value, uint16_t default_value, char *string);
void sendPingFrame(uint32_t uid, uint16_t hash, uint16_t version, uint16_t type);
void sendConfigConfirm(uint32_t uid, uint16_t hash, uint8_t channel, uint8_t valid);
void sendS88Event(uint32_t portID, uint16_t hash, uint8_t old_value, uint8_t new_value);
void sendACCEvent(uint32_t accUID, uint16_t hash, uint8_t value, uint8_t power);
void sendStatus(uint32_t uid, uint16_t hash, uint8_t channel, uint16_t value);

// CAN interrupt service routine
ISR(INTCAN_vect);

#ifdef __cplusplus
}
#endif

#endif // MCAN_H