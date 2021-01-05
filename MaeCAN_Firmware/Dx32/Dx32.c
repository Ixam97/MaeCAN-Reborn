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
 * V 0.1
 * [2021-01-05.1]
 */

#define NAME "M\u00E4CAN 32-fach Gleisbelegtmelder"
#define BASE_UID 0x4D430000
#define ITEM "Dx32"
#define VERSION 0x0001
#define TYPE 0x0053

#define TLEDBRIGHTNESS 63

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <string.h>

#include "../Common/mcp2515_basic.h"
#include "../Common/mcan.h"
#include "Dx32v1.0_Pindefs.h"

const uint8_t ledLookupTable[] = {0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,7,7,7,8,8,8,9,9,9,10,10,11,11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20,21,21,22,23,23,24,24,25,26,26,27,28,28,29,30,30,31,32,32,33,34,35,35,36,37,38,38,39,40,41,42,42,43,44,45,46,47,47,48,49,50,51,52,53,54,55,56,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,73,74,75,76,77,78,79,80,81,82,84,85,86,87,88,89,91,92,93,94,95,97,98,99,100,102,103,104,105,107,108,109,111,112,113,115,116,117,119,120,121,123,124,126,127,128,130,131,133,134,136,137,139,140,142,143,145,146,148,149,151,152,154,155,157,158,160,162,163,165,166,168,170,171,173,175,176,178,180,181,183,185,186,188,190,192,193,195,197,199,200,202,204,206,207,209,211,213,215,217,218,220,222,224,226,228,230,232,233,235,237,239,241,243,245,247,249,251,253,255};

uint32_t uid;
uint16_t hash;
uint16_t serial_nbr;
uint16_t id;

canFrame frame_in, frame_in_buffer;		// Buffers for incoming frames
uint8_t new_frame = 0;					// New frame awaiting processing

uint8_t reset_request;					// 1 if software reset is requested, else 0

uint32_t heartbeat_millis;				// Time counter for status LED
uint32_t test_millis;					// Running Lights Test
uint8_t test_index;

uint8_t t_led_state[32];
uint8_t t_led_brightness;
uint8_t t_led_speed = 100;

uint8_t s_led_state;
uint8_t s_led_brightness = 0;
uint8_t s_led_brightness_set;
uint8_t s_led_rising = 1;

uint8_t duty_cycle_counter;
uint8_t last_duty_cycle_counter;
uint8_t new_duty_cycle;


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
	
	// Running light test
	if (test_millis >= t_led_speed && t_led_speed > 0) {
		test_millis = 0;
		if (test_index > 31) {
			test_index = 0;
			for (uint8_t i = 0; i < 32; i++) {
				t_led_state[i] = 0;
			}
		} else {
			t_led_state[test_index] = 1;
			test_index++;
		}
	} else if (t_led_speed == 0) {
		for (uint8_t i = 0; i < 32; i++) {
			t_led_state[i] = 1;
		}
	}
	
	if (heartbeat_millis >= 1200) {
		//setLow(statusPin);
		//s_led_brightness = 0;
		s_led_rising = 0;
		heartbeat_millis = 0;
	} else if (heartbeat_millis >= 1000) {
		//setHigh(statusPin);
		//s_led_brightness = 0xff;
		s_led_rising = 1;
	} else if (heartbeat_millis >= 800) {
		//setLow(statusPin);
		//s_led_brightness = 0;
		s_led_rising = 0;
	} else if (heartbeat_millis >= 600) {
		//setHigh(statusPin);
		//s_led_brightness = 0xff;
		s_led_rising = 1;
	}	
}

//
// LED brightness handling.
//
void dutyCycler() {
	
	if (last_duty_cycle_counter != duty_cycle_counter) {
		last_duty_cycle_counter = duty_cycle_counter;
		
		if (duty_cycle_counter < s_led_brightness && s_led_state == 0) {
			setHigh(statusPin);
			s_led_state = 1;
			} else if (duty_cycle_counter >= s_led_brightness  && s_led_state == 1) {
			setLow(statusPin);
			s_led_state = 0;
		}
		
		if (duty_cycle_counter < t_led_brightness && new_duty_cycle == 1) {
			new_duty_cycle = 0;
			for (uint8_t i = 0; i < 32; i++) {
				if (t_led_state[i] == 1) {
					setLow(t_led[i]);
				}
			}
			} else if (duty_cycle_counter >= t_led_brightness && new_duty_cycle == 0) {
			new_duty_cycle = 1;
			for (uint8_t i = 0; i < 32; i++) {
				setHigh(t_led[i]);
			}
		}
	}
}

//
// Debouncing inputs
//
void debounce() {

}

//
// main
//
int main(void)
{
	/************************************************************************/
	/*  INIT                                                                */
	/************************************************************************/
	
	t_led_brightness = ledLookupTable[TLEDBRIGHTNESS];
	
	initPins();
	
	// Get serial number and calculate UID and hash
	serial_nbr = (uint16_t)(boot_signature_byte_get(22) << 8) | boot_signature_byte_get(23);
	id = ~(0xfff0 + (readPin(dip_switch[0]) << 3) + (readPin(dip_switch[1]) << 2) + (readPin(dip_switch[2]) << 1) + readPin(dip_switch[3]));
	if (id == 0) {
		id = serial_nbr;
	}
	uid = BASE_UID + serial_nbr;
	hash = generateHash(uid);
	
	// Setup ext. interrupt on falling edge.
	cli();
	EICRA |= (1 << ISC21);
	EICRA &= ~(1 << ISC20);
	EIMSK |= (1 << INT2);
	setInput(intPin);
	setHigh(intPin);
	
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
	
	// Init CAN bus
	mcp2515_init();
		
	
	/************************************************************************/
	/* MAIN LOOP                                                            */
	/************************************************************************/
	
    while (1) 
    {
		
		heartbeat();
		/* Handle new CAN frame */
		if (new_frame == 1) {
			new_frame = 0;
			getCanFrame(&frame_in);
			
			switch (frame_in.cmd) {
				
				case 0x40 : { 
					// Bootloader request
					// THIS SECTION IS REQUIRED FOR THE BOOTLOADER TO WORK DURING NORMAL OPARATION
					
					if (frame_in.resp == 0 && frame_in.dlc == 4) {
						if (compareUID(frame_in.data, uid) == 1) {
							
							// Write bootloader request flag to EEPROM and reset.
							eeprom_update_byte((void *)1023, 1);
							_delay_ms(100);
							reset_request = 1;
						}
					}
					break;
				}
				case CMD_PING : { 
					// Ping
					if (frame_in.resp == 0) {
						sendPingFrame(uid, hash, VERSION, TYPE);
						break;
					}
				}
				case CMD_CONFIG : { 
					// Config data
					if (frame_in.resp == 0 && compareUID(frame_in.data, uid) == 1) {
						switch (frame_in.data[4]) {
							case 0 : {
								sendDeviceInfo(uid, hash, id, 0, 2, ITEM, NAME);
								break;
							}
							case 1 : {
								sendConfigInfoSlider(uid, hash, 1, 0, 0xff, t_led_brightness, "Helligkeit_0_255");
							}
							case 2 : {
								sendConfigInfoSlider(uid, hash, 2, 0, 0xff, t_led_speed, "Geschwindigkeit_0_255");
							}
							default : {
								break;
							}
						}
					}
					break;
				}
				
				case SYS_CMD : {
					// Write config data
					if (frame_in.resp == 0 && compareUID(frame_in.data, uid) == 1 && frame_in.data[4] == SYS_STAT) {
						if (frame_in.data[5] == 1) {
							t_led_brightness = ledLookupTable[frame_in.data[7]];
						} else if (frame_in.data[5] == 2) {
							t_led_speed = frame_in.data[7] ^ 0xff;
						}
						sendConfigConfirm(uid, hash, frame_in.data[5]);
					}
				}
				default : break;
			}
		}
		
		//
		// Software reset.
		//
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
	// MCP2515 interrupt on new CAN-frame.
	new_frame = 1;
}

ISR(TIMER0_COMPA_vect) {
	// Count up time counters.
	heartbeat_millis++;
	test_millis++;
	
	// Adjust staus LED fade brightness.
	if (s_led_rising == 1 && s_led_brightness_set < 0xff - 3) {
		s_led_brightness_set += 3;
		s_led_brightness = ledLookupTable[s_led_brightness_set];
	} else if (s_led_rising == 0 && s_led_brightness_set > 3) {
		s_led_brightness_set -= 3;
		s_led_brightness = ledLookupTable[s_led_brightness_set];
	}
	
}

ISR(TIMER1_COMPA_vect) {
	// Duty cycle counter.
	if (duty_cycle_counter == 0xff) {
		duty_cycle_counter = 0;
	} else {
		++duty_cycle_counter;
	}
}