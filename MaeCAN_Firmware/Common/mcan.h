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

/** 
 *  @file
 *  @defgroup maecan MCAN Library
 *  @code #include "mcan.h" @endcode
 * 
 *  @brief Communicate with CAN-bus using the Märklin CAN Protocol. 
 *
 *  @author Maximilian Goldschmidt https://github.com/Ixam97
 */

#ifndef MCAN_H
#define MCAN_H

#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>
#include "uart.h"
#include "mcp2515_defs.h"

/**@{*/

/*
* Address areas of local IDs
*/
#define MM_ACC 			0x3000	// Magnetartikel Motorola
#define DCC_ACC 		0x3800	// Magbetartikel NRMA_DCC
#define MM_TRACK 		0x0000	// Gleissignal Motorola
#define DCC_TRACK 		0xC000	// Gleissignal NRMA_DCC

/*
* Märklin CAN commands
*/
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

/**
 * @brief Märklin CAN frame structure
 */
typedef struct {
	uint8_t cmd;		/**< Märklin CAN command */
	uint8_t resp;		/**< Response-bit, 1 if response, else 0 */
	uint16_t hash;		/**< Hash, calculated by using generateHash() */
	uint8_t dlc;		/**< Number of data bytes (0 - 8) */
	uint8_t data[8];	/**< Array of 8 data bytes */
} canFrame;

uint32_t mcan_uid;
uint16_t mcan_hash;

#ifdef SLCAN
// Use New Line char after every frame sent over UART
extern uint8_t use_nl;
#endif // SLCAN

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the CAN-Bus.
 * 
 * @param _uid 32-bit device UID
 */
void mcan_init(uint32_t _uid);

/**
 * @brief Send can frame on bus only.
 * 
 * @param frame Pointer to frame to be sent
 */
void sendCanFrameNoSLCAN(canFrame *frame);

/**
 * @brief Send can frame on bus and UART.
 * 
 * @param frame Pointer to frame to be sent
 */
void sendCanFrame(canFrame *frame);

/**
 * @brief Read CAN-Frame from CAN-Bus or SLCAN.
 * 
 * @param frame Pointer to frame to store incomig frame
 * @returns 1 if frame is available, else 0
 */
uint8_t readCanFrame(canFrame *frame);

/**
 * @brief Generates hash from device UID.
 * 
 * @param uid 32-bit device UID
 * @return uint16_t hash
 */
uint16_t generateHash(uint32_t _uid);

/**
 * @brief Compares a 8-byte data array with a 32-bit UID
 * 
 * @param _data 8-byte data array
 * @param _uid 32 bit UID
 * @return 1 if UIDs match, else 0
 */
uint8_t compareUID(uint8_t _data[8], uint32_t _uid);

/*
** Typical Märklin CAN commands
*/

/**
 * @brief Sends device info. Device info is requested by GUIs to be displayed after receiving a ping of the device.
 * 
 * @param serial_nbr Serial number of the device
 * @param measure_ch Number of measurement channels provided by the device
 * @param config_ch Number of configuration channels to configure the device
 * @param art_nbr String of the item number of the device
 * @param name String of the describing name of the device
 */
void sendDeviceInfo(uint32_t serial_nbr, uint8_t measure_ch, uint8_t config_ch, char *art_nbr, char *name);

/**
 * @brief Send the relevant information for a dropdomwn config channel.
 * 
 * @param channel Index of the config channel
 * @param options Number of available options
 * @param default_option Index of the current or default option
 * @param string String for channel description, syntax: "[Description]_[Option 1]_[Option 2]_ ... _[Option n]"
 */
void sendConfigInfoDropdown(uint8_t channel, uint8_t options, uint8_t default_option, char *string);

/**
 * @brief Sends the relevant information for a slider config channel.
 * 
 * @param channel Index of the config channel
 * @param min_value Lowest possible value
 * @param max_value Highest possible value
 * @param default_value Current or default value
 * @param string String for channel description, syntax: "[Description]_[MinValue]_[MaxValue]_[Uinit]"
 * 
 * The requesting UI scales the integer Values acording to the values contained in the describing string.
 */
void sendConfigInfoSlider(uint8_t channel, uint16_t min_value, uint16_t max_value, uint16_t default_value, char *string);

/**
 * @brief Sends a ping response.
 * 
 * @param version Version of the current software; low byte behind the dot, high byte in front of the dot
 * @param type Device type
 */
void sendPingFrame(uint16_t version, uint16_t type);

/**
 * @brief Confirms a config channel change request.
 * 
 * @param channel Index of the config channel
 * @param valid 1 if channel index and value are valid, else 0
 */
void sendConfigConfirm(uint8_t channel, uint8_t valid);

/**
 * @brief Sends a S88-event.
 * 
 * @param portID ID of the specific port to be reported
 * @param old_value Value of the port before the event (1 or 0)
 * @param new_value  Value of the port after the event (1 or 0)
 */
void sendS88Event(uint32_t portID, uint8_t old_value, uint8_t new_value);

/**
 * @brief Sends information about changed outputs
 * 
 * @param accUID UID of the output (protocol-range + address - 1)
 * @param value Port of the output (1 or 0, represents green and red)
 * @param power Current power state of the port
 */
void sendACCEvent(uint32_t accUID, uint8_t value, uint8_t power);

/**
 * @brief Transmitt the current reading of a meassurement channel
 * 
 * @param channel Meassurement channel index
 * @param value current rerading value
 */
void sendStatus(uint8_t channel, uint16_t value);

/**
 * @brief Construct a new ISR object for MCP2515.
 */
ISR(INTCAN_vect);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif // MCAN_H