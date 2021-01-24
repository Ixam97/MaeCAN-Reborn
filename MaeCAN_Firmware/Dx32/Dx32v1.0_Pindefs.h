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
 * MaeCAN Dx32
 * V 0.2
 * [2021-01-24.1]
 */


#ifndef DX32V1_0_PINDEFS_H_
#define DX32V1_0_PINDEFS_H_

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

static uint8_t reverse_lookup[] = {0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};
	
uint8_t TRKA, TRKB, TRKC, TRKD;
uint8_t LEDA, LEDB, LEDC, LEDD;

typedef struct  
{
	volatile uint8_t * port;
	volatile uint8_t * ddr;
	volatile uint8_t * in;
	uint8_t high;
	uint8_t pin;
} ioPin;

ioPin statusPin;

ioPin intPin;

ioPin dip_switch[4];

//ioPin t_led[32];

void setOutput(ioPin _pin);

void setInput(ioPin _pin);

void setHigh(ioPin _pin);

void setLow(ioPin _pin);

uint8_t readPin(ioPin _pin);

uint8_t reverseNibble(uint8_t _nibble);

uint8_t reverseByte(uint8_t _byte);

void initPins();

void readTracks(void);

void setLEDs(void);

void resetLEDs(void);

#endif /* DX32V1_0_PINDEFS_H_ */