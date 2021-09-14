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
 * MaeCAN MP5+D
 * V 0.1
 * [2021-09-14.1]
 */


#ifndef MP5_D_PINDEFS_H_
#define MP5_D_PINDEFS_H_

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

typedef struct  
{
	volatile uint8_t * port;
	volatile uint8_t * ddr;
	volatile uint8_t * in;
	uint8_t high;
	uint8_t pin;
} ioPin;

ioPin statusPin;

ioPin aux_1[16];
ioPin aux_2[16];
ioPin poz_1[16];
ioPin poz_2[16];

ioPin dip_switch[4];

void setOutput(ioPin _pin);

void setInput(ioPin _pin);

void setHigh(ioPin _pin);

void setLow(ioPin _pin);

uint8_t readPin(ioPin _pin);

void initPins();


#endif /* MP5_D_PINDEFS_H_ */