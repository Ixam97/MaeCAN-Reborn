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


#define NAME "M\u00E4CAN MP5x8+S"
#define BASE_UID 0x4D430000
#define ITEM "MP5x8+S"
#define VERSION 0x0001
#define TYPE 0x0052

#define MP5_COUNT 8

#define EEPROM_STATE_OFFSET 1024
#define EEPROM_BOOTLOADER_BYTE 1023

#define CHANNEL_OFFSET  3 + (MP5_COUNT * 2)
#define FEEDBACK_ENABLED eeprom_read_word((void *)6)
#define TRACK_PROTOCOL eeprom_read_word((void *)2)

// CAN-Befehle:

#define CMD_HALT		0x0
#define CMD_HP1			0x1
#define CMD_HP2			0x2
#define CMD_SH1			0x3
#define CMD_HPS_1_ZS	0x4
#define CMD_LS_KE		0x5
#define CMD_ZS1			0x6
#define CMD_HP_KE		0x7
#define CMD_ZS8			0x8
#define CMD_HPS_2_ZS	0x9
#define CMD_HPS_3_ZS	0xa
#define CMD_HPS_4_ZS	0xb

#define CMD_VR0			0x0
#define CMD_VR1			0x1
#define CMD_VR2			0x2
#define CMD_VRS_1_ZS	0x4
#define CMD_VRS_2_ZS	0x9
#define CMD_VRS_3_ZS	0xa
#define CMD_VRS_4_ZS	0xb
#define CMD_VR0_ZS		0xc
#define CMD_VR_KE		0xd

#define CMD_DONT_CARE	0xff

/*Signalbilder*/
#define HP0			 0

#define HP1			 1
#define HP1_ZS3		 2
#define HP1_ZSX		 3
#define HP1_ZS3_ZSX	 4
#define HP2			 5
#define HP2_ZS3		 6
#define HP2_ZSX		 7
#define HP2_ZS3_ZSX  8

#define VR0			 1
#define VR1			 2
#define VR2			 3
#define VR1_ZS3V	 4
#define VR2_ZS3V	 5
#define VR0_ZS3V	 6
#define VR_DARK		 7

#define KS1					   1
#define KS1_ZS3				   2
#define KS1_ZS3V			   3
#define KS1_ZS3_ZS3V		   4
#define KS1_ZS3V_BLINK		   5
#define KS1_ZS3_ZS3V_BLINK	   6
#define KS2					   7
#define KS2_ZS3				   8
#define KS2_ZS3V			   9
#define KS2_ZS3_ZS3V		  10
#define KS1_ZS3V_BLINK_ZL	  11
#define KS1_ZS3_ZS3V_BLINK_ZL 12
#define KS2_ZL				  13
#define KS2_ZS3_ZL			  14
#define KS2_ZS3V_ZL			  15
#define KS2_ZS3_ZS3V_ZL		  16

#define SH1		    17
#define ZS1			18
#define KENN		19
#define ZS8			20

#define SIG_INIT	21

/*Signale Pindeffinitionen*/
#define HV_GREEN	0
#define HV_RED1		1
#define HV_YELLOW	2
#define HV_RED2		3
#define HV_SH1		4
#define HV_KENN		5
#define HV_ZS3		6
#define HV_ZSX		7
#define HV_ZS1		8

#define VR_GREEN1	0
#define VR_GREEN2	1
#define VR_YELLOW1	2
#define VR_YELLOW2	3
#define VR_ZS3V		4
#define VR_ZL		5

/*Signale Sonstiges*/
#define SIG_FADE_ON		  1.0
#define SIG_FADE_OFF	  1.0
#define SIG_BIRGHTNESS	255
//#define SIG_NC			 32
#define SIG_NC			 16
#define SIG_NUM			  3

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <string.h>

#include "../Common/mcan.h"
#include "MP5x8+S_Pindefs.h"

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

uint16_t signal_words[5] = {13712, 11560, 14535, 64, 4096}; // Signal-Konfigparameter (siehe Excel-Tabelle)
	
uint16_t signalWordsDecode(uint16_t _signal_words[5], uint8_t param) {
	switch (param) {
		case 0 : return (_signal_words[0] >> 10) & 0b11111; break;
		case 1 : return (_signal_words[0] >> 5) & 0b11111; break;
		case 2 : return (_signal_words[0] >> 0) & 0b11111; break;
		case 3 : return (_signal_words[1] >> 10) & 0b11111; break;
		case 4 : return (_signal_words[1] >> 5) & 0b11111; break;
		case 5 : return (_signal_words[1] >> 0) & 0b11111; break;
		case 6 : return (_signal_words[2] >> 10) & 0b11111; break;
		case 7 : return (_signal_words[2] >> 5) & 0b11111; break;
		case 8 : return (_signal_words[2] >> 0) & 0b11111; break;
		case 9 : return (_signal_words[3] >> 6) & 0b1; break;
		case 10 : return (_signal_words[3] >> 3) & 0b111; break;
		case 11 : return (_signal_words[3] >> 2) & 0b1; break;
		case 12 : return (_signal_words[3] >> 0) & 0b11; break;
		case 13 : return (_signal_words[0] >> 12) & 0b1111; break;
		case 14 : return (_signal_words[0] >> 11) & 0b1; break;
		case 15 : return (_signal_words[0] >> 0) & 0b11111111111; break;
		default : return 0xffff; break;
	}
	return 0xffff;
}

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

/* Lichtsignalsteuerung */

// Struct f�r H/V-Hauptsignale
typedef struct {
	uint8_t outputs[9];
	uint8_t aspect_is;
	uint8_t aspect_wanted;
	uint8_t aspect_wanted_lock;
	uint8_t has_vsig;
	uint64_t millis;
	
	uint8_t hps_1, hps_2, hps_3, hps_4; // Sonderbefehle gem�� der ESTWGJ-Nomenklatur. Kann im Signalsetup frei einem Signalbild zugewiesen werden.
	uint8_t current_cmd;
	
} HV_HSig;

// Struct f�r H/V-Vorsignale
typedef struct {
	uint8_t outputs[6];
	uint8_t aspect_is;
	uint8_t aspect_wanted;
	uint8_t aspect_wanted_lock;
	HV_HSig *parent_hsig;
	uint8_t *parent_hsig_aspect_is;
	uint8_t is_on_hsig;
	uint8_t reduced_dist;
	uint64_t millis;
	
	uint8_t vrs_1, vrs_2, vrs_3, vrs_4; // Sonderbefehle gem�� der ESTWGJ-Nomenklatur. Kann im Signalsetup frei einem Signalbild zugewiesen werden.
	uint8_t current_cmd;
	
} HV_VSig;

void HV_HSigCmd(uint8_t command, HV_HSig *sig) {
	switch (command) {
		case CMD_HALT:
			sig->aspect_wanted = HP0;
			sig->current_cmd = CMD_HALT;
			break;
		case CMD_HP1:
			sig->aspect_wanted = HP1;
			sig->current_cmd = CMD_HP1;
			break;
		case CMD_HP2:
			sig->aspect_wanted = HP2;
			sig->current_cmd = CMD_HP2;
			break;
		case CMD_SH1:
			sig->aspect_wanted = SH1;
			sig->current_cmd = CMD_SH1;
			break;
		case CMD_HP_KE:
			sig->aspect_wanted = KENN;
			sig->current_cmd = CMD_HP_KE;
			break;
		case CMD_LS_KE:
			sig->aspect_wanted = KENN;
			sig->current_cmd = CMD_LS_KE;
			break;
		case CMD_ZS1:
			sig->aspect_wanted = ZS1;
			sig->current_cmd = CMD_ZS1;
			break;
		case CMD_ZS8:
			sig->aspect_wanted = ZS8;
			sig->current_cmd = CMD_ZS8;
			break;
		case CMD_HPS_1_ZS:
			sig->aspect_wanted = sig->hps_1;
			sig->current_cmd = CMD_HPS_1_ZS;
			break;
		case CMD_HPS_2_ZS:
			sig->aspect_wanted = sig->hps_2;
			sig->current_cmd = CMD_HPS_2_ZS;
			break;
		case CMD_HPS_3_ZS:
			sig->aspect_wanted = sig->hps_3;
			sig->current_cmd = CMD_HPS_3_ZS;
			break;
		case CMD_HPS_4_ZS:
			sig->aspect_wanted = sig->hps_4;
			sig->current_cmd = CMD_HPS_4_ZS;
			break;
		case CMD_DONT_CARE:
			break;
		default:
			sig->aspect_wanted = HP0;
			sig->current_cmd = CMD_HALT;
			break;
	}
}
void HV_VSigCmd(uint8_t command, HV_VSig *sig) {
	switch (command) {
		case CMD_VR0:		
			sig->aspect_wanted = VR0;
			sig->current_cmd = CMD_VR0;
			break;
		case CMD_VR1:		
			sig->aspect_wanted = VR1;
			sig->current_cmd = CMD_VR1;
			break;
		case CMD_VR2:		
			sig->aspect_wanted = VR2;
			sig->current_cmd = CMD_VR2;
			break;
		case CMD_VR_KE:		
			sig->aspect_wanted = KENN;
			sig->current_cmd = CMD_VR_KE;
			break;
		case CMD_VRS_1_ZS:
			sig->aspect_wanted = sig->vrs_1;
			sig->current_cmd = CMD_VRS_1_ZS;
			break;
		case CMD_VRS_2_ZS:	
			sig->aspect_wanted = sig->vrs_2;
			sig->current_cmd = CMD_VRS_2_ZS;
			break;
		case CMD_VRS_3_ZS:	
			sig->aspect_wanted = sig->vrs_3;
			sig->current_cmd = CMD_VRS_3_ZS;
			break;
		case CMD_VRS_4_ZS:	
			sig->aspect_wanted = sig->vrs_4;
			sig->current_cmd = CMD_VRS_4_ZS;
			break;
		case CMD_VR0_ZS:	
			sig->aspect_wanted = VR0_ZS3V;
			sig->current_cmd = CMD_VR0_ZS;
			break;
		case CMD_DONT_CARE:
			break;
		default: 
			sig->aspect_wanted = VR0;
			sig->current_cmd = CMD_VR0;
			break;
	}
}

HV_HSig setup_HV_HSig(uint8_t green, uint8_t red1, uint8_t red2, uint8_t yellow, uint8_t sh1, uint8_t kenn, uint8_t zs3, uint8_t zsx, uint8_t zs1, uint8_t has_vsig) {
	HV_HSig _sig;
	_sig.outputs[HV_GREEN] = green;
	_sig.outputs[HV_RED1] = red1;
	_sig.outputs[HV_YELLOW] = yellow;
	_sig.outputs[HV_RED2] = red2;
	_sig.outputs[HV_SH1] = sh1;
	_sig.outputs[HV_KENN] = kenn;
	_sig.outputs[HV_ZS3] = zs3;
	_sig.outputs[HV_ZSX] = zsx;
	_sig.outputs[HV_ZS1] = zs1;
	_sig.has_vsig = has_vsig;
	_sig.aspect_is = SIG_INIT;
	_sig.aspect_wanted = HP0;
	_sig.aspect_wanted_lock = HP0;
	_sig.millis = 0;
	_sig.current_cmd = CMD_HALT;
	
	return _sig;
}

HV_VSig setup_HV_VSig(uint8_t green1, uint8_t green2, uint8_t yellow1, uint8_t yellow2, uint8_t zs3v, uint8_t zl, uint8_t is_on_hsig, uint8_t reduced_dist, HV_HSig *parent_hsig) {
	HV_VSig _sig;
	_sig.outputs[VR_GREEN1] = green1;
	_sig.outputs[VR_GREEN2] = green2;
	_sig.outputs[VR_YELLOW1] = yellow1;
	_sig.outputs[VR_YELLOW2] = yellow2;
	_sig.outputs[VR_ZS3V] = zs3v;
	_sig.outputs[VR_ZL] = zl;
	_sig.is_on_hsig = is_on_hsig;
	_sig.aspect_is = SIG_INIT;
	_sig.parent_hsig = parent_hsig;
	_sig.aspect_wanted = VR0;
	_sig.aspect_wanted_lock = _sig.aspect_wanted;
	_sig.reduced_dist = reduced_dist;
	_sig.millis = 0;
	_sig.current_cmd = CMD_VR0;
	
	return _sig;
}

void HV_HSig_makro(HV_HSig * sig) {
	// Steuert die Schaltabfolge beim Bildwechsel von H/V-Hauptsignalen
	
	if (sig->aspect_is != sig->aspect_wanted || sig->millis > 0) {
		if (sig->millis == 0){
			sig->aspect_wanted_lock = sig->aspect_wanted;
			sig->millis = millis_counter;
		}
		int32_t current_millis = (uint16_t)(millis_counter - sig->millis);
		
		if (sig->aspect_is != HP0 || sig->aspect_wanted_lock == HP0) {
			
			if (current_millis > 1400) {
				sig->aspect_is = HP0;
				sig->millis = 0;
				} else if (current_millis > 500) {
				if (sig->outputs[HV_RED1] != SIG_NC) switchOutput(sig->outputs[HV_RED1], SIG_BIRGHTNESS, SIG_FADE_ON);
				} else if (current_millis > 400) {
				if (sig->outputs[HV_RED2] != SIG_NC) switchOutput(sig->outputs[HV_RED2], SIG_BIRGHTNESS, SIG_FADE_ON);
				} else if (current_millis > 100) {
				if(sig->outputs[HV_YELLOW] != SIG_NC) switchOutput(sig->outputs[HV_YELLOW], 0, SIG_FADE_OFF);
				if(sig->outputs[HV_ZS3] != SIG_NC) switchOutput(sig->outputs[HV_ZS3], 0, SIG_FADE_OFF/2);
				if(sig->outputs[HV_ZS1] != SIG_NC) switchOutput(sig->outputs[HV_ZS1], 0, SIG_FADE_OFF/2);
				if(sig->outputs[HV_ZSX] != SIG_NC) switchOutput(sig->outputs[HV_ZSX], 0, SIG_FADE_OFF/2);
				if(sig->outputs[HV_KENN] != SIG_NC) switchOutput(sig->outputs[HV_KENN], 0, SIG_FADE_OFF/2);
				if(sig->outputs[HV_SH1] != SIG_NC) switchOutput(sig->outputs[HV_SH1], 0, SIG_FADE_OFF/2);
				} else if (current_millis > 0) {
				if(sig->outputs[HV_GREEN] != SIG_NC) switchOutput(sig->outputs[HV_GREEN], 0, SIG_FADE_OFF);
			}
			
			} else {
			
			if (sig->has_vsig == 1 && sig->aspect_wanted != SH1 && sig->aspect_wanted != ZS1 && sig->aspect_wanted != ZS8) current_millis -= 1000; // Zus�tzliche Verz�gerung bei Vorsignal am Mast
			
			if (current_millis > 1400) {
				sig->aspect_is = sig->aspect_wanted_lock;
				if (sig->aspect_is == ZS8) blink_clock = 0;
				sig->millis = 0;
				} else if (current_millis > 500) {
				if (sig->outputs[HV_KENN] != SIG_NC && sig->aspect_wanted_lock == KENN)
				switchOutput(sig->outputs[HV_KENN], SIG_BIRGHTNESS/4, SIG_FADE_ON/2);
				if (sig->outputs[HV_SH1] != SIG_NC && sig->aspect_wanted_lock == SH1)
				switchOutput(sig->outputs[HV_SH1], SIG_BIRGHTNESS/4, SIG_FADE_ON/2);
				if (sig->outputs[HV_ZS1] != SIG_NC && sig->aspect_wanted_lock == ZS1)
				switchOutput(sig->outputs[HV_ZS1], SIG_BIRGHTNESS/4, SIG_FADE_ON/2);
				if (sig->outputs[HV_GREEN]!=SIG_NC && sig->aspect_wanted_lock != KENN && sig->aspect_wanted_lock != SH1 && sig->aspect_wanted_lock != ZS1 && sig->aspect_wanted_lock != ZS8)
				switchOutput(sig->outputs[HV_GREEN], SIG_BIRGHTNESS, SIG_FADE_ON);
				} else if (current_millis > 400) {
				if (sig->outputs[HV_YELLOW]!=SIG_NC && (sig->aspect_wanted_lock == HP2 || sig->aspect_wanted_lock == HP2_ZS3 || sig->aspect_wanted_lock == HP2_ZS3_ZSX || sig->aspect_wanted_lock == HP2_ZSX))
				switchOutput(sig->outputs[HV_YELLOW], SIG_BIRGHTNESS, SIG_FADE_ON);
				if (sig->outputs[HV_ZS3] != SIG_NC && (sig->aspect_wanted_lock == HP1_ZS3 || sig->aspect_wanted_lock == HP1_ZS3_ZSX || sig->aspect_wanted_lock == HP2_ZS3 || sig->aspect_wanted_lock == HP2_ZS3_ZSX))
				switchOutput(sig->outputs[HV_ZS3], SIG_BIRGHTNESS/4, SIG_FADE_ON/2);
				if (sig->outputs[HV_ZSX] != SIG_NC && (sig->aspect_wanted_lock == HP1_ZSX || sig->aspect_wanted_lock == HP1_ZS3_ZSX || sig->aspect_wanted_lock == HP2_ZSX || sig->aspect_wanted_lock == HP2_ZS3_ZSX))
				switchOutput(sig->outputs[HV_ZSX], SIG_BIRGHTNESS/4, SIG_FADE_ON/2);
				} else if (current_millis > 100) {
				if (sig->outputs[HV_RED1]!=SIG_NC && sig->aspect_wanted_lock != SH1 && sig->aspect_wanted_lock != ZS1 && sig->aspect_wanted_lock != ZS8)
				switchOutput(sig->outputs[HV_RED1], 0, SIG_FADE_OFF);
				} else if (current_millis > 0) {
				if (sig->outputs[HV_RED2]!=SIG_NC && sig->aspect_wanted_lock != ZS1 && sig->aspect_wanted_lock != ZS8)
				switchOutput(sig->outputs[HV_RED2], 0, SIG_FADE_OFF);
				if (sig->aspect_wanted_lock == ZS1 || sig->aspect_wanted_lock == ZS8) sig->millis -= 1000;
			}
		}
	}
	
	if (sig->aspect_wanted == ZS8 && sig->aspect_is == ZS8) {
		// Blinkendes ZS1 f�r das Gegengleis-Ersatzsignal
		if (blink_clock < 450) {
			if (sig->outputs[HV_ZS1] != SIG_NC) switchOutput(sig->outputs[HV_ZS1], SIG_BIRGHTNESS/4, SIG_FADE_ON/4);
		} else {
			if (sig->outputs[HV_ZS1] != SIG_NC) switchOutput(sig->outputs[HV_ZS1], 0, SIG_FADE_OFF/4);
		}
	}
}

void HV_VSig_makro(HV_VSig *sig) {
	// Steuert die Schaltabfolge beim Bildwechsel von H/V-Vorsignalen
	
	// Dunkeltasten, wenn kein Fahrtbegriff
	if(sig->is_on_hsig == 1) {
		if ((sig->parent_hsig->aspect_wanted == HP0 /*|| sig->parent_hsig->aspect_wanted == KENN*/ || sig->parent_hsig->aspect_wanted == ZS1 || sig->parent_hsig->aspect_wanted == ZS8 || sig->parent_hsig->aspect_wanted == SH1) && sig->aspect_wanted_lock != VR_DARK) {
			sig->millis = 0;
			sig->aspect_wanted_lock = VR_DARK;
			} else if (sig->aspect_wanted_lock == VR_DARK && sig->aspect_wanted != VR_DARK && sig->parent_hsig->aspect_wanted != HP0 /*&& sig->parent_hsig->aspect_wanted != KENN*/ && sig->parent_hsig->aspect_wanted != ZS8 && sig->parent_hsig->aspect_wanted != ZS1 && sig->parent_hsig->aspect_wanted != SH1) {
			sig->aspect_wanted_lock = sig->aspect_wanted;
			sig->aspect_is = SIG_INIT;
		}
	}
	
	if ((sig->aspect_is != VR_DARK || sig->millis > 0) && (sig->aspect_wanted_lock == VR_DARK || sig->aspect_wanted == VR_DARK)) {
		if (sig->millis == 0){
			sig->millis = millis_counter;
		}
		int16_t current_millis = (uint16_t)(millis_counter - sig->millis);
		
		if (current_millis > 1400) {
			sig->aspect_is = VR_DARK;
			sig->millis = 0;
			} else if (current_millis > 1300) {
			if(sig->outputs[VR_YELLOW1] != SIG_NC) switchOutput(sig->outputs[VR_YELLOW1], 0, SIG_FADE_OFF);
			if(sig->outputs[VR_ZL] != SIG_NC) switchOutput(sig->outputs[VR_ZL], 0, SIG_FADE_OFF/2);
			} else if (current_millis > 1200) {
			if(sig->outputs[VR_YELLOW2] != SIG_NC) switchOutput(sig->outputs[VR_YELLOW2], 0, SIG_FADE_OFF);
			} else if (current_millis > 600) {
			if(sig->outputs[VR_YELLOW1] != SIG_NC) switchOutput(sig->outputs[VR_YELLOW1], SIG_BIRGHTNESS, SIG_FADE_ON);
			} else if (current_millis > 500) {
			if(sig->outputs[VR_YELLOW2] != SIG_NC) switchOutput(sig->outputs[VR_YELLOW2], SIG_BIRGHTNESS, SIG_FADE_ON);
			} else if (current_millis > 400) {
			if(sig->outputs[VR_GREEN1] != SIG_NC) switchOutput(sig->outputs[VR_GREEN1], 0, SIG_FADE_OFF);
			} else if (current_millis > 350) {
			if(sig->outputs[VR_ZS3V] != SIG_NC) switchOutput(sig->outputs[VR_ZS3V], 0, SIG_FADE_OFF);
			} else if (current_millis > 300) {
			if(sig->outputs[VR_GREEN2] != SIG_NC) switchOutput(sig->outputs[VR_GREEN2], 0, SIG_FADE_OFF);
		}
		
		} else if (sig->aspect_wanted_lock != VR_DARK && sig->aspect_wanted != VR_DARK) {
		if (sig->aspect_is != sig->aspect_wanted || sig->millis > 0) {
			if (sig->millis == 0){
				sig->aspect_wanted_lock = sig->aspect_wanted;
				sig->millis = millis_counter;
			}
			int16_t current_millis = (uint16_t)(millis_counter - sig->millis);
			
			//if(((sig->aspect_is != VR0 && !(sig->aspect_is == SIG_INIT && sig->aspect_wanted != VR0)) || sig->aspect_wanted_lock == VR0) || ((sig->aspect_is != VR0_ZL && !(sig->aspect_is == SIG_INIT && sig->aspect_wanted != VR0_ZL)) || sig->aspect_wanted_lock == VR0_ZL)) {
			if ((sig->aspect_is != VR0 /*&& sig->aspect_is != VR0_ZS3V && sig->aspect_is != VR0_ZS3V_ZL*/ && !(sig->aspect_is == SIG_INIT && sig->aspect_wanted != VR0 /*&& sig->aspect_wanted != VR0_ZS3V */)) || sig->aspect_wanted_lock == VR0 || sig->aspect_wanted_lock == VR0_ZS3V) {
				if (current_millis > 1400) {
					if (sig->aspect_wanted_lock == VR0_ZS3V) sig->aspect_is = VR0_ZS3V;
					else sig->aspect_is = VR0;
					sig->millis = 0;
				} else if (current_millis > 500) {
					if (sig->outputs[VR_YELLOW1] != SIG_NC) switchOutput(sig->outputs[VR_YELLOW1], SIG_BIRGHTNESS, SIG_FADE_ON);
				} else if (current_millis > 400) {
					if (sig->outputs[VR_YELLOW2] != SIG_NC) switchOutput(sig->outputs[VR_YELLOW2], SIG_BIRGHTNESS, SIG_FADE_ON);
				} else if (current_millis > 100) {
					if (sig->outputs[VR_GREEN1] != SIG_NC) switchOutput(sig->outputs[VR_GREEN1], 0, SIG_FADE_OFF);
				} else if (current_millis > 50) {
					if (sig->outputs[VR_ZS3V] != SIG_NC) switchOutput(sig->outputs[VR_ZS3V], 0, SIG_FADE_OFF);
					if (sig->outputs[VR_ZS3V] != SIG_NC && sig->aspect_wanted_lock == VR0_ZS3V) switchOutput(sig->outputs[VR_ZS3V], SIG_BIRGHTNESS, SIG_FADE_ON);
					if (sig->outputs[VR_ZL] != SIG_NC) switchOutput(sig->outputs[VR_ZL], 0, SIG_FADE_OFF/2);
					if (sig->outputs[VR_ZL] != SIG_NC && sig->reduced_dist == 1) switchOutput(sig->outputs[VR_ZL], SIG_BIRGHTNESS/4, SIG_FADE_ON/2);
				} else if (current_millis > 0) {
					if (sig->outputs[VR_GREEN2] != SIG_NC) switchOutput(sig->outputs[VR_GREEN2], 0, SIG_FADE_OFF);
				}
			} else {
				if (current_millis > 1400) {
					sig->aspect_is = sig->aspect_wanted_lock;
					sig->millis = 0;
				} else if (current_millis > 500) {
					if (sig->outputs[VR_GREEN1] != SIG_NC && (sig->aspect_wanted_lock == VR1 || sig->aspect_wanted_lock == VR1_ZS3V))
						switchOutput(sig->outputs[VR_GREEN1], SIG_BIRGHTNESS, SIG_FADE_ON);
				} else if (current_millis > 400) {
					if (sig->outputs[VR_GREEN2] != SIG_NC && sig->aspect_wanted_lock != KENN)
						switchOutput(sig->outputs[VR_GREEN2], SIG_BIRGHTNESS, SIG_FADE_ON);
				} else if (current_millis > 300) {
					if (sig->outputs[VR_ZS3V] != SIG_NC && (sig->aspect_wanted_lock == VR1_ZS3V || sig->aspect_wanted_lock == VR2_ZS3V))
						switchOutput(sig->outputs[VR_ZS3V], SIG_BIRGHTNESS, SIG_FADE_ON);
					else if (sig->outputs[VR_ZS3V] != SIG_NC)
						switchOutput(sig->outputs[VR_ZS3V], 0, SIG_FADE_OFF);
					if (sig->outputs[VR_ZL] != SIG_NC) 
						switchOutput(sig->outputs[VR_ZL], 0, SIG_FADE_OFF/2);
					if (sig->outputs[VR_ZL] != SIG_NC && (sig->reduced_dist == 1 || (sig->aspect_wanted_lock == KENN && sig->is_on_hsig == 0))) 
						switchOutput(sig->outputs[VR_ZL], SIG_BIRGHTNESS/4, SIG_FADE_ON/2);
				} else if (current_millis > 100) {
					if (sig->outputs[VR_YELLOW1] != SIG_NC && (sig->aspect_wanted_lock == VR1 || sig->aspect_wanted_lock == VR1_ZS3V || sig->aspect_wanted_lock == KENN))
						switchOutput(sig->outputs[VR_YELLOW1], 0, SIG_FADE_OFF);
					if (sig->outputs[VR_YELLOW1] != SIG_NC && (sig->aspect_wanted_lock == VR2 || sig->aspect_wanted_lock == VR2_ZS3V))
						switchOutput(sig->outputs[VR_YELLOW1], SIG_BIRGHTNESS, SIG_FADE_ON);
				} else if (current_millis > 0) {
					if(sig->outputs[VR_YELLOW2] != SIG_NC)
						switchOutput(sig->outputs[VR_YELLOW2], 0, SIG_FADE_OFF);
				}
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
	
	/*Signal-Init*/
	
	// Anzahl der Signalarten hier festlegen:
	uint8_t num_HV_HSigs = 1;
	uint8_t num_HV_VSigs = 1;
	//
	HV_HSig HV_HSigs[num_HV_HSigs];
	HV_VSig HV_VSigs[num_HV_VSigs];
	
	// Setup der einzelnen Signale. Nicht verwendete Ausg�nge m�ssen als SIG_NC gesetzt werden!
	// Die Z�hlung der Ausg�nge beginnt bei Port I.
	
	// H/V-Hauptsignale	setup_HV_HSig(Gr,	Rt 1,	Rt 2,	Ge, 	SH1,	Kennl.,	ZS3,	ZSx,	ZS1,	VSig.a.M)
	//								  0-15	0-15	0-15	0-15	0-15	0-15	0-15	0-15	0-15	0 o. 1
	HV_HSigs[0] =		setup_HV_HSig(13,	12,		10,		11,		9,		8,		14,		6,		7,		1);
	
	//HV_HSigs[0] =		setup_HV_HSig(
	//					signalWordsDecode(signal_words, 0),
	//					signalWordsDecode(signal_words, 1),
	//					signalWordsDecode(signal_words, 2),
	//					signalWordsDecode(signal_words, 3),
	//					signalWordsDecode(signal_words, 4),
	//					signalWordsDecode(signal_words, 5),
	//					signalWordsDecode(signal_words, 6),
	//					signalWordsDecode(signal_words, 7),
	//					signalWordsDecode(signal_words, 8),
	//					signalWordsDecode(signal_words, 9));
	HV_HSigs[0].hps_1 = HP1_ZSX;
	HV_HSigs[0].hps_2 = HP1_ZS3;
	HV_HSigs[0].hps_3 = HP1_ZS3_ZSX;
	HV_HSigs[0].hps_4 = HP2_ZS3;
	
	// H/V-Vorsignale	setup_HV_VSig(Gr 1,	Gr 2,	Ge 1,	Ge 2,	ZS3V,	Zus.l., Vsig.a.M,	verk.Bremsw.,	Pointer zum HSig am selben Mast)
	//								  0-15	0-15	0-15	0-15	0-15	0-15	0 o. 1		0 o. 1			Pointer o. NULL
	HV_VSigs[0] =		setup_HV_VSig(0,	3,		2,		4,		5,		1,		1,			0,				&HV_HSigs[0]);
	HV_VSigs[0].vrs_1 = VR2_ZS3V;
	
	
	
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
								sendDeviceInfo(id, 0, CHANNEL_OFFSET, ITEM, NAME);
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
							//case (CHANNEL_OFFSET + 1) : {
							//	sendConfigInfoSlider(3 + (MP5_COUNT * 2) + 1, 1, 2048, eeprom_read_word((void *)((CHANNEL_OFFSET + 1)*2)), "Erste R\u00FCckmeldeadresse_1_2048");
							//	break;
							//}
							//case (CHANNEL_OFFSET + 2) : {
							//	sendConfigInfoSlider(3 + (MP5_COUNT * 2) + 2, 1, 2048, eeprom_read_word((void *)((CHANNEL_OFFSET + 2)*2)), "Erste Schaltadresse_1_2048");
							//	break;
							//}
							//case (CHANNEL_OFFSET + 3) : {
							//	sendConfigInfoDropdown(3 + (MP5_COUNT * 2) + 3, 2, eeprom_read_word((void *)((CHANNEL_OFFSET + 3)*2)), "Ausgangsart_Einfach_Paarweise");
							//	break;
							//}
							/*case 20: {
								sendConfigInfoDropdown(20, 17, 0, "Pin Rot 1_NC_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15_16");
								break;
							}*/
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
						
						// Signalsteuerung
						
						if (acc_adr == DCC(1) && acc_state == 0) HV_HSigCmd(CMD_HALT, &HV_HSigs[0]);
						if (acc_adr == DCC(1) && acc_state == 1) HV_HSigCmd(CMD_HP1, &HV_HSigs[0]);
						if (acc_adr == DCC(2) && acc_state == 0) HV_HSigCmd(CMD_SH1, &HV_HSigs[0]);
						if (acc_adr == DCC(2) && acc_state == 1) HV_HSigCmd(CMD_HP2, &HV_HSigs[0]);
						
						if (acc_adr == 0x1800){
							 HV_HSigCmd(acc_state, &HV_HSigs[0]);
							 HV_VSigCmd(acc_power, &HV_VSigs[0]);
							 sendACCEvent(0x1800,HV_HSigs[0].current_cmd, HV_VSigs[0].current_cmd);
						}
						
						
						//uint16_t adr_protocol;
						//if (TRACK_PROTOCOL == 0) adr_protocol = 0x3800;
						//else adr_protocol = 0x3000;
						//
						//if (eeprom_read_word((void *)((CHANNEL_OFFSET + 3)*2)) == 1) {
						//	for (uint8_t i = 0; i < 8; i++) {
						//		if (acc_adr == adr_protocol + eeprom_read_word((void *)((CHANNEL_OFFSET + 2)*2)) + i - 1) {
						//			if (acc_state != output_states[i]) {
						//				if (acc_state == 1) {
						//					switchOutput(i*2, 0xff, 0);
						//					switchOutput(i*2+1, 0, 0);
						//					} else {
						//					switchOutput(i*2, 0, 0);
						//					switchOutput(i*2+1, 0xff, 0);
						//				}
						//				output_states[i] = acc_state;
						//				sendACCEvent((uint32_t)acc_adr, acc_state, acc_power);
						//			}
						//		}
						//	}
						//} else {
						//	for (uint8_t i = 0; i < 16; i++) {
						//		if (acc_adr == adr_protocol + eeprom_read_word((void *)((CHANNEL_OFFSET + 2)*2)) + i - 1) {
						//			if (acc_state != output_states[i]) {
						//				switchOutput(i, acc_state * 0xff, 0);
						//				output_states[i] = acc_state;
						//				sendACCEvent((uint32_t)acc_adr, acc_state, acc_power);
						//			}
						//		}
						//	}
						//}
						
						
												
					}
					break;
				}
				default : break;
			}
		}
		
		/* Update S88 feedback and is_position */
		if (FEEDBACK_ENABLED) {
			for (uint8_t i = 0; i < MP5_COUNT; i++) {
				if (aux_1[i].high == 1 && aux_1_lastreport[i] != 1 && connected[i] == 1) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)), 1, 0);
					aux_1_lastreport[i] = 1;
				} else if (aux_1[i].high == 0 && aux_1_lastreport[i] != 0 && active == 0 && connected[i] == 1) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)), 0, 1);
					aux_1_lastreport[i] = 0;
					is_position[i] = 0;
					sendACCEvent((uint32_t)getAccUID(i), 0, 0);
				}
				if (aux_2[i].high == 1 && aux_2_lastreport[i] != 1&& connected[i] == 1) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)) + 1, 1, 0);
					aux_2_lastreport[i] = 1;
				} else if (aux_2[i].high == 0 && aux_2_lastreport[i] != 0 && active == 0  && connected[i] == 1) {
					if (eeprom_read_word((void *)((i*4)+10)) > 0) sendS88Event(((uint32_t)eeprom_read_word((void *)4) << 16) + eeprom_read_word((void *)((i*4)+10)) + 1, 0, 1);
					aux_2_lastreport[i] = 0;
					is_position[i] = 1;
					sendACCEvent((uint32_t)getAccUID(i), 1, 0);
				}
				eeprom_update_byte((void *)EEPROM_STATE_OFFSET + i, is_position[i]);
			}
		}
		//} else {
		//	/* Send normal input events if feedback disabled */
		//	for (uint8_t i = 0; i < MP5_COUNT; i++) {
		//		uint16_t S88_address = eeprom_read_word((void*)((CHANNEL_OFFSET + 1) * 2)) + 2 * i;
		//		
		//		if (aux_1[i].high != aux_1_lastreport[i]) {
		//			sendS88Event(S88_address, aux_1[i].high, !aux_1[i].high);
		//			aux_1_lastreport[i] = aux_1[i].high;
		//		}
		//		if (aux_2[i].high != aux_2_lastreport[i]) {
		//			sendS88Event(S88_address + 1, aux_2[i].high, !aux_2[i].high);
		//			aux_2_lastreport[i] = aux_2[i].high;
		//		}
		//	}
		//}
		
		/* Inputs not assigned to MP5 */
		//for (uint8_t i = MP5_COUNT; i < 16; i++) {
		//	uint16_t S88_address = 0;
		//	
		//	if (FEEDBACK_ENABLED) S88_address = eeprom_read_word((void*)((CHANNEL_OFFSET + 1) * 2)) + 2 * (i - MP5_COUNT);
		//	else S88_address = eeprom_read_word((void*)((CHANNEL_OFFSET + 1) * 2)) + 2 * i;
		//	
		//	if (aux_1[i].high != aux_1_lastreport[i]) {
		//		sendS88Event(S88_address, aux_1[i].high, !aux_1[i].high);
		//		aux_1_lastreport[i] = aux_1[i].high;
		//	}
		//	if (aux_2[i].high != aux_2_lastreport[i]) {
		//		sendS88Event(S88_address + 1, aux_2[i].high, !aux_2[i].high);
		//		aux_2_lastreport[i] = aux_2[i].high;
		//	}
		//}
		
		/* Switch output one at a time */
		for (uint8_t i = 0; i < MP5_COUNT; i++) {
			if (req_position[i] != is_position[i] && active == 0 && connected[i] == 1) {
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
				break;
			}
			if (!eeprom_read_word((void *)6)) eeprom_update_byte((void *)EEPROM_STATE_OFFSET + i, is_position[i]);
		}
		
		debounce();
		
		/* Reset switching time counter */
		if (active_millis >= 1500) {
			if (FEEDBACK_ENABLED == 0) sendACCEvent(getAccUID(active-1), req_position[active-1], 0);
			active = 0;
			active_millis = 0;
			for (uint8_t i = 0; i < MP5_COUNT; i++) {
				setLow(poz_1[i]);
				setLow(poz_2[i]);
			}
		}
		
		/* Signal Makros */
		
		for (uint8_t i = 0; i < num_HV_HSigs; i++) HV_HSig_makro(&HV_HSigs[i]);
		for (uint8_t i = 0; i < num_HV_VSigs; i++) HV_VSig_makro(&HV_VSigs[i]);
		
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