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
 * MaeCAN MP5x16
 * V 1.0
 * [2020-08-27.1]
 */

#include "MP5x16v1.0_Pindefs.h"

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
	
	/* OUTPUT A*/
	aux_1[0].port = &PORTA;
	aux_2[0].port = &PORTA;
	poz_1[0].port = &PORTA;
	poz_2[0].port = &PORTA;
	aux_1[0].ddr = &DDRA;
	aux_2[0].ddr = &DDRA;
	poz_1[0].ddr = &DDRA;
	poz_2[0].ddr = &DDRA;
	aux_1[0].in = &PINA;
	aux_2[0].in = &PINA;
	poz_1[0].in = &PINA;
	poz_2[0].in = &PINA;
	aux_1[0].pin = 0;
	aux_2[0].pin = 1;
	poz_1[0].pin = 6;
	poz_2[0].pin = 7;
	
	/* OUTPUT B*/
	aux_1[1].port = &PORTA;
	aux_2[1].port = &PORTA;
	poz_1[1].port = &PORTA;
	poz_2[1].port = &PORTA;
	aux_1[1].ddr = &DDRA;
	aux_2[1].ddr = &DDRA;
	poz_1[1].ddr = &DDRA;
	poz_2[1].ddr = &DDRA;
	aux_1[1].in = &PINA;
	aux_2[1].in = &PINA;
	poz_1[1].in = &PINA;
	poz_2[1].in = &PINA;
	aux_1[1].pin = 2;
	aux_2[1].pin = 3;
	poz_1[1].pin = 5;
	poz_2[1].pin = 4;
	
	/* OUTPUT C*/
	aux_1[2].port = &PORTJ;
	aux_2[2].port = &PORTJ;
	poz_1[2].port = &PORTJ;
	poz_2[2].port = &PORTJ;
	aux_1[2].ddr = &DDRJ;
	aux_2[2].ddr = &DDRJ;
	poz_1[2].ddr = &DDRJ;
	poz_2[2].ddr = &DDRJ;
	aux_1[2].in = &PINJ;
	aux_2[2].in = &PINJ;
	poz_1[2].in = &PINJ;
	poz_2[2].in = &PINJ;
	aux_1[2].pin = 3;
	aux_2[2].pin = 2;
	poz_1[2].pin = 7;
	poz_2[2].pin = 6;
	
	/* OUTPUT D*/
	aux_1[3].port = &PORTJ;
	aux_2[3].port = &PORTJ;
	poz_1[3].port = &PORTJ;
	poz_2[3].port = &PORTJ;
	aux_1[3].ddr = &DDRJ;
	aux_2[3].ddr = &DDRJ;
	poz_1[3].ddr = &DDRJ;
	poz_2[3].ddr = &DDRJ;
	aux_1[3].in = &PINJ;
	aux_2[3].in = &PINJ;
	poz_1[3].in = &PINJ;
	poz_2[3].in = &PINJ;
	aux_1[3].pin = 1;
	aux_2[3].pin = 0;
	poz_1[3].pin = 4;
	poz_2[3].pin = 5;
	
	/* OUTPUT E*/
	aux_1[4].port = &PORTC;
	aux_2[4].port = &PORTC;
	poz_1[4].port = &PORTC;
	poz_2[4].port = &PORTC;
	aux_1[4].ddr = &DDRC;
	aux_2[4].ddr = &DDRC;
	poz_1[4].ddr = &DDRC;
	poz_2[4].ddr = &DDRC;
	aux_1[4].in = &PINC;
	aux_2[4].in = &PINC;
	poz_1[4].in = &PINC;
	poz_2[4].in = &PINC;
	aux_1[4].pin = 7;
	aux_2[4].pin = 6;
	poz_1[4].pin = 1;
	poz_2[4].pin = 0;
	
	/* OUTPUT F*/
	aux_1[5].port = &PORTC;
	aux_2[5].port = &PORTC;
	poz_1[5].port = &PORTC;
	poz_2[5].port = &PORTC;
	aux_1[5].ddr = &DDRC;
	aux_2[5].ddr = &DDRC;
	poz_1[5].ddr = &DDRC;
	poz_2[5].ddr = &DDRC;
	aux_1[5].in = &PINC;
	aux_2[5].in = &PINC;
	poz_1[5].in = &PINC;
	poz_2[5].in = &PINC;
	aux_1[5].pin = 5;
	aux_2[5].pin = 4;
	poz_1[5].pin = 2;
	poz_2[5].pin = 3;
	
	/* OUTPUT G*/
	aux_1[6].port = &PORTL;
	aux_2[6].port = &PORTL;
	poz_1[6].port = &PORTL;
	poz_2[6].port = &PORTL;
	aux_1[6].ddr = &DDRL;
	aux_2[6].ddr = &DDRL;
	poz_1[6].ddr = &DDRL;
	poz_2[6].ddr = &DDRL;
	aux_1[6].in = &PINL;
	aux_2[6].in = &PINL;
	poz_1[6].in = &PINL;
	poz_2[6].in = &PINL;
	aux_1[6].pin = 3;
	aux_2[6].pin = 2;
	poz_1[6].pin = 7;
	poz_2[6].pin = 6;
	
	/* OUTPUT H*/
	aux_1[7].port = &PORTL;
	aux_2[7].port = &PORTL;
	poz_1[7].port = &PORTL;
	poz_2[7].port = &PORTL;
	aux_1[7].ddr = &DDRL;
	aux_2[7].ddr = &DDRL;
	poz_1[7].ddr = &DDRL;
	poz_2[7].ddr = &DDRL;
	aux_1[7].in = &PINL;
	aux_2[7].in = &PINL;
	poz_1[7].in = &PINL;
	poz_2[7].in = &PINL;
	aux_1[7].pin = 1;
	aux_2[7].pin = 0;
	poz_1[7].pin = 4;
	poz_2[7].pin = 5;
	
	/* OUTPUT I*/
	aux_1[8].port = &PORTH;
	aux_2[8].port = &PORTH;
	poz_1[8].port = &PORTH;
	poz_2[8].port = &PORTH;
	aux_1[8].ddr = &DDRH;
	aux_2[8].ddr = &DDRH;
	poz_1[8].ddr = &DDRH;
	poz_2[8].ddr = &DDRH;
	aux_1[8].in = &PINH;
	aux_2[8].in = &PINH;
	poz_1[8].in = &PINH;
	poz_2[8].in = &PINH;
	aux_1[8].pin = 7;
	aux_2[8].pin = 6;
	poz_1[8].pin = 1;
	poz_2[8].pin = 0;
	
	/* OUTPUT J*/
	aux_1[9].port = &PORTH;
	aux_2[9].port = &PORTH;
	poz_1[9].port = &PORTH;
	poz_2[9].port = &PORTH;
	aux_1[9].ddr = &DDRH;
	aux_2[9].ddr = &DDRH;
	poz_1[9].ddr = &DDRH;
	poz_2[9].ddr = &DDRH;
	aux_1[9].in = &PINH;
	aux_2[9].in = &PINH;
	poz_1[9].in = &PINH;
	poz_2[9].in = &PINH;
	aux_1[9].pin = 5;
	aux_2[9].pin = 4;
	poz_1[9].pin = 2;
	poz_2[9].pin = 3;
	
	/* OUTPUT K*/
	aux_1[10].port = &PORTE;
	aux_2[10].port = &PORTE;
	poz_1[10].port = &PORTE;
	poz_2[10].port = &PORTE;
	aux_1[10].ddr = &DDRE;
	aux_2[10].ddr = &DDRE;
	poz_1[10].ddr = &DDRE;
	poz_2[10].ddr = &DDRE;
	aux_1[10].in = &PINE;
	aux_2[10].in = &PINE;
	poz_1[10].in = &PINE;
	poz_2[10].in = &PINE;
	aux_1[10].pin = 3;
	aux_2[10].pin = 2;
	poz_1[10].pin = 7;
	poz_2[10].pin = 6;
	
	/* OUTPUT L*/
	aux_1[11].port = &PORTE;
	aux_2[11].port = &PORTE;
	poz_1[11].port = &PORTE;
	poz_2[11].port = &PORTE;
	aux_1[11].ddr = &DDRE;
	aux_2[11].ddr = &DDRE;
	poz_1[11].ddr = &DDRE;
	poz_2[11].ddr = &DDRE;
	aux_1[11].in = &PINE;
	aux_2[11].in = &PINE;
	poz_1[11].in = &PINE;
	poz_2[11].in = &PINE;
	aux_1[11].pin = 1;
	aux_2[11].pin = 0;
	poz_1[11].pin = 4;
	poz_2[11].pin = 5;
	
	/* OUTPUT M*/
	aux_1[12].port = &PORTF;
	aux_2[12].port = &PORTF;
	poz_1[12].port = &PORTF;
	poz_2[12].port = &PORTF;
	aux_1[12].ddr = &DDRF;
	aux_2[12].ddr = &DDRF;
	poz_1[12].ddr = &DDRF;
	poz_2[12].ddr = &DDRF;
	aux_1[12].in = &PINF;
	aux_2[12].in = &PINF;
	poz_1[12].in = &PINF;
	poz_2[12].in = &PINF;
	aux_1[12].pin = 0;
	aux_2[12].pin = 1;
	poz_1[12].pin = 6;
	poz_2[12].pin = 7;
	
	/* OUTPUT N*/
	aux_1[13].port = &PORTF;
	aux_2[13].port = &PORTF;
	poz_1[13].port = &PORTF;
	poz_2[13].port = &PORTF;
	aux_1[13].ddr = &DDRF;
	aux_2[13].ddr = &DDRF;
	poz_1[13].ddr = &DDRF;
	poz_2[13].ddr = &DDRF;
	aux_1[13].in = &PINF;
	aux_2[13].in = &PINF;
	poz_1[13].in = &PINF;
	poz_2[13].in = &PINF;
	aux_1[13].pin = 2;
	aux_2[13].pin = 3;
	poz_1[13].pin = 5;
	poz_2[13].pin = 4;
	
	/* OUTPUT O*/
	aux_1[14].port = &PORTK;
	aux_2[14].port = &PORTK;
	poz_1[14].port = &PORTK;
	poz_2[14].port = &PORTK;
	aux_1[14].ddr = &DDRK;
	aux_2[14].ddr = &DDRK;
	poz_1[14].ddr = &DDRK;
	poz_2[14].ddr = &DDRK;
	aux_1[14].in = &PINK;
	aux_2[14].in = &PINK;
	poz_1[14].in = &PINK;
	poz_2[14].in = &PINK;
	aux_1[14].pin = 4;
	aux_2[14].pin = 5;
	poz_1[14].pin = 0;
	poz_2[14].pin = 1;
	
	/* OUTPUT P*/
	aux_1[15].port = &PORTK;
	aux_2[15].port = &PORTK;
	poz_1[15].port = &PORTK;
	poz_2[15].port = &PORTK;
	aux_1[15].ddr = &DDRK;
	aux_2[15].ddr = &DDRK;
	poz_1[15].ddr = &DDRK;
	poz_2[15].ddr = &DDRK;
	aux_1[15].in = &PINK;
	aux_2[15].in = &PINK;
	poz_1[15].in = &PINK;
	poz_2[15].in = &PINK;
	aux_1[15].pin = 6;
	aux_2[15].pin = 7;
	poz_1[15].pin = 3;
	poz_2[15].pin = 2;
	
	for (uint8_t i = 0; i < 4; i++) {
		dip_switch[i].port = &PORTD;
		dip_switch[i].ddr = &DDRD;
		dip_switch[i].in = &PIND;
		dip_switch[i].pin = 7 - i;
		setHigh(dip_switch[i]);
		setInput(dip_switch[i]);
	}
	
	
	setOutput(statusPin);
	
	for (uint8_t i = 0; i < 16; i++)
	{
		setInput(aux_1[i]);
		setHigh(aux_1[i]);
		setInput(aux_2[i]);
		setHigh(aux_2[i]);
		setOutput(poz_1[i]);
		setOutput(poz_2[i]);
	}
	
}