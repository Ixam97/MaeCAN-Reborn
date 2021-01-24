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
 * [2021-01-24.1]
 */

#ifndef MCAN_H
#define MCAN_H

/*
 *  Adressbereich der Local-IDs:
 */
#define MM_ACC 		  0x3000	//Magnetartikel Motorola
#define DCC_ACC 	  0x3800	//Magbetartikel NRMA_DCC
#define MM_TRACK 	  0x0000	//Gleissignal Motorola
#define DCC_TRACK 	0xC000	//Gleissignal NRMA_DCC

/*
 *  CAN-Befehle (Märklin)
 */
#define SYS_CMD		0x00 	//Systembefehle
 	#define SYS_STOP 	0x00 	//System - Stopp
 	#define SYS_GO		0x01	//System - Go
 	#define SYS_HALT	0x02	//System - Halt
 	#define SYS_STAT	0x0b	//System - Status (sendet geänderte Konfiguration oder übermittelt Messwerte)
#define CMD_SWITCH_ACC 	0x0b	//Magnetartikel Schalten
#define CMD_S88_EVENT	0x11	//Rückmelde-Event
#define CMD_PING 		0x18	//CAN-Teilnehmer anpingen
#define CMD_CONFIG		0x1d	//Konfiguration
#define CMD_BOOTLOADER	0x1B

#include <avr/io.h>

uint16_t generateHash(uint32_t uid);

void sendDeviceInfo(uint32_t uid, uint16_t hash, uint32_t serial_nbr, uint8_t meassure_ch, uint8_t config_ch, char *art_nbr, char *name);
void sendConfigInfoDropdown(uint32_t uid, uint16_t hash, uint8_t channel, uint8_t options, uint8_t default_option, char *string);
void sendConfigInfoSlider(uint32_t uid, uint16_t hash, uint8_t channel, uint16_t min_value, uint16_t max_value, uint16_t default_value, char *string);
void sendPingFrame(uint32_t uid, uint16_t hash, uint16_t version, uint16_t type);
void sendConfigConfirm(uint32_t uid, uint16_t hash, uint8_t channel);
void sendS88Event(uint32_t portID, uint16_t hash, uint8_t old_value, uint8_t new_value);
void sendACCEvent(uint32_t accUID, uint16_t hash, uint8_t value, uint8_t power);

#endif