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
 * MaeCAN MP5x8+S
 * V 0.1
 * [2022-03-26.1]
 */


#ifndef MP5_S_PINDEFS_H_
#define MP5_S_PINDEFS_H_

#define IO_PORTS 16

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

extern const uint8_t ledLookupTable[];

uint32_t heartbeat_millis;
uint32_t last_heartbeat_millis;

uint8_t duty_cycle_counter;

uint8_t outputs_state[IO_PORTS];
uint8_t outputs_brightness[IO_PORTS];
float outputs_brightness_set[IO_PORTS];
uint8_t outputs_brightness_target[IO_PORTS];
uint8_t outputs_rising[IO_PORTS];
float outputs_is_fading[IO_PORTS];

void setOutput(ioPin _pin);

void setInput(ioPin _pin);

void setHigh(ioPin _pin);

void setLow(ioPin _pin);

uint8_t readPin(ioPin _pin);

void switchOutput(uint8_t _pin, uint8_t _brightness, float _fade);

void initPins();

void setupMsTimer();

void setupPwmTimer();

void ledMillisCounter();

void dutyCycleCounter();

void heartbeat();

void resetLEDs(void);


#endif /* MP5_S_PINDEFS_H_ */