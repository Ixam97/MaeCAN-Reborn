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
 * V 1.1 
 * [2020-08-27.1]
 */

#define F_CPU 16000000UL

#define NAME "M\u00E4CAN 16-fach MP5-Decoder"
#define BASE_UID 0x4D430000
#define ITEM "MP5x16"
#define VERSION 0x0101
#define TYPE 0x0052

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "mcp2515_basic.h"
#include "mcan.h"
#include "MP5x16v1.0_Pindefs.h"

uint32_t uid;
uint16_t hash;
uint16_t serial_nbr;
uint16_t id;

canFrame frame_in, frame_in_buffer;		// Buffers for incoming frames
uint8_t new_frame = 0;					// New frame awaiting processing

uint8_t reset_request;					// 1 if software reset is requested, else 0

uint32_t heartbeat_millis;				// Time counter for status LED
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



uint8_t compareUID(uint8_t data[8], uint32_t _uid) {
	/* Compare UID inside frame data to another UID */
	
	if ((data[0] == (uint8_t)(_uid >> 24)) && (data[1] == (uint8_t)(_uid >> 16)) && (data[2] == (uint8_t)(_uid >> 8)) && (data[3] == (uint8_t)_uid)) {
		return 1;
		} else {
		return 0;
	}
}

void configChannel(uint8_t channel) {
	/* generate config channels for all outputs */
	#define ADDRESS "Adresse_1_2048"
	#define FB_GREEN "R\u00FCckmelder_1_2048"
	
	char letters[][5] = {"[A] ", "[B] ", "[C] ", "[D] ", "[E] ", "[F] ", "[G] ", "[H] ", "[I] ", "[J] ", "[K] ", "[L] ", "[M] ", "[N] ", "[O] ", "[P] "};
		
	char config_string[32];
	
	memset(config_string, '\0', 32);
	strcpy(config_string, letters[(channel - 3) / 2]);
	
	if (channel > 2 && (channel + 1) % 2 == 0) {
		strcat(config_string, ADDRESS);
		sendConfigInfoSlider(uid, hash, channel, 1, 2048, eeprom_read_word(channel * 2), config_string);
	} else if (channel > 2 && channel % 2 == 0){
		strcat(config_string, FB_GREEN);
		sendConfigInfoSlider(uid, hash, channel, 1, 2048, eeprom_read_word(channel * 2) , config_string);
	}
}

uint16_t getAccUID(uint8_t _index) {
	/* Generating accessory UID from address and protocol */
	if (eeprom_read_word(2) == 0) {
		return 0x3800 + eeprom_read_word((4 * _index) + 6) - 1;
	} else if (eeprom_read_word(2) == 1) {
		return 0x3000 + eeprom_read_word((4 * _index) + 6) - 1;
	}
}

void heartbeat() {
	/* Status LED blinking pattern. */
	
	/* Heartbeat blinking during normal operation */
	if (heartbeat_millis >= 1000) {
		setLow(statusPin);
		heartbeat_millis = 0;
	} else if (heartbeat_millis >= 900) {
		setHigh(statusPin);
	} else if (heartbeat_millis >= 800) {
		setLow(statusPin);
	} else if (heartbeat_millis >= 700) {
		setHigh(statusPin);
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
	id = ~(0xfff0 + (readPin(dip_switch[3]) << 3) + (readPin(dip_switch[2]) << 2) + (readPin(dip_switch[1]) << 1) + readPin(dip_switch[0]));
	if (id == 0) {
		id = serial_nbr;
	}
	uid = BASE_UID + serial_nbr;
	hash = generateHash(uid);
	
	/* Setup ext. interrupt on falling edge */
	cli();
	EICRA |= (1 << ISC21);
	EICRA &= ~(1 << ISC20);
	EIMSK |= (1 << INT2);
	setInput(intPin);
	setHigh(intPin);
	
	/* Setup heartbeat timer */
	TCCR0A |= (1 << WGM01);
	OCR0A = 249;
	TIMSK0 |= (1 << OCIE0A);
	sei();
	TCCR0B |= ((1 << CS01)|(1 << CS00));
	
	/* Init CAN bus */
	mcp2515_init();
	
	/* check for connected headers and report current positions */
	for (uint8_t i = 0; i < 16; i++) {
		if (readPin(aux_1[i]) != readPin(aux_2[i])) {
			connected[i] = 1;
			
			is_position[i] = readPin(aux_1[i]);
			req_position[i] = is_position[i];
			
			if (is_position[i] == 1) {
				sendS88Event(((uint32_t)eeprom_read_word(4) << 16) + eeprom_read_word((i*4)+8), hash, 0, 1);
				sendS88Event(((uint32_t)eeprom_read_word(4) << 16) + eeprom_read_word((i*4)+8) + 1, hash, 1, 0);
			} else if (is_position[i] == 0) {
				sendS88Event(((uint32_t)eeprom_read_word(4) << 16) + eeprom_read_word((i*4)+8), hash, 1, 0);
				sendS88Event(((uint32_t)eeprom_read_word(4) << 16) + eeprom_read_word((i*4)+8) + 1, hash, 0, 1);
			}
		} else {
			connected[i] = 0;
		}
	}
	
	
	
	/************************************************************************/
	/* MAIN LOOP                                                            */
	/************************************************************************/
	
    while (1) 
    {
		/* Handle new CAN frame */
		if (new_frame == 1) {
			new_frame = 0;
			getCanFrame(&frame_in);
			
			switch (frame_in.cmd) {
				
				case 0x40 : { /* Bootloader request */
					/* THIS SECTION IS REQUIRED FOR THE BOOTLOADER TO WORK DURING NORMAL OPARATION */
					
					if (frame_in.resp == 0 && frame_in.dlc == 4) {
						if (compareUID(frame_in.data, uid) == 1) {
							
							/* Write bootloader request flag to EEPROM and reset. */
							eeprom_update_byte(1023, 1);
							_delay_ms(100);
							reset_request = 1;
						}
					}
					break;
				}
				case CMD_PING : { /* Ping */
					if (frame_in.resp == 0) {
						sendPingFrame(uid, hash, VERSION, TYPE);
						break;
					}
				}
				case CMD_CONFIG : { /* Config data */
					if (frame_in.resp == 0 && compareUID(frame_in.data, uid) == 1) {
						switch (frame_in.data[4]) {
							case 0 : {
								sendDeviceInfo(uid, hash, id, 0, 34, ITEM, NAME);
								break;
							}
							case 1 : {
								sendConfigInfoDropdown(uid, hash, 1, 2, eeprom_read_word(2), "Protokoll_NRMA-DCC_Motorola");
								break;
							}
							case 2 : {
								sendConfigInfoSlider(uid, hash, 2, 1, 2048, eeprom_read_word(4), "R\u00FCckmelde-Bus_1_2048");
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
					if (frame_in.resp == 0 && compareUID(frame_in.data, uid) == 1 && frame_in.data[4] == SYS_STAT) {
						eeprom_update_word(frame_in.data[5] * 2, (frame_in.data[6] << 8) + frame_in.data[7]);
						sendConfigConfirm(uid, hash, frame_in.data[5]);
					}
				}
				case CMD_SWITCH_ACC : {
					/* Switch turnout */
					if (frame_in.resp == 0) {
						for (uint8_t i = 0; i < 16; i++) {
							if ((frame_in.data[2] << 8) + frame_in.data[3] == getAccUID(i) && frame_in.data[5] == 1 && connected[i] == 1) {
								req_position[i] = frame_in.data[4];
								sendACCEvent((uint32_t)getAccUID(i), hash, frame_in.data[4], 1);
							}
						}
					}
				}
				default : break;
			}
		}
		
		/* Update S88 feedback and is_position */
		for (uint8_t i = 0; i < 16; i++) {
			if (aux_1_laststate[i] == 1 && aux_1_lastreport[i] == 0 && active == 0 && connected[i] == 1) {
				sendS88Event(((uint32_t)eeprom_read_word(4) << 16) + eeprom_read_word((i*4)+8), hash, 0, 1);
				aux_1_lastreport[i] = 1;
				is_position[i] = 1;
				sendACCEvent((uint32_t)getAccUID(i), hash, 1, 0);
			} else if (aux_1_laststate[i] == 0 && aux_1_lastreport[i] == 1 && connected[i] == 1) {
				sendS88Event(((uint32_t)eeprom_read_word(4) << 16) + eeprom_read_word((i*4)+8), hash, 1, 0);
				aux_1_lastreport[i] = 0;
			}
			if (aux_2_laststate[i] == 1 && aux_2_lastreport[i] == 0 && active == 0 && connected[i] == 1) {
				sendS88Event(((uint32_t)eeprom_read_word(4) << 16) + eeprom_read_word((i*4)+8) + 1, hash, 0, 1);
				aux_2_lastreport[i] = 1;
				is_position[i] = 0;
				sendACCEvent((uint32_t)getAccUID(i), hash, 0, 0 && connected[i] == 1);
			} else if (aux_2_laststate[i] == 0 && aux_2_lastreport[i] == 1) {
				sendS88Event(((uint32_t)eeprom_read_word(4) << 16) + eeprom_read_word((i*4)+8) + 1, hash, 1, 0);
				aux_2_lastreport[i] = 0;
			}
		}
		
		/* Switch output one at a time */
		for (uint8_t i = 0; i < 16; i++) {
			if (req_position[i] != is_position[i] && active == 0 && connected[i] == 1) {
				active = 1;
				if (req_position[i] == 1) {
					setLow(poz_1[i]);
					setHigh(poz_2[i]);
				} else if (req_position[i] == 0) {
					setLow(poz_2[i]);
					setHigh(poz_1[i]);
				}
				break;
			}
		}
		
		debounce();
		
		/* Reset switching time counter */
		if (active_millis >= 1500) {
			active = 0;
			active_millis = 0;
			for (uint8_t i = 0; i < 16; i++) {
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

ISR(INT2_vect) {
	/* MCP2515 interrupt on new CAN-frame. */
	new_frame = 1;
}

ISR(TIMER0_COMPA_vect) {
	/* count up time counters */
	heartbeat_millis++;
	heartbeat();
	debounce_millis++;
	if (active == 1) {
		active_millis++;
	}
}