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
 * V 0.3
 * [2021-01-25.1]
 */

/*  SERIAL_INTERFACE
 *
 *	If "SERIAL_INTERFACE" is defined, the USB-Port creates a virtual COM-Port. 
 *	This interface transmits and receives CAN-Frames in the format specified
 *	by Märklin for it's Ethernet communication. There is no framing, so data 
 *	is just sent raw. This may cause data confusion on both ends and require
 *	a hardware reset to untangle it.
 *	The device handles serial frames the same way as CAN-Frames and sends it's
 *	responses on CAN and on Serial, while also converting the whole bus traffic.
 *	Other operating modes might be added in the future
 */

#define NAME "M\u00E4CAN 32-fach Gleisbelegtmelder"
#define BASE_UID 0x4D430000
#define ITEM "Dx32"
#define VERSION 0x0001
#define TYPE 0x0053

//#define TLEDBRIGHTNESS 63
#define BAUD_RATE 57200
#define SERIAL_TIMEOUT 10

#include <stdlib.h>
#include <avr/io.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "../Common/mcp2515_basic.h"
#include "../Common/mcan.h"
#include "../Common/uart.h"
#include "Dx32v1.0_Pindefs.h"

const uint8_t ledLookupTable[] PROGMEM = {0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,7,7,7,8,8,8,9,9,9,10,10,11,11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20,21,21,22,23,23,24,24,25,26,26,27,28,28,29,30,30,31,32,32,33,34,35,35,36,37,38,38,39,40,41,42,42,43,44,45,46,47,47,48,49,50,51,52,53,54,55,56,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,73,74,75,76,77,78,79,80,81,82,84,85,86,87,88,89,91,92,93,94,95,97,98,99,100,102,103,104,105,107,108,109,111,112,113,115,116,117,119,120,121,123,124,126,127,128,130,131,133,134,136,137,139,140,142,143,145,146,148,149,151,152,154,155,157,158,160,162,163,165,166,168,170,171,173,175,176,178,180,181,183,185,186,188,190,192,193,195,197,199,200,202,204,206,207,209,211,213,215,217,218,220,222,224,226,228,230,232,233,235,237,239,241,243,245,247,249,251,253,255};

uint32_t uid;
uint16_t hash;
uint16_t serial_nbr;
uint16_t id;

canFrame frame_in_can, frame_in_buffer;		// Buffers for incoming frames
canFrame frame_in_uart;
uint8_t new_frame = 0;					// New frame awaiting processing
uint8_t new_uart_frame = 0;

uint8_t reset_request;					// 1 if software reset is requested, else 0

uint32_t heartbeat_millis;				// Time counter for status LED
uint32_t last_heartbeat_millis;

uint8_t timeout_millis;
uint8_t frame_counter;
uint8_t byte_buffer;
uint8_t uart_to_can_buffer[13];

uint8_t t_led_brightness;
uint8_t t_led_brightness_set = 63;

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
uint8_t trk_sense_ports = 4; // 0,1,2,4,8				// number of sense ports
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
// Fun with Frames
//
void handleCanFrame(canFrame frame_in) {
	
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
			}
			break;
		}
		case CMD_S88_EVENT : {
			// S88 querry
			if (frame_in.resp == 0 && frame_in.dlc == 4) {
				uint32_t s88_id = ((uint32_t)frame_in.data[0] << 24) + ((uint32_t)frame_in.data[1] << 16) + ((uint32_t)frame_in.data[2] << 8) + ((uint32_t)frame_in.data[3]);
				if (s88_id == 0) {
					for (uint8_t i = 0; i < 32; i++) {
						sendS88Event(i + 1, hash, (trk_state_logic[i / 8] >> (i % 8)) & 0b1, (trk_state_last[i / 8] >> (i % 8)) & 0b1);
					}
				} else if (s88_id <= 32) {
					sendS88Event(s88_id, hash, (trk_state_logic[(s88_id - 1) / 8] >> ((s88_id - 1) % 8)) & 0b1, (trk_state_last[(s88_id - 1) / 8] >> ((s88_id - 1) % 8)) & 0b1);
				}
			}
			break;
		}
		case CMD_CONFIG : {
			// Config data
			if (frame_in.resp == 0 && compareUID(frame_in.data, uid) == 1) {
				switch (frame_in.data[4]) {
					case 0 : {
						sendDeviceInfo(uid, hash, id, 0, 1, ITEM, NAME);
						break;
					}
					case 1 : {
						sendConfigInfoSlider(uid, hash, 1, 0, 0xff, t_led_brightness_set, "Helligkeit_0_255");
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
				if (frame_in.dlc == 8) {
					if (frame_in.data[5] == 1) {
						t_led_brightness_set = frame_in.data[7];
						t_led_brightness = ledLookupTable[t_led_brightness_set];
					}
					sendConfigConfirm(uid, hash, frame_in.data[5]);
				} else if (frame_in.dlc == 6 && frame_in.data[5] == 1) {
					sendStatus(uid, hash, 1, t_led_brightness_set);
				}
			}
		}
		default : break;
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
	
	t_led_brightness = ledLookupTable[t_led_brightness_set];
	
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

#ifdef SERIAL_INTERFACE
	uart_init(UART_BAUD_SELECT(BAUD_RATE, F_CPU));
#endif 
		
	
	/************************************************************************/
	/* MAIN LOOP                                                            */
	/************************************************************************/
	
    while (1) 
    {
		
		heartbeat();	
		
		// Reading actual Track Values:
		readTracks();
		trk_state_actual[0] = TRKA;
		trk_state_actual[1] = TRKB;
		trk_state_actual[2] = TRKC;
		trk_state_actual[3] = TRKD;
		
		// Generate sense flags and masks
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
		
		// Update logic occupation states
		for (uint8_t i = 0; i < 4; i++) {
			for (uint8_t j = 0; j < 8; j++) {
				
				// Get bit values for specific port
				uint8_t trk_bit_actual = (trk_state_actual[i] >> j) & 0b1;
				uint8_t trk_bit_logic = (trk_state_logic[i] >> j) & 0b1;
				uint8_t trk_bit_sense = (~trk_sense_flag >> ((i * 2) + (j / 4))) & 0b1;
				uint8_t trk_bit_sense_mask = (trk_sense_mask[i] >> j) & 0b1;
				
				
				if (trk_bit_actual < trk_bit_logic) {
					// Change logic state on occupation
					trk_state_logic[i] &= ~(1 << j);
					trk_state_last[i] |= (1 << j);
					sendS88Event((8 * i + j) + 1, hash, (~trk_bit_logic & 0b1), (~trk_bit_actual & 0b1));
				} else if (trk_bit_actual == 0) {
					// Reset timeout when occupation returns
					trk_timeout_flag[i] &= ~(1 << j);
					trk_timeout[i][j] = 0;
				} else if (trk_bit_logic < trk_bit_actual && (trk_timeout[i][j] >= trk_timeout_millis || trk_bit_sense_mask == 1)) {
					// Wait for timeout before resetting occupation
					trk_state_logic[i] |= (1 << j);
					trk_state_last[i] &= ~(1 << j);
					sendS88Event((8 * i + j) + 1, hash, (~trk_bit_logic & 0b1), (~trk_bit_actual & 0b1));
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
				
				if (trk_bit_sense == 0) {
					// Blink LEDs if no track power sensed
					trk_LEDs[i] = (trk_LEDs[i] & ~(1 << j)) + ((~(~(trk_bit_logic & 0b1) & trk_blinker) & 0b1) << j);
				} else if (trk_LEDs[i] != trk_state_logic[i]){
					trk_LEDs[i] = (trk_LEDs[i] & ~(1 << j)) + (trk_bit_logic << j);
				}
			}
		}
		
		// update LEDs depending on logic states
		LEDA = ~trk_LEDs[0];
		LEDB = ~trk_LEDs[1];
		LEDC = ~trk_LEDs[2];
		LEDD = ~trk_LEDs[3];
		
		
		
		
		//
		// Handle new CAN frame */
		//
#ifdef SERIAL_INTERFACE
		// Read serial interface for new frame, send it on CAN and
		// convert to CAN-Frame format to handle in device
		uint16_t uart_in = uart_getc();
		
		if (uart_in >> 8 == 0 && timeout_millis <= SERIAL_TIMEOUT){
			// buffer 13 data bytes
			uart_to_can_buffer[frame_counter] = (uint8_t)uart_in;
			frame_counter++;
			timeout_millis = 0;
			
			if (frame_counter == 13) {
				// Convert byte buffer to can frame format when full
				//
				// 4 Bytes ID, 1 Byte DLC, 8 DATA-Bytes, 
				
				frame_counter = 0;
				canFrame uartToCanFrame;
				uartToCanFrame.cmd = (uart_to_can_buffer[0] << 7) + (uart_to_can_buffer[1] >> 1);
				uartToCanFrame.hash = (uart_to_can_buffer[2] << 8) + uart_to_can_buffer[3];
				uartToCanFrame.resp = uart_to_can_buffer[1] & 0b1;
				uartToCanFrame.dlc = uart_to_can_buffer[4];
				
				for(uint8_t i = 0; i < 8; i++) {
					uartToCanFrame.data[i] = uart_to_can_buffer[i + 5];
				}
				sendCanFrame(&uartToCanFrame);
				frame_in_uart = uartToCanFrame;
				new_uart_frame = 1;
			}
		} else if((uart_in >> 8) == 0 && (timeout_millis > 10)) {
			frame_counter = 0;
			timeout_millis = 0;	
		}	
#endif	
		
		if (new_frame == 1) {
			new_frame = 0;
			// Read new CAN-Frame from bus
			getCanFrame(&frame_in_can);
			
#ifdef SERIAL_INTERFACE
			// Send CAN-Frame to serial interfrace
			uart_putc(frame_in_can.cmd >> 7);
			uart_putc((frame_in_can.cmd << 1) + frame_in_can.resp);
			uart_putc(frame_in_can.hash >> 8);
			uart_putc(frame_in_can.hash);
			uart_putc(frame_in_can.dlc);
			
			for (uint8_t i = 0; i < 8; i++) {
				uart_putc(frame_in_can.data[i]);
			}
#endif
			handleCanFrame(frame_in_can);
			
		}
		//
		// Software reset.
		//
		if (reset_request == 1) {
			
			reset_request = 0;
			resetLEDs();
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

ISR(INT2_vect) {
	// MCP2515 interrupt on new CAN-frame.
	new_frame = 1;
}

ISR(TIMER0_COMPA_vect) {
	// Count up time counters.
	heartbeat_millis++;
	trk_blinker_millis++;
	if (frame_counter > 0) {
		timeout_millis++;
	}
	
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