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
 * MaeCAN Bootloader
 * V 1.9
 * [2022-03-07.1]
 *
 * V1.8: 
 *		- Update-Abbruch fix
 * V1.9:
 *		- Unterstützung von 16Bit Page-Counts
 */

/*
 *	Notwendige Einstellungen zum Programmieren:
 *  Bootloader-Section in den Fuses: 2048 Words
 * 
 *  Einstellungen in Atmel Studio:
 *  ATmega328P: Toolchain -> AVR/GNU Linker -> Misc.: -Wl,--section-start=.text=0x7000
 * 				Fuses: FD D8 FF
 *	ATmega2560: Toolchain -> AVR/GNU Linker -> Misc.: -Wl,--section-start=.text=0x3f000
 *				Fuses: FF D2 FD
 *  ATmega1280: Toolchain -> AVR/GNU Linker -> Misc.: -Wl,--section-start=.text=0x1f000
 * 				Fuses: FF D2 FF
 *	ATmega16A:  Toolchain -> AVR/GNU Linker -> Misc.: -Wl,--section-start=.text=0xXXXXX
 *				Fuses: XX XX XX
 */


/* 
 * Config options 
 * Die nachfolgenden Deffinitionen bitte ausschließlich unter Toolchain -> AVR/GNU Compiler -> Symbols ändern! 
 * Andernfalls ist die externe Makefile nicht zuverlässig.
 */


//#define F_CPU 16000000UL

//#define TYPE 0x51 /* Busankoppler (328P) */
//#define TYPE 0x52 /* MP5x16 (2560) */
//#define TYPE 0x53 /* Dx32 (1280),(2560) */

//#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <string.h>
#include "../Common/mcan.h"

#if TYPE == 0x51
	#define PAGE_COUNT 224
	#define LED_PORT PORTB
	#define LED_DDR DDRB
	#define LED_PIN PB1
	#define NAME "Busankoppler"
	#define ITEM "BAK-01"
	#define BASE_UID 0x43440000
#elif TYPE == 0x52
	#define PAGE_COUNT 1008
	#define LED_PORT PORTD
	#define LED_DDR DDRD
	#define LED_PIN	PD3
	#define NAME "M\u00E4CAN 16-fach MP5-Decoder"
	#define ITEM "MP5x16"
	#define BASE_UID 0x4d430000
#elif TYPE == 0x53
	#define PAGE_COUNT 496
	#define LED_PORT PORTD
	#define LED_DDR DDRD
	#define LED_PIN	PD3
	#define NAME "M\u00E4CAN 32-fach Gleisbelegtmelder"
	#define ITEM "Dx32"
	#define BASE_UID 0x4d430000
#elif TYPE == 0x70
	#define PAGE_COUNT 224
	#define LED_PORT PORTB
	#define LED_DDR DDRB
	#define LED_PIN PB1
	#define NAME "NScale Grundmodul"
	#define ITEM "NSGM"
	#define BASE_UID 0x19000000
#elif TYPE == 0x54
	#define PAGE_COUNT
	#define LED_PORT
	#define LED_DDR
	#define LED_PIN
	#define NAME "M\u00E4CAN 16-Fach Schaltdecoder"
	#define ITEM "Ox16"
	#define BASE_UID 0x4d430000
#else
	#error "No device type has been set!"
#endif

/* Deactivate Watchdog */
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));

void wdt_init(void)
{
	MCUSR = 0;
	wdt_disable();

	return;
}

uint32_t can_uid;
uint16_t hash;
uint16_t serial_nbr;

uint32_t millis;
uint16_t ledmillis;

canFrame frame_out;
canFrame frame_in;

uint8_t updating;

uint16_t page_index;
uint8_t page_buffer[SPM_PAGESIZE];

uint8_t frame_index_check;


void (*start)(void) = 0x0000;	/* Sprung zum Programmstart */

void startApp(void) {
	unsigned char temp;
	cli();
	temp = MCUCR;
	MCUCR = temp | (1<<IVCE);
	MCUCR = temp & ~(1<<IVSEL);
	#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
	EIND = 0;
	#endif
	start();
}

void errasePage(uint32_t page)
{
	// uint16_t i;
	uint8_t sreg;

	/* Disable interrupts */
	sreg = SREG;
	cli();

	eeprom_busy_wait ();

	boot_page_erase (page);
	boot_spm_busy_wait ();      /* Wait until the memory is erased. */
	/* Reenable RWW-section again. We need this if we want to jump back */
	/* to the application after bootloading. */
	boot_rww_enable ();

	/* Re-enable interrupts (if they were ever enabled). */
	SREG = sreg;
}

void programPage (uint32_t page, uint8_t *buf)
{
	uint16_t i;
	uint8_t sreg;

	/* Disable interrupts */
	sreg = SREG;
	cli();

	eeprom_busy_wait ();

	boot_page_erase (page);
	boot_spm_busy_wait ();      /* Wait until the memory is erased. */

	for (i = 0; i < SPM_PAGESIZE; i += 2)
	{
		/* Set up little-endian word. */
		uint16_t w = *buf++;
		w += (*buf++) << 8;
		
		boot_page_fill (page + i, w);
	}

	boot_page_write (page);     /* Store buffer in flash page.		*/
	boot_spm_busy_wait();       /* Wait until the memory is written.*/

	/* Reenable RWW-section again. We need this if we want to jump back */
	/* to the application after bootloading. */
	boot_rww_enable ();

	/* Re-enable interrupts (if they were ever enabled). */
	SREG = sreg;
}

int main(void)
{
	serial_nbr = (uint16_t)(boot_signature_byte_get(22) << 8) | boot_signature_byte_get(23);
	//can_uid = BASE_UID + serial_nbr;
	//hash = generateHash(can_uid);
	
	unsigned char temp = MCUCR;
	MCUCR = temp | (1<<IVCE);
	MCUCR = temp | (1<<IVSEL);
		
	/* init CAN */
	mcan_init(BASE_UID + serial_nbr);
	
	// Setup heartbeat timer:
	TCCR0A |= (1 << WGM01); // Timer 0 clear time on compare match
	OCR0A = (F_CPU / (64 * 1000UL)) - 1; // Timer 0 compare value
	TIMSK0 |= (1 << OCIE0A); // Timer interrupt
	sei();
	TCCR0B |= ((1 << CS01)|(1 << CS00)); // Set timer 0 prescaler to 64
	
	frame_out.cmd = 0x40;
	frame_out.resp = 1;
	frame_out.hash = mcan_hash;
	frame_out.dlc = 7;
	frame_out.data[0] = (mcan_uid >> 24);
	frame_out.data[1] = (mcan_uid >> 16);
	frame_out.data[2] = (mcan_uid >> 8);
	frame_out.data[3] = mcan_uid;
	frame_out.data[6] = 0;
	frame_out.data[7] = 0;

	LED_DDR |= (1 << LED_PIN);
	LED_PORT |= (1 << LED_PIN);
	
	if (eeprom_read_byte((void *)1023) == 0x01) {
		// Bootloader triggered:
		frame_out.data[4] = 0x01;
		frame_out.data[5] = TYPE;
		frame_out.data[6] = VERSION;
		sendCanFrame(&frame_out);
		
		frame_out.dlc = 7;
		frame_out.data[4]++;
		frame_out.data[5] = (uint8_t)(SPM_PAGESIZE >> 8);
		frame_out.data[6] = (uint8_t)SPM_PAGESIZE;
		sendCanFrame(&frame_out);
		
		frame_out.data[4]++;
		frame_out.data[5] = (uint8_t)(PAGE_COUNT >> 8);
		frame_out.data[6] = (uint8_t)PAGE_COUNT;
		sendCanFrame(&frame_out);
		
		eeprom_update_byte((void *)1023, 0);
	}
	
	while (1) 
    {
		if (readCanFrame(&frame_in)) {
			
			if (frame_in.cmd == CMD_PING && frame_in.resp == 0) {
				sendPingFrame(0, TYPE);
			} else if (frame_in.cmd == CMD_CONFIG && frame_in.resp == 0 && compareUID(frame_in.data, mcan_uid) && frame_in.data[4] == 0) {
				sendDeviceInfo(serial_nbr, 0, 0, ITEM, NAME);
			} else if (frame_in.cmd == 0x40) {
				if (updating == 0) {
					// Bootloader in den Update-Modus versetzen, wenn Updater Ping-Antwort best�tigt:
					if (compareUID(frame_in.data, mcan_uid)) {
						if (frame_in.dlc == 7 && frame_in.resp == 1 && frame_in.data[4] == 0x01){
							if (frame_in.data[6] == 1) {
								updating = 1;
								eeprom_update_byte((void *)1023, 0);
							}
						} else if (frame_in.dlc == 4) {
							eeprom_update_byte((void *)1023, 1);
						}
					}
				} else {
					if (frame_in.dlc == 8) {
						// Daten-Frame in Puffer schreiben:
						frame_index_check++;
						uint8_t frame_index = (frame_in.hash - 0x301);
						memcpy(&page_buffer[frame_index * 8], frame_in.data, 8);
						
					} else if (compareUID(frame_in.data, mcan_uid)) {
						switch (frame_in.data[4]) {
							/* LEGACY */
							case 0x04 : {
								// Page-Start:
								page_index = frame_in.data[5];
								break;
							}
							case 0x05 : {
								//Page-Ende:
								
								frame_out.dlc = 7;
								frame_out.data[4] = 0x05;
								frame_out.data[5] = frame_in.data[5];
								
								if ((page_index != frame_in.data[5]) || (frame_index_check != (SPM_PAGESIZE / 8))) {
									// Page fehlerhaft, erneut anfordern:
									frame_out.data[6] = 0;
								} else {
									// Page schreiben:
									programPage(frame_in.data[5] * SPM_PAGESIZE, page_buffer);
									frame_out.data[6] = 1;
								}
								sendCanFrame(&frame_out);
								frame_index_check = 0;
								break;
							}
							/* END LEGACY */
							case 0x14 : {
								page_index = (frame_in.data[5] << 8) | frame_in.data[6];
								break;
							}
							case 0x15 : {
								frame_out.data[4] = 0x15;
								frame_out.data[5] = frame_in.data[5];
								frame_out.data[6] = frame_in.data[6];
								
								if ((page_index != ((frame_in.data[5] << 8) | frame_in.data[6])) || (frame_index_check != (SPM_PAGESIZE / 8))) {
									// Page fehlerhaft, erneut anfordern:
									frame_out.dlc = 5;
								} else {
									// Page schreiben:
									frame_out.dlc = 7;
									programPage(page_index * SPM_PAGESIZE, page_buffer);
								}
								sendCanFrame(&frame_out);
								frame_index_check = 0;
								break;
							}
							case 0x06 : {
								// Update-Abbruch:
								for (uint32_t i = 0; i < PAGE_COUNT; i++) {
									errasePage(i*SPM_PAGESIZE);
								}
								updating = 0;
								break;
							}
							case 0x07 : {
								// Update-Ende:
								updating = 0;
								break;
							}
							default : {
								// Unbekannter Befehl, abbrechen:
								for (uint32_t i = 0; i < PAGE_COUNT; i++) {
									errasePage(i*SPM_PAGESIZE);
								}
								updating = 0;
								break;
							}
						}
					}
				}
			}
			
			
			
		}
		
		if (millis >= 2000 && updating != 1) {
			startApp();
		}
		
		if (ledmillis >= (200 - (updating * 100))) {
			ledmillis = 0;
			LED_PORT ^= _BV(LED_PIN);
		}
    }
}
ISR(TIMER0_COMPA_vect) {
	++millis;
	++ledmillis;
}