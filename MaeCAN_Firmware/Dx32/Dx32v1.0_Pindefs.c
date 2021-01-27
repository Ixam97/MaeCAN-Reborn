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
	/*
	t_led[0].port = &PORTA;
	t_led[0].ddr = &DDRA;
	t_led[0].in = &PINA;
	t_led[0].pin = 4;
	t_led[1].port = &PORTA;
	t_led[1].ddr = &DDRA;
	t_led[1].in = &PINA;
	t_led[1].pin = 5;
	t_led[2].port = &PORTA;
	t_led[2].ddr = &DDRA;
	t_led[2].in = &PINA;
	t_led[2].pin = 6;
	t_led[3].port = &PORTA;
	t_led[3].ddr = &DDRA;
	t_led[3].in = &PINA;
	t_led[3].pin = 7;
	
	t_led[4].port = &PORTJ;
	t_led[4].ddr = &DDRJ;
	t_led[4].in = &PINJ;
	t_led[4].pin = 3;
	t_led[5].port = &PORTJ;
	t_led[5].ddr = &DDRJ;
	t_led[5].in = &PINJ;
	t_led[5].pin = 2;
	t_led[6].port = &PORTJ;
	t_led[6].ddr = &DDRJ;
	t_led[6].in = &PINJ;
	t_led[6].pin = 1;
	t_led[7].port = &PORTJ;
	t_led[7].ddr = &DDRJ;
	t_led[7].in = &PINJ;
	t_led[7].pin = 0;
	
	t_led[8].port = &PORTC;
	t_led[8].ddr = &DDRC;
	t_led[8].in = &PINC;
	t_led[8].pin = 3;
	t_led[9].port = &PORTC;
	t_led[9].ddr = &DDRC;
	t_led[9].in = &PINC;
	t_led[9].pin = 2;
	t_led[10].port = &PORTC;
	t_led[10].ddr = &DDRC;
	t_led[10].in = &PINC;
	t_led[10].pin = 1;
	t_led[11].port = &PORTC;
	t_led[11].ddr = &DDRC;
	t_led[11].in = &PINC;
	t_led[11].pin = 0;
	
	t_led[12].port = &PORTL;
	t_led[12].ddr = &DDRL;
	t_led[12].in = &PINL;
	t_led[12].pin = 7;
	t_led[13].port = &PORTL;
	t_led[13].ddr = &DDRL;
	t_led[13].in = &PINL;
	t_led[13].pin = 6;
	t_led[14].port = &PORTL;
	t_led[14].ddr = &DDRL;
	t_led[14].in = &PINL;
	t_led[14].pin = 5;
	t_led[15].port = &PORTL;
	t_led[15].ddr = &DDRL;
	t_led[15].in = &PINL;
	t_led[15].pin = 4;
	
	t_led[16].port = &PORTH;
	t_led[16].ddr = &DDRH;
	t_led[16].in = &PINH;
	t_led[16].pin = 3;
	t_led[17].port = &PORTH;
	t_led[17].ddr = &DDRH;
	t_led[17].in = &PINH;
	t_led[17].pin = 2;
	t_led[18].port = &PORTH;
	t_led[18].ddr = &DDRH;
	t_led[18].in = &PINH;
	t_led[18].pin = 1;
	t_led[19].port = &PORTH;
	t_led[19].ddr = &DDRH;
	t_led[19].in = &PINH;
	t_led[19].pin = 0;
	
	t_led[20].port = &PORTE;
	t_led[20].ddr = &DDRE;
	t_led[20].in = &PINE;
	t_led[20].pin = 3;
	t_led[21].port = &PORTE;
	t_led[21].ddr = &DDRE;
	t_led[21].in = &PINE;
	t_led[21].pin = 2;
	t_led[22].port = &PORTB;
	t_led[22].ddr = &DDRB;
	t_led[22].in = &PINB;
	t_led[22].pin = 6;
	t_led[23].port = &PORTB;
	t_led[23].ddr = &DDRB;
	t_led[23].in = &PINB;
	t_led[23].pin = 7;
	
	t_led[24].port = &PORTF;
	t_led[24].ddr = &DDRF;
	t_led[24].in = &PINF;
	t_led[24].pin = 0;
	t_led[25].port = &PORTF;
	t_led[25].ddr = &DDRF;
	t_led[25].in = &PINF;
	t_led[25].pin = 1;
	t_led[26].port = &PORTF;
	t_led[26].ddr = &DDRF;
	t_led[26].in = &PINF;
	t_led[26].pin = 2;
	t_led[27].port = &PORTF;
	t_led[27].ddr = &DDRF;
	t_led[27].in = &PINF;
	t_led[27].pin = 3;
	
	t_led[28].port = &PORTK;
	t_led[28].ddr = &DDRK;
	t_led[28].in = &PINK;
	t_led[28].pin = 0;
	t_led[29].port = &PORTK;
	t_led[29].ddr = &DDRK;
	t_led[29].in = &PINK;
	t_led[29].pin = 1;
	t_led[30].port = &PORTK;
	t_led[30].ddr = &DDRK;
	t_led[30].in = &PINK;
	t_led[30].pin = 2;
	t_led[31].port = &PORTK;
	t_led[31].ddr = &DDRK;
	t_led[31].in = &PINK;
	t_led[31].pin = 3;
	
	for (uint8_t i = 0; i < 32; i++) {
		setHigh(t_led[i]);
		setOutput(t_led[i]);
	}
	*/
	
	for (uint8_t i = 0; i < 4; i++) {
		dip_switch[i].port = &PORTD;
		dip_switch[i].ddr = &DDRD;
		dip_switch[i].in = &PIND;
		dip_switch[i].pin = 7 - i;
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