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


#define NAME "M\u00E4CAN MP5+D"
#define BASE_UID 0x4D430000
#define ITEM "MP5+D"
#define VERSION 0x0001
#define TYPE 0x0052

#define MP5_COUNT 8
//#define ENABLE_FEEDBACK 0

#define EEPROM_STATE_OFFSET 1024

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <string.h>

#include "../Common/mcan.h"
#include "MP5+D_Pindefs.h"

uint16_t serial_nbr;
uint16_t id;

uint8_t reset_request;					// 1 if software reset is requested, else 0

uint32_t heartbeat_millis;				// Time counter for status LED
uint32_t last_heartbeat_millis;
uint8_t debounce_millis;				// Time counter for input debouncing

uint8_t aux_1_laststate[16];			// Last read value of aux 1 input
uint8_t aux_2_laststate[16];			// Last read value of aux 2 input
uint8_t aux_1_lastreport[16];			// Last reported value of aux 1 input
uint8_t aux_2_lastreport[16];			// Last reported value of aux 2 input

uint8_t req_position[16];				// Requested position of output
uint8_t is_position[16];				// Current position of output

uint8_t active;							// Output currently moving
uint16_t active_millis;					// Movement timer

uint8_t connected[16];					// 1 if corresponding output is connected, else 0

uint8_t duty_cycle_counter;
uint8_t last_duty_cycle_counter;

uint8_t s_led_state;
uint8_t s_led_brightness = 0;
uint8_t s_led_brightness_set = 0;
uint8_t s_led_rising = 1;

const uint8_t ledLookupTable[] PROGMEM = {0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,7,7,7,8,8,8,9,9,9,10,10,11,11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20,21,21,22,23,23,24,24,25,26,26,27,28,28,29,30,30,31,32,32,33,34,35,35,36,37,38,38,39,40,41,42,42,43,44,45,46,47,47,48,49,50,51,52,53,54,55,56,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,73,74,75,76,77,78,79,80,81,82,84,85,86,87,88,89,91,92,93,94,95,97,98,99,100,102,103,104,105,107,108,109,111,112,113,115,116,117,119,120,121,123,124,126,127,128,130,131,133,134,136,137,139,140,142,143,145,146,148,149,151,152,154,155,157,158,160,162,163,165,166,168,170,171,173,175,176,178,180,181,183,185,186,188,190,192,193,195,197,199,200,202,204,206,207,209,211,213,215,217,218,220,222,224,226,228,230,232,233,235,237,239,241,243,245,247,249,251,253,255};

uint16_t DCC(uint16_t adr) {
	if (adr > 0 && adr <= 2048) return 0x3800 + adr - 1;
	else return 0x3800;
}

uint16_t MM(uint16_t adr) {
	if (adr > 0 && adr <= 1024) return 0x3000 + adr - 1;
	else return 0x3000;
}

void configChannel(uint8_t channel) {
	/* generate config channels for all outputs */
	#define ADDRESS "Adresse_0_2048"
	#define FB_GREEN "R\u00FCckmelder_0_2048"
	
	char letters[][5] = {"[A] ", "[B] ", "[C] ", "[D] ", "[E] ", "[F] ", "[G] ", "[H] ", "[I] ", "[J] ", "[K] ", "[L] ", "[M] ", "[N] ", "[O] ", "[P] "};
		
	char config_string[32];
	
	memset(config_string, '\0', 32);
	strcpy(config_string, letters[(channel - 4) / 2]);
	
	if ((channel > 2 && (channel) % 2 == 0)) {
		strcat(config_string, ADDRESS);
		sendConfigInfoSlider(channel, 1, 2048, eeprom_read_word((void *)(channel * 2)), config_string);
	} else if (channel > 2 && (channel + 1) % 2 == 0){
		strcat(config_string, FB_GREEN);
		sendConfigInfoSlider(channel, 1, 2048, eeprom_read_word((void *)(channel * 2)) , config_string);
	}
}

uint16_t getAccUID(uint8_t _index) {
	/* Generating accessory UID from address and protocol */
	if (eeprom_read_word((void *)((4 * _index) + 8)) == 0) return 0;
	if (eeprom_read_word((void *)2) == 0) {
		return 0x3800 + eeprom_read_word((void *)((4 * _index) + 8)) - 1;
	} else if (eeprom_read_word((void *)2) == 1) {
		return 0x3000 + eeprom_read_word((void *)((4 * _index) + 8)) - 1;
	}
	return 0;
}



//
// Status LED blinking pattern.
// Heartbeat blinking during normal operation.
//
void heartbeat() {
	
	if (heartbeat_millis >= 1200) {
		//setLow(statusPin);
		s_led_rising = 0;
		heartbeat_millis = 0;
		} else if (heartbeat_millis >= 1000) {
		//setHigh(statusPin);
		s_led_rising = 1;
		} else if (heartbeat_millis >= 800) {
		//setLow(statusPin);
		s_led_rising = 0;
		} else if (heartbeat_millis >= 600) {
		//setHigh(statusPin);
		s_led_rising = 1;
	}
	
	if (heartbeat_millis != last_heartbeat_millis) {
		last_heartbeat_millis = heartbeat_millis;
		
		if (s_led_rising == 1 && s_led_brightness_set <= (0xff - 3)) {
			s_led_brightness_set += 3;
			s_led_brightness = (uint8_t) pgm_read_byte (&ledLookupTable[s_led_brightness_set]);
			} else if (s_led_rising == 0 && s_led_brightness_set >= 3) {
			s_led_brightness_set -= 3;
			s_led_brightness = (uint8_t) pgm_read_byte (&ledLookupTable[s_led_brightness_set]);
		}
		
	}
	
}

void debounce() {
	/* Debouncing aux inputs */
	if (debounce_millis >= 20) {
		debounce_millis = 0;
		
		for (uint8_t i = 0; i < 16; i++) {
			uint8_t aux_1_read = readPin(aux_1[i]);
			uint8_t aux_2_read = readPin(aux_2[i]);
			
			if (aux_1_read == aux_1_laststate[i]) {
				/* Input is stable */
				aux_1[i].high = aux_1_laststate[i];
				} else {
				aux_1_laststate[i] = aux_1_read;
			}
			
			if (aux_2_read == aux_2_laststate[i]) {
				/* Input is stable */
				aux_2[i].high = aux_2_laststate[i];
				} else {
				aux_2_laststate[i] = aux_2_read;
			}
		}
	}
}

int main(void)
{
	/************************************************************************/
	/*  INIT                                                                */
	/************************************************************************/

	
	initPins();
	
	/* get serial number and calculate UID and hash */
	serial_nbr = (uint16_t)(boot_signature_byte_get(22) << 8) | boot_signature_byte_get(23);
	id = ~(0xfff0 + (readPin(dip_switch[0]) << 3) + (readPin(dip_switch[1]) << 2) + (readPin(dip_switch[2]) << 1) + readPin(dip_switch[3]));
	if (id == 0) {
		id = serial_nbr;
	}
	
	// Setup heartbeat timer:
	TCCR0A |= (1 << WGM01); // Timer 0 clear time on compare match
	OCR0A = (F_CPU / (64 * 1000UL)) - 1; // Timer 0 compare value
	TIMSK0 |= (1 << OCIE0A); // Timer interrupt
	TCCR0B |= ((1 << CS01)|(1 << CS00)); // Set timer 0 prescaler to 64
	
	// Setup dimm timer:
	TCCR1A |= (1 << WGM11); // Timer 1 clear time on compare match
	OCR1A = (F_CPU / (1 * 128000UL)) - 1;
	TIMSK1 |= (1 << OCIE1A);
	TCCR1B |= (1 << CS10);
	sei();
	
	/* Init CAN bus */
	mcan_init(BASE_UID + serial_nbr);
	
	/* check for connected headers and report current positions */
	for (uint8_t i = 0; i < MP5_COUNT; i++) {
		
		if (eeprom_read_word((void *)6)) {
			if (readPin(aux_1[i]) != readPin(aux_2[i])) {
				connected[i] = 1;
				
				is_position[i] = readPin(aux_1[i]);
				req_position[i] = is_position[i];
				
				if (is_position[i] == 1) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)), 0, 1);
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)) + 1, 1, 0);
					} else if (is_position[i] == 0) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)), 1, 0);
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)) + 1, 0, 1);
				}
				} else {
				connected[i] = 0;
			}
		} else {
			is_position[i] = eeprom_read_byte((void *)EEPROM_STATE_OFFSET + i);
			if (is_position[i] > 1) {
				eeprom_update_byte((void *)(EEPROM_STATE_OFFSET + i), 0);
				is_position[i] = 0;
			}
			req_position[i] = is_position[i];
			connected[i] = 1;
		}
	}
	
	
	
	/************************************************************************/
	/* MAIN LOOP                                                            */
	/************************************************************************/
    while (1) 
    {
		
		heartbeat();
		
		/* Handle new CAN frame */

		canFrame frame_in;
		if (readCanFrame(&frame_in) == 1) {
			
			switch (frame_in.cmd) {
				
				case 0x40 : { /* Bootloader request */
					/* THIS SECTION IS REQUIRED FOR THE BOOTLOADER TO WORK DURING NORMAL OPARATION */
					
					if (frame_in.resp == 0 && frame_in.dlc == 4) {
						if (compareUID(frame_in.data, mcan_uid) == 1) {
							
							/* Write bootloader request flag to EEPROM and reset. */
							eeprom_update_byte((void *)1023, 1);
							_delay_ms(100);
							reset_request = 1;
						}
					}
					break;
				}
				case CMD_PING : { /* Ping */
					if (frame_in.resp == 0) {
						sendPingFrame(VERSION, TYPE);
						break;
					}
				}
				case CMD_CONFIG : { /* Config data */
					if (frame_in.resp == 0 && compareUID(frame_in.data, mcan_uid) == 1) {
						switch (frame_in.data[4]) {
							case 0 : {
								sendDeviceInfo(id, 0, 3 + (MP5_COUNT * 2), ITEM, NAME);
								break;
							}
							case 1 : {
								sendConfigInfoDropdown(1, 2, eeprom_read_word((void *)2), "Protokoll_NRMA-DCC_Motorola");
								break;
							}
							case 2 : {
								sendConfigInfoSlider(2, 1, 2048, eeprom_read_word((void *)4), "R\u00FCckmelde-Bus_1_2048");
								break;
							}
							case 3 : {
								sendConfigInfoDropdown(3, 2, eeprom_read_word((void *)6), "Weichenr\u00FCckmeldung_Aus_An");
								break;
							}
							default : {
								configChannel(frame_in.data[4]);
								break;
							}
						}
					}
					break;
				}
				case SYS_CMD : {
					/* Write config data to EEPROM */
					if (frame_in.resp == 0 && compareUID(frame_in.data, mcan_uid) == 1 && frame_in.data[4] == SYS_STAT) {
						if (frame_in.dlc == 8) {
							eeprom_update_word((void *)(frame_in.data[5] * 2), (frame_in.data[6] << 8) + frame_in.data[7]);
							sendConfigConfirm(frame_in.data[5], 1);
						} else {
							/* Send channel value if requested */
							sendStatus(frame_in.data[5], eeprom_read_word((void *)(frame_in.data[5] * 2)));
						}
					}
				}
				case CMD_SWITCH_ACC : {
					/* Switch turnout */
					if (frame_in.resp == 0) {
						for (uint8_t i = 0; i < MP5_COUNT; i++) {
							if ((frame_in.data[2] << 8) + frame_in.data[3] == getAccUID(i) && frame_in.data[5] == 1 && connected[i] == 1 && getAccUID(i) > 0) {
								req_position[i] = frame_in.data[4];
								sendACCEvent((uint32_t)getAccUID(i), frame_in.data[4], 1);
							}
						}
						
						uint16_t acc_adr = (frame_in.data[2] << 8) + frame_in.data[3];
						uint8_t acc_state = frame_in.data[4];
						uint8_t acc_power = frame_in.data[5];
						
						if (acc_adr == MM(1) && acc_power == 1) {
							if (acc_state == 0) { 
								// Aktionen bei "Rot":
								setLow(poz_1[8]);
							} else { 
								// Aktionen bei "Gr�n":
								setHigh(poz_1[8]);
							}
							sendACCEvent((uint32_t)acc_adr, acc_state, acc_power);
						}						
					}
				}
				default : break;
			}
		}
		
		/* Update S88 feedback and is_position */
		if (eeprom_read_word((void *)6)) {
			for (uint8_t i = 0; i < MP5_COUNT; i++) {
				if (aux_1_laststate[i] == 1 && aux_1_lastreport[i] == 0 && active == 0 && connected[i] == 1) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)), 0, 1);
					aux_1_lastreport[i] = 1;
					is_position[i] = 1;
					sendACCEvent((uint32_t)getAccUID(i), 1, 0);
					} else if (aux_1_laststate[i] == 0 && aux_1_lastreport[i] == 1 && connected[i] == 1) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)), 1, 0);
					aux_1_lastreport[i] = 0;
				}
				if (aux_2_laststate[i] == 1 && aux_2_lastreport[i] == 0 && active == 0 && connected[i] == 1) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)) + 1, 0, 1);
					aux_2_lastreport[i] = 1;
					is_position[i] = 0;
					sendACCEvent((uint32_t)getAccUID(i), 0, 0 && connected[i] == 1);
					} else if (aux_2_laststate[i] == 0 && aux_2_lastreport[i] == 1) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)) + 1, 1, 0);
					aux_2_lastreport[i] = 0;
				}
				eeprom_update_byte((void *)EEPROM_STATE_OFFSET + i, is_position[i]);
			}
		}
		
		/* Switch output one at a time */
		for (uint8_t i = 0; i < MP5_COUNT; i++) {
			if (req_position[i] != is_position[i] && active == 0 && connected[i] == 1) {
				active = 1;
				if (req_position[i] == 1) {
					setLow(poz_1[i]);
					setHigh(poz_2[i]);
					if (!eeprom_read_word((void *)6)) is_position[i] = 1;
				} else if (req_position[i] == 0) {
					setLow(poz_2[i]);
					setHigh(poz_1[i]);
					if (!eeprom_read_word((void *)6)) is_position[i] = 0;
				}
				break;
			}
			if (!eeprom_read_word((void *)6)) eeprom_update_byte((void *)EEPROM_STATE_OFFSET + i, is_position[i]);
		}
		
		debounce();
		
		/* Reset switching time counter */
		if (active_millis >= 1500) {
			active = 0;
			active_millis = 0;
			for (uint8_t i = 0; i < MP5_COUNT; i++) {
				setLow(poz_1[i]);
				setLow(poz_2[i]);
			}
		}
		
		/* Software reset. */
		if (reset_request == 1) {
			
			reset_request = 0;
			cli();
			wdt_enable(WDTO_15MS);
			while(1);
		}
    }
}


/************************************************************************/
/*  INTERRUPT SERVICE ROUTINES                                          */
/************************************************************************/

ISR(TIMER0_COMPA_vect) {
	/* count up time counters */
	heartbeat_millis++;
	debounce_millis++;
	if (active == 1) {
		active_millis++;
	}	
}

ISR(TIMER1_COMPA_vect) {
		
	if (duty_cycle_counter == 0xff) {
		duty_cycle_counter = 0;
		} else {
		++duty_cycle_counter;
		}
		
	if (duty_cycle_counter < s_led_brightness && s_led_state == 0) {
		setHigh(statusPin);
		s_led_state = 1;
		} else if (duty_cycle_counter >= s_led_brightness  && s_led_state == 1) {
		setLow(statusPin);
		s_led_state = 0;
	}		
}