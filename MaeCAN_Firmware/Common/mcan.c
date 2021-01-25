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
 * [2021-01-25.1]
 */

#include "mcan.h"
#include "mcp2515_basic.h"

uint16_t generateHash(uint32_t uid) {
	uint16_t uid_high = (uint16_t)(uid >> 16);
	uint16_t uid_low  = (uint16_t)(uid);
	
	uint16_t hash = uid_high ^ uid_low;
	hash = (hash | 0x300) & ~0x80;
	
	return hash;
}

/*
 * sendDeviceInfo: Sending basic device information to GUI
 */

void sendDeviceInfo(uint32_t uid, uint16_t hash, uint32_t serial_nbr, uint8_t meassure_ch, uint8_t config_ch, char *art_nbr, char *name) {
	canFrame frame;
	
	frame.cmd = CMD_CONFIG;
	frame.resp = 1;
	frame.hash = 0x301;
	frame.dlc = 8;
	frame.data[0] = meassure_ch; //number of measurement channels
	frame.data[1] = config_ch;   //number of configuration channels
	frame.data[2] = 0;
	frame.data[3] = 0;
	frame.data[4] = (uint8_t) (serial_nbr >> 24); //serial number
	frame.data[5] = (uint8_t) (serial_nbr >> 16);
	frame.data[6] = (uint8_t) (serial_nbr >> 8);
	frame.data[7] = (uint8_t) serial_nbr;
	
	sendCanFrame(&frame);
	
	
	uint8_t art_nbr_len = 0;
	
	while(1) {
		if (art_nbr[art_nbr_len] == 0) break;
		else art_nbr_len++;
	}
	
	for (uint8_t i = 0; i < 8; i++) {
		if (i < art_nbr_len) frame.data[i] = art_nbr[i];
		else frame.data[i] = 0;
	}
	frame.hash++;
	sendCanFrame(&frame);
	
	uint8_t name_len = 0;
	uint8_t needed_frames = 0;
	
	while(1) {
		if (name[name_len] == 0) break;
		else name_len++;
	}
	
	if ((name_len % 8) > 0) needed_frames = (name_len / 8) + 1;
	else needed_frames = name_len / 8;
	
	for (uint8_t i = 0; i < needed_frames; i++){
		for (uint8_t j = 0; j < 8; j++){
			if ((8 * i) + j < name_len) frame.data[j] = name[(8 * i) + j];
			else frame.data[j] = 0;
		}
		frame.hash++;
		sendCanFrame(&frame);
	}
	
	uint8_t frame_count = frame.hash - 0x300;
	
	frame.hash = hash;
	frame.dlc = 6;
	frame.data[0] = (uint8_t) (uid >> 24);
	frame.data[1] = (uint8_t) (uid >> 16);
	frame.data[2] = (uint8_t) (uid >> 8);
	frame.data[3] = (uint8_t) uid;
	frame.data[4] = 0;
	frame.data[5] = frame_count;
	
	sendCanFrame(&frame);
}

void sendPingFrame(uint32_t uid, uint16_t hash, uint16_t version, uint16_t type) {
	canFrame frame = {
		.cmd = CMD_PING,
		.resp = 1,
		.hash = hash,
		.dlc = 8,
		.data[0] = (uint8_t)(uid >> 24),
		.data[1] = (uint8_t)(uid >> 16),
		.data[2] = (uint8_t)(uid >> 8),
		.data[3] = (uint8_t)uid,
		.data[4] = (uint8_t)(version >> 8),
		.data[5] = (uint8_t)version,
		.data[6] = (uint8_t)(type >> 8),
		.data[7] = (uint8_t)type
	};
	
	sendCanFrame(&frame);
}

/*
 *
 */
void sendConfigInfoDropdown(uint32_t uid, uint16_t hash, uint8_t channel, uint8_t options, uint8_t default_option, char *string) {
	canFrame frame;
	
	frame.cmd = CMD_CONFIG;
	frame.resp = 1;
	frame.hash = 0x301;
	frame.dlc = 8;
	
	// Basic chanel info:
	frame.data[0] = channel;
	frame.data[1] = 1;
	frame.data[2] = options;
	frame.data[3] = default_option;
	frame.data[4] = 0;
	frame.data[5] = 0;
	frame.data[6] = 0;
	frame.data[7] = 0;
	
	sendCanFrame(&frame);
	
	// Chanel text:
	uint8_t string_len = 0;
	uint8_t needed_frames = 0;
	
	while (1) {
		if (string[string_len] == 0) break;
		else string_len++;
	}
	
	if ((string_len % 8) > 0) needed_frames = (string_len / 8) + 1;
	else needed_frames = string_len / 8;
	
	for (uint8_t i = 0; i < needed_frames; i++){
		for(uint8_t j = 0; j < 8; j++){
			if(((8*i)+j < string_len) && (string[(8*i)+j] != '_')) frame.data[j] = string[(8*i)+j];
			else frame.data[j] = 0;
		}
		frame.hash++;
		sendCanFrame(&frame);
	}
	
	// End Frame:
	uint8_t frame_count = frame.hash - 0x300;
	
	frame.hash = hash;
	frame.dlc = 6;
	frame.data[0] = (uint8_t) (uid >> 24);
	frame.data[1] = (uint8_t) (uid >> 16);
	frame.data[2] = (uint8_t) (uid >> 8);
	frame.data[3] = (uint8_t) uid;
	frame.data[4] = channel;
	frame.data[5] = frame_count;
	
	sendCanFrame(&frame);
}

void sendConfigInfoSlider(uint32_t uid, uint16_t hash, uint8_t channel, uint16_t min_value, uint16_t max_value, uint16_t default_value, char *string) {
	canFrame frame;
	
	frame.cmd = CMD_CONFIG;
	frame.resp = 1;
	frame.hash = 0x301;
	frame.dlc = 8;
	
	// Basic channel info:
	frame.data[0] = channel;
	frame.data[1] = 2;
	frame.data[2] = (uint8_t)(min_value >> 8);
	frame.data[3] = (uint8_t)min_value;
	frame.data[4] = (uint8_t)(max_value >> 8);
	frame.data[5] = (uint8_t)max_value;
	frame.data[6] = (uint8_t)(default_value >> 8);
	frame.data[7] = (uint8_t)default_value;
	
	sendCanFrame(&frame);
	
	// Chanel text:
	uint8_t string_len = 0;
	uint8_t needed_frames = 0;
	
	while (1) {
		if (string[string_len] == 0) break;
		else string_len++;
	}
	
	if (((string_len + 1) % 8) > 0) needed_frames = ((string_len + 1) / 8) + 1;
	else needed_frames = (string_len + 1) / 8;
	
	for(int i = 0; i < needed_frames; i++){
		for(int j = 0; j < 8; j++){
			if(((8*i)+j < string_len) && (string[(8*i)+j] != '_')) frame.data[j] = string[(8*i)+j];
			else frame.data[j] = 0;
		}
		
		frame.hash++;
		sendCanFrame(&frame);		
	}
	
	// End Frame:
	uint8_t frame_count = frame.hash - 0x300;

	frame.hash = hash;
	frame.dlc = 6;
	frame.data[0] = (uint8_t) (uid >> 24);
	frame.data[1] = (uint8_t) (uid >> 16);
	frame.data[2] = (uint8_t) (uid >> 8);
	frame.data[3] = (uint8_t) uid;
	frame.data[4] = channel;
	frame.data[5] = frame_count;

	sendCanFrame(&frame);	
}

void sendConfigConfirm(uint32_t uid, uint16_t hash, uint8_t channel) {
	canFrame frame;
	
	frame.hash = hash;
	frame.cmd = SYS_CMD;
	frame.resp = 1;
	frame.dlc = 7;
	
	frame.data[0] = (uint8_t) (uid >> 24);
	frame.data[1] = (uint8_t) (uid >> 16);
	frame.data[2] = (uint8_t) (uid >> 8);
	frame.data[3] = (uint8_t) uid;
	frame.data[4] = SYS_STAT;
	frame.data[5] = channel;
	frame.data[6] = 1;
	frame.data[7] = 0;
	
	sendCanFrame(&frame);
}

void sendS88Event(uint32_t portID, uint16_t hash, uint8_t old_value, uint8_t new_value) {
	canFrame frame;
	
	frame.hash = hash;
	frame.cmd = CMD_S88_EVENT;
	frame.resp = 1;
	frame.dlc = 8;
	frame.data[0] = (uint8_t) (portID >> 24);
	frame.data[1] = (uint8_t) (portID >> 16);
	frame.data[2] = (uint8_t) (portID >> 8);
	frame.data[3] = (uint8_t) portID;
	frame.data[4] = old_value;
	frame.data[5] = new_value;
	frame.data[6] = 0;
	frame.data[7] = 0;
	
	sendCanFrame(&frame);
}

void sendACCEvent(uint32_t accUID, uint16_t hash, uint8_t value, uint8_t power) {
	canFrame frame;
	
	frame.hash = hash;
	frame.cmd = CMD_SWITCH_ACC;
	frame.resp = 1;
	frame.dlc = 6;
	frame.data[0] = (uint8_t) (accUID >> 24);
	frame.data[1] = (uint8_t) (accUID >> 16);
	frame.data[2] = (uint8_t) (accUID >> 8);
	frame.data[3] = (uint8_t) accUID;
	frame.data[4] = value;
	frame.data[5] = power;
	frame.data[6] = 0;
	frame.data[7] = 0;
	
	sendCanFrame(&frame);
}

void sendStatus(uint32_t uid, uint16_t hash, uint8_t channel, uint16_t value) {
	canFrame frame;
	
	frame.hash = hash;
	frame.cmd = SYS_CMD;
	frame.resp = 1;
	frame.dlc = 8;
	frame.data[0] = (uint8_t) (uid >> 24);
	frame.data[1] = (uint8_t) (uid >> 16);
	frame.data[2] = (uint8_t) (uid >> 8);
	frame.data[3] = (uint8_t) uid;
	frame.data[4] = SYS_STAT;
	frame.data[5] = channel;
	frame.data[6] = (uint8_t) (value >> 8);
	frame.data[7] = (uint8_t) value;
	
	sendCanFrame(&frame);
	
}