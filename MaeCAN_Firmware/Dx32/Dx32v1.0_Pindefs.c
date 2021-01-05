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
 * []
 */

#include "Dx32v1.0_Pindefs.h"

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

void initPins() {
	
	statusPin.port = &PORTD;
	statusPin.ddr = &DDRD;
	statusPin.pin = 3;
	
	intPin.port = &PORTD;
	intPin.ddr = &DDRD;
	intPin.pin = 2;
	
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
	
	
	for (uint8_t i = 0; i < 4; i++) {
		dip_switch[i].port = &PORTD;
		dip_switch[i].ddr = &DDRD;
		dip_switch[i].in = &PIND;
		dip_switch[i].pin = 7 - i;
		setHigh(dip_switch[i]);
		setInput(dip_switch[i]);
	}
	
	
	setOutput(statusPin);
	
}