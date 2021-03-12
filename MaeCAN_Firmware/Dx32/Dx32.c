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

/* 
 * ----------------------------------------------------------------------------
 * EEPROM ADDRESS MAP
 * ----------------------------------------------------------------------------
 * 0x03ff : Bootloader request flag
 * 0x0000 : LED brightness
 * 0x0002 : Number of used inputs
 * 0x0004 : Number of sense inputs
 * 0x0006 : Bus ID
 * 0x0008 : Start address of first input
 */
#define EEPROM_LED_BRIGHTNESS	0x0000
#define EEPROM_NUM_INPUTS		0x0002
#define EEPROM_NUM_SENSE		0x0004
#define EEPROM_BUS_ID			0x0006
#define EEPROM_START_ADR		0x0008
#define EEPROM_BOOTLOADER_FLAG	0x03ff

// Maximum values for config chanels
#define MAX_LED_BRIGHTNESS		0xff
#define MAX_NUM_INPUTS			32
#define MAX_NUM_SENSE			4
#define MAX_BUS_ID				0xffff
#define MAX_START_ADR			0xffff

#define NAME "M\u00E4CAN 32-fach Gleisbelegtmelder"
#define BASE_UID 0x4D430000
#define ITEM "Dx32"
#define VERSION 0x0100
#define TYPE 0x0053

#include <stdlib.h>
#include <avr/io.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "../Common/mcan.h"
#include "Dx32v1.0_Pindefs.h"

const uint8_t ledLookupTable[] PROGMEM = {
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
	  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  4,  4,
	  4,  4,  5,  5,  5,  5,  6,  6,  6,  7,  7,  7,  8,  8,  8,  9,
	  9,  9, 10, 10, 11, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16,
	 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 23, 23, 24, 24,
	 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32, 32, 33, 34, 35, 35,
	 36, 37, 38, 38, 39, 40, 41, 42, 42, 43, 44, 45, 46, 47, 47, 48,
	 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58, 59, 60, 61, 62, 63,
	 64, 65, 66, 67, 68, 69, 70, 71, 73, 74, 75, 76, 77, 78, 79, 80,
	 81, 82, 84, 85, 86, 87, 88, 89, 91, 92, 93, 94, 95, 97, 98, 99,
	100,102,103,104,105,107,108,109,111,112,113,115,116,117,119,120,
	121,123,124,126,127,128,130,131,133,134,136,137,139,140,142,143,
	145,146,148,149,151,152,154,155,157,158,160,162,163,165,166,168,
	170,171,173,175,176,178,180,181,183,185,186,188,190,192,193,195,
	197,199,200,202,204,206,207,209,211,213,215,217,218,220,222,224,
	226,228,230,232,233,235,237,239,241,243,245,247,249,251,253,255
	};

canFrame frame_in;

uint32_t uid;
uint16_t hash;
uint16_t serial_nbr;
uint16_t id;

uint8_t silent;							// Ignore ping requests

uint8_t reset_request;					// 1 if software reset is requested, else 0

uint32_t heartbeat_millis;				// Time counter for status LED
uint32_t last_heartbeat_millis;

char uart_to_can_buffer[25];
uint8_t uart_to_can_buffer_index;

uint8_t t_led_brightness;
uint8_t t_led_brightness_set;

uint8_t num_inputs;		// Number of used inputs

uint16_t bus_id;		// ID of the S88 Bus

uint16_t start_adr;		// Address of the first input

uint8_t s_led_state;
uint8_t s_led_brightness = 0;
uint8_t s_led_brightness_set = 0;
uint8_t s_led_rising = 1;

uint8_t duty_cycle_counter;
uint8_t last_duty_cycle_counter;
uint8_t new_duty_cycle;

uint8_t trk_state_actual[4];							// actual electric occupation state
uint8_t trk_state_logic[4] = {0xff, 0xff, 0xff, 0xff};	// logic occupation state
uint8_t trk_state_last[4];								// last logic occupation state
uint16_t trk_timeout[4][8];								// timeout counters
uint8_t trk_timeout_flag[4];							// timeout counter flags, high bit means count up
uint16_t trk_timeout_millis = 1500;						// timeout time in milliseconds
uint8_t trk_sense_flag = 0xff;							// high bit indicates missing track power, one bit per four ports
uint8_t trk_sense_mask[4];								// masking ports to be handled as sense port (no timeout, change when power is off)
uint8_t trk_sense_ports;								// number of sense ports (0,1,2,4,8)
const uint8_t trk_sense_ports_table[] PROGMEM = {0,1,2,4,8};
const uint8_t trk_sense_reverse_table[] PROGMEM = {0,1,2,0,3,0,0,0,4};
uint8_t trk_LEDs[4];									// LED state to be set
uint8_t trk_blinker;									// blink value provider
uint16_t trk_blinker_millis;							// blink counter

uint8_t current_input;
uint8_t last_input;

//
// Compare UID inside frame data to another UID.
//
uint8_t compareUID(uint8_t data[8], uint32_t _uid) {

	if ((data[0] == (uint8_t)(_uid >> 24)) && (data[1] == (uint8_t)(_uid >> 16)) && (data[2] == (uint8_t)(_uid >> 8)) && (data[3] == (uint8_t)_uid)) {
		return 1;
		} else {
		return 0;
	}
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
	
	if (trk_blinker_millis >= 500) {
		trk_blinker = 0;
		trk_blinker_millis = 0;
	} else if (trk_blinker_millis >= 250) {
		trk_blinker = 1;
	}
	
	if (heartbeat_millis != last_heartbeat_millis) {
		last_heartbeat_millis = heartbeat_millis;
		
		for (uint8_t i = 0; i < 4; i++) {
			for (uint8_t j = 0; j < 8; j++) {
				// Track occupation timeout
				trk_timeout[i][j] += (trk_timeout_flag[i] >> j) & 0b1;
			}
		}
		
		if (s_led_rising == 1 && s_led_brightness_set <= (0xff - 3)) {
			s_led_brightness_set += 3;
			s_led_brightness = (uint8_t) pgm_read_byte (&ledLookupTable[s_led_brightness_set]);
		} else if (s_led_rising == 0 && s_led_brightness_set >= 3) {
			s_led_brightness_set -= 3;
			s_led_brightness = (uint8_t) pgm_read_byte (&ledLookupTable[s_led_brightness_set]);
		}
		
	}	
	
}

//
// Update sense flags and masks.
//
void applySenseFlagsMasks(void) {
	switch (trk_sense_ports) {
		case 8: {
			trk_sense_flag = (TRKA & 0b00000001) + ((TRKA >> 6) & 0b00000010) + ((TRKB << 2) & 0b00000100) + ((TRKB >> 4) & 0b00001000) + ((TRKC << 4) & 0b00010000) + ((TRKC >> 2) & 0b00100000) + ((TRKD << 6) & 0b01000000) + (TRKD & 0b10000000);
			trk_sense_mask[0] = 0b10000001;
			trk_sense_mask[1] = 0b10000001;
			trk_sense_mask[2] = 0b10000001;
			trk_sense_mask[3] = 0b10000001;
			break;
		}
		case 4: {
			trk_sense_flag = (TRKA & 0b00000001) + ((TRKA << 1) & 0b00000010) + ((TRKB << 2) & 0b00000100) + ((TRKB << 3) & 0b00001000) + ((TRKC << 4) & 0b00010000) + ((TRKC << 5) & 0b00100000) + ((TRKD << 6) & 0b01000000) + ((TRKD << 7) & 0b10000000);
			trk_sense_mask[0] = 0b00000001;
			trk_sense_mask[1] = 0b00000001;
			trk_sense_mask[2] = 0b00000001;
			trk_sense_mask[3] = 0b00000001;
			break;
		}
		case 2: {
			trk_sense_flag = (TRKA & 0b00000001) + ((TRKA << 1) & 0b00000010) + ((TRKA << 2) & 0b00000100) + ((TRKA << 3) & 0b00001000) + ((TRKC << 4) & 0b00010000) + ((TRKC << 5) & 0b00100000) + ((TRKC << 6) & 0b01000000) + ((TRKC << 7) & 0b10000000);
			trk_sense_mask[0] = 0b00000001;
			trk_sense_mask[1] = 0b00000000;
			trk_sense_mask[2] = 0b00000001;
			trk_sense_mask[3] = 0b00000000;
			break;
		}
		case 1: {
			trk_sense_flag = (TRKA & 0b00000001) + ((TRKA << 1) & 0b00000010) + ((TRKA << 2) & 0b00000100) + ((TRKA << 3) & 0b00001000) + ((TRKA << 4) & 0b00010000) + ((TRKA << 5) & 0b00100000) + ((TRKA << 6) & 0b01000000) + ((TRKA << 7) & 0b10000000);
			trk_sense_mask[0] = 0b00000001;
			trk_sense_mask[1] = 0b00000000;
			trk_sense_mask[2] = 0b00000000;
			trk_sense_mask[3] = 0b00000000;
			break;
		}
		default: {
			trk_sense_flag = 0;
			trk_sense_mask[0] = 0b00000000;
			trk_sense_mask[1] = 0b00000000;
			trk_sense_mask[2] = 0b00000000;
			trk_sense_mask[3] = 0b00000000;
			break;
		}
	}
}

//
// Updates for config values.
// Returns 1 on success, 0 when value too large.
//
uint8_t updateLEDBrightness(uint16_t _value) {
	if (_value <= MAX_LED_BRIGHTNESS) {
		t_led_brightness_set = _value;
		t_led_brightness = (uint8_t) pgm_read_byte (&ledLookupTable[t_led_brightness_set]);
		eeprom_update_word((void*)EEPROM_LED_BRIGHTNESS, t_led_brightness_set);
		return 1;
	}
	return 0;
}

uint8_t updateNumInputs(uint16_t _value) {
	if (_value <= MAX_NUM_INPUTS) {
		num_inputs = _value;
		eeprom_update_word((void *)EEPROM_NUM_INPUTS, num_inputs);
		return 1;
	}
	return 0;
}

uint8_t updateSensePorts(uint16_t _value) {
	if (_value <= MAX_NUM_SENSE) {
		trk_sense_ports = (uint8_t)pgm_read_byte(&trk_sense_ports_table[_value]);
		eeprom_update_word((void*)EEPROM_NUM_SENSE, trk_sense_ports);
		return 1;
	}
	return 0;
}

uint8_t updateBusID(uint16_t _value) {
	if (_value <= MAX_BUS_ID) {
		bus_id = _value;
		eeprom_update_word((void*)EEPROM_BUS_ID, bus_id);
		return 1;
	}
	return 0;
}

uint8_t updateStartAdr(uint16_t _value) {
	if (_value <= MAX_START_ADR) {
		start_adr = _value;
		eeprom_update_word((void *)EEPROM_START_ADR, start_adr);
		return 1;
	}
	return 0;
}

//
// Report Inputs when requested.
//
void reportInput(uint16_t _s88_bus_id, uint16_t _s88_port_id) {
	if (_s88_port_id >= start_adr && _s88_port_id < start_adr + num_inputs && _s88_bus_id == bus_id) {
		uint8_t index = _s88_port_id - start_adr;
		sendS88Event(((uint32_t)_s88_bus_id << 16) + _s88_port_id, hash, (trk_state_logic[index / 8] >> (index % 8)) & 0b1, (trk_state_last[index / 8] >> (index % 8)) & 0b1);
	}
}

void reportAllInputs(void) {
	for (uint8_t i = 0; i < num_inputs; i++) {
		sendS88Event(((uint32_t)bus_id << 16) + i + start_adr, hash, (trk_state_logic[i / 8] >> (i % 8)) & 0b1, (trk_state_last[i / 8] >> (i % 8)) & 0b1);
	}
}

//
// Fun with Frames.
//
void handleCanFramePointer(canFrame *frame_in) {
	
	switch (frame_in->cmd) {
		
		case 0x40 : {
			// Bootloader request
			// THIS SECTION IS REQUIRED FOR THE BOOTLOADER TO WORK DURING NORMAL OPARATION
			
			if (frame_in->resp == 0 && frame_in->dlc == 4) {
				if (compareUID(frame_in->data, uid) == 1) {
					
					// Write bootloader request flag to EEPROM and reset.
					eeprom_update_byte((void *)EEPROM_BOOTLOADER_FLAG, 1);
					_delay_ms(100);
					reset_request = 1;
				}
			}
			break;
		}
		case CMD_PING : {
			// Ping
			if (frame_in->resp == 0 && silent == 0) sendPingFrame(uid, hash, VERSION, TYPE);
			break;
		}
		case CMD_S88_EVENT : {
			// S88 querry
			if (frame_in->resp == 0 && frame_in->dlc == 0) {
				reportAllInputs();
			} else if (frame_in->resp == 0 && frame_in->dlc == 2) {
				if (((uint16_t)frame_in->data[0] << 8) + ((uint16_t)frame_in->data[1]) == bus_id) reportAllInputs();
			} else if (frame_in->resp == 0 && frame_in->dlc == 4) {
				reportInput(((uint16_t)frame_in->data[0] << 8) + ((uint16_t)frame_in->data[1]), ((uint16_t)frame_in->data[2] << 8) + ((uint16_t)frame_in->data[3]));
			}
			break;
		}
		case CMD_CONFIG : {
			// Config data
			if (frame_in->resp == 0 && compareUID(frame_in->data, uid) == 1 && silent == 0) {
				switch (frame_in->data[4]) {
					case 0 :
					sendDeviceInfo(uid, hash, id, 0, 5, ITEM, NAME);
					break;
					case 1 :
					sendConfigInfoSlider(uid, hash, 1, 0, 0xff, t_led_brightness_set, "Melder-Helligkeit_0_255");
					break;
					case 2 :
					sendConfigInfoSlider(uid, hash, 2, 1, 32, num_inputs, "Verwendete Eing\u00E4nge_1_32");
					break;
					case 3 :
					sendConfigInfoDropdown(uid, hash, 3, 5, (uint8_t)pgm_read_byte(&trk_sense_reverse_table[trk_sense_ports]), "Anzahl Sense-Eing\u00E4nge_0_1_2_4_8");
					break;
					case 4:
					sendConfigInfoSlider(uid, hash, 4, 0, 0xffff, 0, "Buskennung_0_65535");
					break;
					case 5:
					sendConfigInfoSlider(uid, hash, 5, 1, 0xffff, 1, "Startadresse_1_65535");
					break;
					default : {
						break;
					}
				}
			}
			break;
		}
		
		case SYS_CMD : {
			// Write config value
			if (frame_in->resp == 0 && compareUID(frame_in->data, uid) == 1 && frame_in->data[4] == SYS_STAT && frame_in->dlc == 8) {
				uint16_t value = ((uint16_t)frame_in->data[6] << 8) + (uint16_t)frame_in->data[7];
				
				switch (frame_in->data[5]) {
					case 1: // LED Brightness
					sendConfigConfirm(uid, hash, 1, updateLEDBrightness(value));
					break;
					case 2: // Number of inputs
					sendConfigConfirm(uid, hash, 2, updateNumInputs(value));
					break;
					case 3: // Number of sense ports
					sendConfigConfirm(uid, hash, 3, updateSensePorts(value));
					break;
					case 4: // Bus ID
					sendConfigConfirm(uid, hash, 4, updateBusID(value));
					break;
					case 5: // Start address
					sendConfigConfirm(uid, hash, 5, updateStartAdr(value));
					break;
					default:
					sendConfigConfirm(uid, hash, frame_in->data[5], 0);
					break;
					
				}
				
			// Send config value
			} else if (frame_in->resp == 0 && compareUID(frame_in->data, uid) == 1 && frame_in->data[4] == SYS_STAT && frame_in->dlc == 6) {
				
				switch (frame_in->data[5]) {
					case 1: // LED Brightness
					sendStatus(uid, hash, 1, t_led_brightness_set);
					break;
					case 2: // Number of inputs
					sendStatus(uid, hash, 2, num_inputs);
					break;
					case 3: // Number of sense ports
					sendStatus(uid, hash, 3, (uint8_t)pgm_read_byte(&trk_sense_reverse_table[trk_sense_ports]));
					break;
					case 4: // Bus ID
					sendStatus(uid, hash, 4, bus_id);
					break;
					case 5: // Start address
					sendStatus(uid, hash, 5, start_adr);
					break;
					default:
					break;
					
				}
			}
		}
		default : break;
	}
}

//
// Update logic states of track occupation.
//
void updateOccupationLogic(void) {
	for (uint8_t i = 0; i < 4; i++) {
		for (uint8_t j = 0; j < 8; j++) {
			
			// Get bit values for specific port
			uint8_t trk_bit_actual = (trk_state_actual[i] >> j) & 0b1;
			uint8_t trk_bit_logic = (trk_state_logic[i] >> j) & 0b1;
			uint8_t trk_bit_sense = (~trk_sense_flag >> ((i * 2) + (j / 4))) & 0b1;
			uint8_t trk_bit_sense_mask = (trk_sense_mask[i] >> j) & 0b1;
			
			
			if (trk_bit_actual < trk_bit_logic && (8 * i + j) < num_inputs) {
				// Don't do logic changes when input not used
				// Change logic state on occupation
				trk_state_logic[i] &= ~(1 << j);
				trk_state_last[i] |= (1 << j);
				sendS88Event(((uint32_t)bus_id << 16) + (8 * i + j) + start_adr, hash, (~trk_bit_logic & 0b1), (~trk_bit_actual & 0b1));
				} else if (trk_bit_actual == 0) {
				// Reset timeout when occupation returns
				trk_timeout_flag[i] &= ~(1 << j);
				trk_timeout[i][j] = 0;
				} else if (trk_bit_logic < trk_bit_actual && (trk_timeout[i][j] >= trk_timeout_millis || trk_bit_sense_mask == 1)) {
				// Wait for timeout before resetting occupation
				trk_state_logic[i] |= (1 << j);
				trk_state_last[i] &= ~(1 << j);
				sendS88Event(((uint32_t)bus_id << 16) + (8 * i + j) + start_adr, hash, (~trk_bit_logic & 0b1), (~trk_bit_actual & 0b1));
				trk_timeout_flag[i] &= ~(1 << j);
				trk_timeout[i][j] = 0;
				} else if (trk_bit_logic < trk_bit_actual && trk_timeout[i][j] == 0 && ((trk_bit_sense == 1) || (trk_bit_sense_mask == 1))) {
				// start timeout
				trk_timeout_flag[i] |= (1 << j);
				} else if (trk_bit_sense == 0 && trk_timeout[i][j] > 0) {
				// Reset timeout when no track power
				trk_timeout_flag[i] &= ~(1 << j);
				trk_timeout[i][j] = 0;
			}
			
			if (trk_bit_sense == 0 && (8 * i + j) < num_inputs) {
				// Blink LEDs if no track power sensed
				trk_LEDs[i] = (trk_LEDs[i] & ~(1 << j)) + ((~(~(trk_bit_logic & 0b1) & trk_blinker) & 0b1) << j);
			} else if (trk_LEDs[i] != trk_state_logic[i] && (8 * i + j) < num_inputs) {
				trk_LEDs[i] = (trk_LEDs[i] & ~(1 << j)) + (trk_bit_logic << j);
			} else if ((8 * i + j) >= num_inputs) {
				// turn on LEDs permanently when input unused
				trk_LEDs[i] = (trk_LEDs[i] & ~(1 << j));
			}
		}
	}
}

//
// main
//
int main(void)
{
	/************************************************************************/
	/*  INIT                                                                */
	/************************************************************************/
	
	// Read config values from EEPROM
	t_led_brightness_set = (uint8_t)eeprom_read_word((void *)EEPROM_LED_BRIGHTNESS);
	t_led_brightness = (uint8_t) pgm_read_byte (&ledLookupTable[t_led_brightness_set]);
	num_inputs = (uint8_t)eeprom_read_word((void *)EEPROM_NUM_INPUTS);
	trk_sense_ports = (uint8_t)eeprom_read_word((void *)EEPROM_NUM_SENSE);
	bus_id = eeprom_read_word((void*)EEPROM_BUS_ID);
	start_adr = eeprom_read_word((void *)EEPROM_START_ADR);
	
	
	initPins();
	
	// Get serial number and calculate UID and hash
	serial_nbr = (uint16_t)(boot_signature_byte_get(22) << 8) | boot_signature_byte_get(23);
	id = serial_nbr;
	uid = BASE_UID + serial_nbr;
	hash = generateHash(uid);
	
	cli();	
	
	// Setup heartbeat timer:
	TCCR0A |= (1 << WGM01);					// Timer 0 clear time on compare match
	OCR0A = (F_CPU / (64 * 1000UL)) - 1;	// Timer 0 compare value
	TIMSK0 |= (1 << OCIE0A);				// Timer interrupt
	TCCR0B |= ((1 << CS01)|(1 << CS00));	// Set timer 0 prescaler to 64
	
	// Setup dimm timer:
	TCCR1A |= (1 << WGM11);					// Timer 1 clear time on compare match
	OCR1A = (F_CPU / (1 * 128000UL)) - 1;	// Timer 1 compare value (128kHz)
	TIMSK1 |= (1 << OCIE1A);				// Timer interrupt
	TCCR1B |= (1 << CS10);					// No prescaler
	sei();
	
	// Init CAN bus
	mcp2515_init();
	
	/************************************************************************/
	/* MAIN LOOP                                                            */
	/************************************************************************/
	
    while (1) 
    {
		// Read dip switch states
		if (readPin(dip_switch[0]) == 0) reset_request = 1; // Bootloader mode
		
		if (readPin(dip_switch[1]) == 0) use_nl = 1;		// Send [NL] after every SLCAN frame on UART
		else use_nl = 0;
		
		if (readPin(dip_switch[2]) == 0) silent = 1;		// Silent mode
		else silent = 0;
		
		heartbeat();	
		
		// Reading actual Track Values:
		readTracks();
		trk_state_actual[0] = TRKA;
		trk_state_actual[1] = TRKB;
		trk_state_actual[2] = TRKC;
		trk_state_actual[3] = TRKD;
		
		
		// Update sense flags and masks
		applySenseFlagsMasks();
		
		// Update logic occupation states
		updateOccupationLogic();
		
		// update LED registers depending on logic states
		LEDA = ~trk_LEDs[0];
		LEDB = ~trk_LEDs[1];
		LEDC = ~trk_LEDs[2];
		LEDD = ~trk_LEDs[3];
		
		// Handle new CAN frame
		if (readCanFrame(&frame_in) == 1) handleCanFramePointer(&frame_in);

		// Software reset.
		if (reset_request == 1) {
			
			reset_request = 0;
			cli();
			resetLEDs();
			wdt_enable(WDTO_15MS);
			while(1);
		}
    }
}


/************************************************************************/
/*  INTERRUPT SERVICE ROUTINES                                          */
/************************************************************************/

ISR(TIMER0_COMPA_vect) {
	// Count up time counters.
	heartbeat_millis++;
	trk_blinker_millis++;
	
}

ISR(TIMER1_COMPA_vect) {
	// Duty cycle counter.
	
	if (duty_cycle_counter == 0xff) {
		duty_cycle_counter = 0;
	} else {
		++duty_cycle_counter;
	}
	if (duty_cycle_counter < t_led_brightness && new_duty_cycle == 1) {
		// turn on all t_leds specified in LEDx-Byte at the beginning of a duty cycle
		new_duty_cycle = 0;
		setLEDs();
	} else if (duty_cycle_counter >= t_led_brightness && new_duty_cycle == 0) {
		// turn off all t_leds at end of brightness-cycle
		new_duty_cycle = 1;
		resetLEDs();
	}
	
	if (duty_cycle_counter < s_led_brightness && s_led_state == 0) {
		setHigh(statusPin);
		s_led_state = 1;
	} else if (duty_cycle_counter >= s_led_brightness  && s_led_state == 1) {
		setLow(statusPin);
		s_led_state = 0;
	}
	
}