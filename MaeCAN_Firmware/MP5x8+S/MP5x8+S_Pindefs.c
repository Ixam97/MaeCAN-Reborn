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

#include "MP5x8+S_Pindefs.h"

const uint8_t ledLookupTable[] = {0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,7,7,7,8,8,8,9,9,9,10,10,11,11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20,21,21,22,23,23,24,24,25,26,26,27,28,28,29,30,30,31,32,32,33,34,35,35,36,37,38,38,39,40,41,42,42,43,44,45,46,47,47,48,49,50,51,52,53,54,55,56,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,73,74,75,76,77,78,79,80,81,82,84,85,86,87,88,89,91,92,93,94,95,97,98,99,100,102,103,104,105,107,108,109,111,112,113,115,116,117,119,120,121,123,124,126,127,128,130,131,133,134,136,137,139,140,142,143,145,146,148,149,151,152,154,155,157,158,160,162,163,165,166,168,170,171,173,175,176,178,180,181,183,185,186,188,190,192,193,195,197,199,200,202,204,206,207,209,211,213,215,217,218,220,222,224,226,228,230,232,233,235,237,239,241,243,245,247,249,251,253,255};

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

void switchOutput(uint8_t _pin, uint8_t _brightness, float _fade) {
	outputs_brightness_target[_pin] = _brightness;
	outputs_is_fading[_pin] = _fade;
}

void setupMsTimer() {
	
	TCCR0A |= (1 << WGM01); // Timer 0 clear time on compare match
	OCR0A = (F_CPU / (64 * 1000UL)) - 1; // Timer 0 compare value
	TIMSK0 |= (1 << OCIE0A); // Timer interrupt
	TCCR0B |= ((1 << CS01)|(1 << CS00)); // Set timer 0 prescaler to 64
	
	
}

void setupPwmTimer() {
	
	TCCR1A |= (1 << WGM11); // Timer 1 clear time on compare match
	//OCR1A = (F_CPU / (1 * 256000UL)) - 1;
	OCR1A = (uint16_t)(F_CPU / (100 * 256))-1;
	//OCR1A = 624;
	TIMSK1 |= (1 << OCIE1A);
	TCCR1B |= (1 << CS10);
}

void ledMillisCounter() {
	heartbeat_millis += 1;
}


void dutyCycleCounter() {
	
	if (duty_cycle_counter == 0) {
		duty_cycle_counter = 0xff;
		PORTH &= ~0b00001111;
		PORTE &= ~0b11110000;
		PORTF &= ~0b11110000;
		PORTK &= ~0b00001111;
		
	} else {
		--duty_cycle_counter;
	
	uint8_t const_counter = duty_cycle_counter;
	
	if (outputs_brightness[0] > const_counter) PORTH |= (1<<1);
	if (outputs_brightness[1] > const_counter) PORTH |= (1<<0);
	if (outputs_brightness[2] > const_counter) PORTH |= (1<<2);
	if (outputs_brightness[3] > const_counter) PORTH |= (1<<3);
	
	if (outputs_brightness[4] > const_counter) PORTE |= (1<<7);
	if (outputs_brightness[5] > const_counter) PORTE |= (1<<6);
	if (outputs_brightness[6] > const_counter) PORTE |= (1<<4);
	if (outputs_brightness[7] > const_counter) PORTE |= (1<<5);
	
	if (outputs_brightness[8] > const_counter) PORTF |= (1<<6);
	if (outputs_brightness[9] > const_counter) PORTF |= (1<<7);
	if (outputs_brightness[10] > const_counter) PORTF |= (1<<5);
	if (outputs_brightness[11] > const_counter) PORTF |= (1<<4);
	
	if (outputs_brightness[12] > const_counter) PORTK |= (1<<0);
	if (outputs_brightness[13] > const_counter) PORTK |= (1<<1);
	if (outputs_brightness[14] > const_counter) PORTK |= (1<<3);
	if (outputs_brightness[15] > const_counter) PORTK |= (1<<2);
	}

}

//
// Status LED blinking pattern.
// Heartbeat blinking during normal operation.
//
void heartbeat() {
	
	if (heartbeat_millis >= 1000) {
		setLow(statusPin);
		//s_led_rising = 0;
		heartbeat_millis = 0;
		} else if (heartbeat_millis >= 900) {
		setHigh(statusPin);
		//s_led_rising = 1;
		} else if (heartbeat_millis >= 800) {
		setLow(statusPin);
		//s_led_rising = 0;
		} else if (heartbeat_millis >= 700) {
		setHigh(statusPin);
		//s_led_rising = 1;
	}
	
	if (heartbeat_millis != last_heartbeat_millis) {
		last_heartbeat_millis = heartbeat_millis;
		
		for (uint8_t i = 0; i < IO_PORTS; i++) {
			if (outputs_is_fading[i] > 0) {
				if (outputs_brightness_set[i] > outputs_brightness_target[i]) {
					if (outputs_brightness_set[i] >= outputs_is_fading[i]) outputs_brightness_set[i] -= outputs_is_fading[i];
					else outputs_brightness_set[i] = 0;
				} else if (outputs_brightness_set[i] < outputs_brightness_target[i]) {
					if (outputs_brightness_set[i] <= (outputs_brightness_target[i] - outputs_is_fading[i])) outputs_brightness_set[i] += outputs_is_fading[i];
					else outputs_brightness_set[i] = outputs_brightness_target[i];
				}

			} else if (outputs_brightness_set[i] != outputs_brightness_target[i]) {
				outputs_brightness_set[i] = outputs_brightness_target[i];
			}
			
			outputs_brightness[i] = ledLookupTable[(uint8_t)outputs_brightness_set[i]];
		}		
	}	
	
	
}


void initPins() {
	
	statusPin.port = &PORTD;
	statusPin.ddr = &DDRD;
	statusPin.pin = 3;
	
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
	
	/* Nachfolgende Ports werden nicht f�r Weichenantriebe genutzt!!! */
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