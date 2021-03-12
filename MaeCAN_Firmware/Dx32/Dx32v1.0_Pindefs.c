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
 * V 1.0
 * [2021-03-12.1]
 */

#include "Dx32v1.0_Pindefs.h"

const uint8_t reverse_lookup[] PROGMEM = {0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};

void setOutput(ioPin _pin) {
	*_pin.ddr |= (1 << _pin.pin);
}

void setInput(ioPin _pin) {
	*_pin.ddr &= ~(1 << _pin.pin);
}

void setHigh(ioPin _pin) {
	*_pin.port |= (1 << _pin.pin);
}

void setLow(ioPin _pin) {
	*_pin.port &= ~(1 << _pin.pin);
}

uint8_t readPin(ioPin _pin) {
	if(*_pin.in & (1 << _pin.pin)) {
		return 1;
	} else {
		return 0;
	}
}

uint8_t reverseNibble(uint8_t _nibble) {
	return (uint8_t)pgm_read_byte(&reverse_lookup[_nibble & 0b00001111]);
}

uint8_t reverseByte(uint8_t _byte) {
	return ((uint8_t)pgm_read_byte(&reverse_lookup[_byte & 0b00001111]) << 4) | (uint8_t)pgm_read_byte(&reverse_lookup[_byte >> 4]);
}

void initPins() {
	
	statusPin.port = &PORTD;
	statusPin.ddr = &DDRD;
	statusPin.pin = 3;
	
	for (uint8_t i = 0; i < 4; i++) {
		dip_switch[i].port = &PORTD;
		dip_switch[i].ddr = &DDRD;
		dip_switch[i].in = &PIND;
		dip_switch[i].pin = 4 + i;
		setHigh(dip_switch[i]);
		setInput(dip_switch[i]);
	}
	
	
	setOutput(statusPin);
	
	PORTA = 0b11111111; // T1 - T8
	PORTJ = 0b11111111;
	
	DDRA = 0b11110000;
	DDRJ = 0b00001111;
	
	PORTC = 0b11111111; // T9 - T16
	PORTL = 0b11111111;
	
	DDRC = 0b00001111;
	DDRL = 0b11110000;
	
	PORTH = 0b11111111; // T17 - T24
	PORTE |= (0b11111100);
	PORTB |= (0b11000000);
	
	DDRH = 0b00001111;
	DDRE |= 0b00001100;
	DDRB |= 0b11000000;
	DDRE &= ~0b11110000;
	
	PORTF = 0b11111111; // T25 - T32
	PORTK = 0b11111111;
	
	DDRF = 0b00001111;
	DDRK = 0b00001111;
}

void readTracks(void) {
	
	TRKA = ((PINA & 0b00001111) + (PINJ & 0b11110000));
	TRKB = ((PINC >> 4) + (reverseNibble(PINL) << 4));
	TRKC = (reverseNibble(PINH >> 4) + (PINE & 0b11110000));
	TRKD = (reverseNibble(PINF >> 4) + (PINK & 0b11110000));
}

void setLEDs(void) {
	
	// T1 - T8
	PORTA &= ~(0b11110000 & (LEDA << 4));
	PORTJ &= ~(0b00001111 & reverseNibble(LEDA >> 4));
	
	// T9 - T16
	PORTC &= ~(0b00001111 & reverseNibble(LEDB));
	PORTL &= ~(0b11110000 & (reverseNibble(LEDB >> 4) << 4));
	
	PORTH &= ~(0b00001111 & reverseNibble(LEDC));
	PORTE &= ~(0b00001100 & reverseNibble(LEDC >> 4));
	PORTB &= ~(0b11000000 & LEDC);
	
	PORTF &= ~(0b00001111 & LEDD);
	PORTK &= ~(0b00001111 & (LEDD >> 4));
	
}

void resetLEDs(void) {
	PORTA |= 0b11110000;
	PORTJ |= 0b00001111;
	PORTC |= 0b00001111;
	PORTL |= 0b11110000;
	PORTH |= 0b00001111;
	PORTE |= 0b00001100;
	PORTB |= 0b11000000;
	PORTF |= 0b00001111;
	PORTK |= 0b00001111;
}