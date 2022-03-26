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
 * V 0.6 
 * [2022-03-26.2]
 */


#define NAME "M\u00E4CAN MP5+D"
#define BASE_UID 0x4D430000
#define ITEM "MP5+D"
#define VERSION 0x0006
#define TYPE 0x0052

#define MP5_COUNT 8

#define EEPROM_STATE_OFFSET 1024
#define EEPROM_BOOTLOADER_BYTE 1023

#define CHANNEL_OFFSET  3 + (MP5_COUNT * 2)
#define FEEDBACK_ENABLED eeprom_read_word((void *)6)
#define TRACK_PROTOCOL eeprom_read_word((void *)2)

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

/*
uint32_t heartbeat_millis;				// Time counter for status LED
uint32_t last_heartbeat_millis;*/
uint64_t millis_counter;
uint16_t blink_clock;
uint8_t debounce_millis;				// Time counter for input debouncing

uint8_t aux_1_laststate[16];			// Last read value of aux 1 input
uint8_t aux_2_laststate[16];			// Last read value of aux 2 input
uint8_t aux_1_lastreport[16] = {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2};			// Last reported value of aux 1 input
uint8_t aux_2_lastreport[16] = {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2};			// Last reported value of aux 2 input
uint64_t aux_millis[16];
	

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

uint8_t output_states[16];

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
	#define ADDRESS "Adresse_1_2048"
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
		sendConfigInfoSlider(channel, 0, 2048, eeprom_read_word((void *)(channel * 2)) , config_string);
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
	
	setupMsTimer();
	setupPwmTimer();
	
	sei();
	
	/* Init CAN bus */
	mcan_init(BASE_UID + serial_nbr);
	
	for (uint8_t i = 0; i < 16; i++)
	{
		aux_1[i].high = readPin(aux_1[i]);
		aux_2[i].high = readPin(aux_2[i]);
	}
	
	/* check for connected headers and report current positions */
	for (uint8_t i = 0; i < MP5_COUNT; i++) {
		
		if (eeprom_read_word((void *)6)) {
			if (readPin(aux_1[i]) != readPin(aux_2[i])) {
				connected[i] = 1;
				
				is_position[i] = readPin(aux_1[i]);
				req_position[i] = is_position[i];
				
				if (is_position[i] == 1) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)), 1, 0);
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)) + 1, 0, 1);
				} else if (is_position[i] == 0) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)), 0, 1);
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)) + 1, 1, 0);
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
	
	/* Output Init */
	if (eeprom_read_word((void *)((CHANNEL_OFFSET + 3)*2)) == 1) {
		for (uint8_t i = 0; i < 8; i++) {
			switchOutput(i*2, 0, 0);
			switchOutput(i*2+1, 0xff, 0);
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
							eeprom_update_byte((void *)EEPROM_BOOTLOADER_BYTE, 1);
							_delay_ms(100);
							reset_request = 1;
						}
					}
					break;
				}
				case CMD_PING : { /* Ping */
					if (frame_in.resp == 0) {
						sendPingFrame(VERSION, TYPE);
					}
					break;
				}
				case CMD_CONFIG : { /* Config data */
					if (frame_in.resp == 0 && compareUID(frame_in.data, mcan_uid) == 1) {
						
						switch (frame_in.data[4]) {
							case 0 : {
								sendDeviceInfo(id, 0, CHANNEL_OFFSET + 5, ITEM, NAME);
								break;
							}
							case 1 : {
								sendConfigInfoDropdown(1, 2, eeprom_read_word((void *)2), "Protokoll_NRMA-DCC_Motorola");
								break;
							}
							case 2 : {
								sendConfigInfoSlider(2, 0, 2048, eeprom_read_word((void *)4), "R\u00FCckmelde-Bus_0_2048");
								break;
							}
							case 3 : {
								sendConfigInfoDropdown(3, 2, eeprom_read_word((void *)6), "Weichenr\u00FCckmeldung_Aus_An");
								break;
							}
							case (CHANNEL_OFFSET + 1) : {
								sendConfigInfoSlider(CHANNEL_OFFSET + 1, 1, 2048, eeprom_read_word((void *)((CHANNEL_OFFSET + 1)*2)), "Erste R\u00FCckmeldeadresse_1_2048");
								break;
							}
							case (CHANNEL_OFFSET + 2) : {
								sendConfigInfoSlider(CHANNEL_OFFSET + 2, 1, 2048, eeprom_read_word((void *)((CHANNEL_OFFSET + 2)*2)), "Erste Schaltadresse_1_2048");
								break;
							}
							case (CHANNEL_OFFSET + 3) : {
								sendConfigInfoDropdown(CHANNEL_OFFSET + 3, 2, eeprom_read_word((void *)((CHANNEL_OFFSET + 3)*2)), "Ausgangsart_Einfach_Paarweise");
								break;
							}
							case (CHANNEL_OFFSET + 4) : {
								sendConfigInfoSlider(CHANNEL_OFFSET + 4, 0, 2000, eeprom_read_word((void *)((CHANNEL_OFFSET + 4)*2)), "Schaltzeit_0_2000_ms");
								break;
							}
							case (CHANNEL_OFFSET + 5) : {
								sendConfigInfoSlider(CHANNEL_OFFSET + 5, 0, 2000, eeprom_read_word((void *)((CHANNEL_OFFSET + 5)*2)), "Verz\u00f6gerung_0_2000_ms");
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
					break;
				}
				case CMD_SWITCH_ACC : {
					/* Switch turnout */
					if (frame_in.resp == 0) {
						
						// Weichensteuerung
						
						for (uint8_t i = 0; i < MP5_COUNT; i++) {
							if ((frame_in.data[2] << 8) + frame_in.data[3] == getAccUID(i) /*&& frame_in.data[5] == 1*/ && connected[i] == 1 && getAccUID(i) > 0) {
								if (frame_in.data[5] == 1) {
									req_position[i] = frame_in.data[4];
									sendACCEvent((uint32_t)getAccUID(i), frame_in.data[4], 1);
									if (req_position[i] == is_position[i]) sendACCEvent((uint32_t)getAccUID(i), frame_in.data[4], 0);
								} else {
									if(FEEDBACK_ENABLED == 0) sendACCEvent((uint32_t)getAccUID(i), frame_in.data[4], 0);
								}
							}
						}
						
						uint16_t acc_adr = (frame_in.data[2] << 8) + frame_in.data[3];
						uint8_t acc_state = frame_in.data[4];
						uint8_t acc_power = frame_in.data[5];						
						
						uint16_t adr_protocol;
						if (TRACK_PROTOCOL == 0) adr_protocol = 0x3800;
						else adr_protocol = 0x3000;
						
						if (eeprom_read_word((void *)((CHANNEL_OFFSET + 3)*2)) == 1) {
							for (uint8_t i = 0; i < 8; i++) {
								if (acc_adr == adr_protocol + eeprom_read_word((void *)((CHANNEL_OFFSET + 2)*2)) + i - 1) {
									if (acc_state != output_states[i]) {
										if (acc_state == 1) {
											switchOutput(i*2, 0xff, 0);
											switchOutput(i*2+1, 0, 0);
											} else {
											switchOutput(i*2, 0, 0);
											switchOutput(i*2+1, 0xff, 0);
										}
										output_states[i] = acc_state;
										sendACCEvent((uint32_t)acc_adr, acc_state, acc_power);
									}
								}
							}
						} else {
							for (uint8_t i = 0; i < 16; i++) {
								if (acc_adr == adr_protocol + eeprom_read_word((void *)((CHANNEL_OFFSET + 2)*2)) + i - 1) {
									if (acc_state != output_states[i]) {
										switchOutput(i, acc_state * 0xff, 0);
										output_states[i] = acc_state;
										sendACCEvent((uint32_t)acc_adr, acc_state, acc_power);
									}
								}
							}
						}
						
						
												
					}
					break;
				}
				default : {
					break;
				}
			}
		}
		
		/* Update S88 feedback and is_position */
		if (FEEDBACK_ENABLED) {
			for (uint8_t i = 0; i < MP5_COUNT; i++) {
				if (aux_1[i].high == 1 && aux_1_lastreport[i] != 1 && active == 0 && connected[i] == 1) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)), 1, 0);
					aux_1_lastreport[i] = 1;
					is_position[i] = 1;
					sendACCEvent((uint32_t)getAccUID(i), 1, 0);
				} else if (aux_1[i].high == 0 && aux_1_lastreport[i] != 0 && active == 0 && connected[i] == 1) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)), 0, 1);
					aux_1_lastreport[i] = 0;
				}
				if (aux_2[i].high == 1 && aux_2_lastreport[i] != 1 && active == 0 && connected[i] == 1) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)) + 1, 1, 0);
					aux_2_lastreport[i] = 1;
					is_position[i] = 0;
					sendACCEvent((uint32_t)getAccUID(i), 0, 0);
				} else if (aux_2[i].high == 0 && aux_2_lastreport[i] != 0 && active == 0  && connected[i] == 1) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)) + 1, 0, 1);
					aux_2_lastreport[i] = 0;
				}
				eeprom_update_byte((void *)EEPROM_STATE_OFFSET + i, is_position[i]);
			}
		} else {
			/* Send normal input events if feedback disabled */
			for (uint8_t i = 0; i < MP5_COUNT; i++) {
				uint16_t S88_address = eeprom_read_word((void*)((CHANNEL_OFFSET + 1) * 2)) + 2 * i;
				
				if (aux_1[i].high != aux_1_lastreport[i]) {
					sendS88Event(S88_address, aux_1[i].high, !aux_1[i].high);
					aux_1_lastreport[i] = aux_1[i].high;
				}
				if (aux_2[i].high != aux_2_lastreport[i]) {
					sendS88Event(S88_address + 1, aux_2[i].high, !aux_2[i].high);
					aux_2_lastreport[i] = aux_2[i].high;
				}
			}
		}
		
		/* Inputs not assigned to MP5 */
		for (uint8_t i = MP5_COUNT; i < 16; i++) {
			uint16_t S88_address = 0;
			
			if (FEEDBACK_ENABLED) S88_address = eeprom_read_word((void*)((CHANNEL_OFFSET + 1) * 2)) + 2 * (i - MP5_COUNT);
			else S88_address = eeprom_read_word((void*)((CHANNEL_OFFSET + 1) * 2)) + 2 * i;
			
			if (aux_1[i].high != aux_1_lastreport[i]) {
				sendS88Event(S88_address, aux_1[i].high, !aux_1[i].high);
				aux_1_lastreport[i] = aux_1[i].high;
			}
			if (aux_2[i].high != aux_2_lastreport[i]) {
				sendS88Event(S88_address + 1, aux_2[i].high, !aux_2[i].high);
				aux_2_lastreport[i] = aux_2[i].high;
			}
		}
		
		/* Switch output one at a time */
		for (uint8_t i = 0; i < MP5_COUNT; i++) {
			if (aux_millis[i] > 0 && (millis_counter - aux_millis[i]) > eeprom_read_word((void *)((CHANNEL_OFFSET + 4)*2))) {
				setLow(poz_1[i]);
				setLow(poz_2[i]);
				aux_millis[i] = 0;
			}
			if (req_position[i] != is_position[i] && active == 0 && connected[i] == 1 && aux_millis[i] == 0) {
				active = i+1;
				if (req_position[i] == 1) {
					setLow(poz_1[i]);
					setHigh(poz_2[i]);
					if (!eeprom_read_word((void *)6)) is_position[i] = 1;
				} else if (req_position[i] == 0) {
					setLow(poz_2[i]);
					setHigh(poz_1[i]);
					if (!eeprom_read_word((void *)6)) is_position[i] = 0;
				}
				if (eeprom_read_word((void *)((CHANNEL_OFFSET + 4)*2)) > 0)
					aux_millis[i] = millis_counter;
				break;
			}
			if (!eeprom_read_word((void *)6)) eeprom_update_byte((void *)EEPROM_STATE_OFFSET + i, is_position[i]);
		}
		
		debounce();
		
		/* Reset switching time counter */
		if (active_millis > eeprom_read_word((void *)((CHANNEL_OFFSET + 5)*2))) {
			if (FEEDBACK_ENABLED == 0) sendACCEvent(getAccUID(active-1), req_position[active-1], 0);
			active = 0;
			active_millis = 0;
			//for (uint8_t i = 0; i < MP5_COUNT; i++) {
			//	setLow(poz_1[i]);
			//	setLow(poz_2[i]);
			//}
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
	ledMillisCounter();
	millis_counter++;
	debounce_millis++;
	if (active >= 1) {
		active_millis++;
	}	
	
	blink_clock++; 
	if (blink_clock >= 1000) {
		blink_clock = 0;
	}
	
}

ISR(TIMER1_COMPA_vect) {
	dutyCycleCounter();
}