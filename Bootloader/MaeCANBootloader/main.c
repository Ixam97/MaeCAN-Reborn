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
 * V 1.1
 * [2020-08-27.1]
 */

/*
 *	Notwendige Einstellungen zum Programmieren:
 *  Bootloader-Section in den Fuses: 2048 Words
 *  ATmega328P: Toolchain -> AVR/GNU Linker -> Misc.: -Wl,--section-start=.text=0x7000
 *	ATmega2560: Toolchain -> AVR/GNU Linker -> Misc.: -Wl,--section-start=.text=0x3f000
 */


/* Config options */
// #define UART


//#define TYPE 0x51 /* Busankoppler (328P) */
#define TYPE 0x52 /* MP5x16 (2560), Fuses: FF 92 FD*/


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include "mcp2515_basic.h"
#include "mcan.h"

#ifdef UART
	#include "uart.h"
#endif

#if TYPE == 0x51
	#define F_CPU 16000000UL
	#define PAGE_COUNT 224
	#define LED_PORT PORTB
	#define LED_DDR DDRB
	#define LED_PIN PB1
	#define INTVECT INT0_vect
	#define NAME "Busankoppler"
	#define ITEM "BAK-01"
	#define BASE_UID 0x43440000
#elif TYPE == 0x52
	#define F_CPU 16000000UL
	#define PAGE_COUNT 1008
	#define LED_PORT PORTD
	#define LED_DDR DDRD
	#define LED_PIN	PD3
	#define INTVECT INT2_vect
	#define NAME "M\u00E4CAN 16-fach MP5-Decoder"
	#define ITEM "MP5x16"
	#define BASE_UID 0x4d430000
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
uint32_t update_start_millis;

canFrame frame_out;
canFrame frame_in_buffer;
canFrame frame_in;

uint8_t new_frame;

uint8_t updating;

uint16_t page_index;
uint8_t page_buffer[SPM_PAGESIZE];

uint8_t frame_index_check;


void (*start)(void) = 0x0000;	/* Sprung zum Programmstart */

uint8_t compareUID(uint8_t data[8], uint32_t _uid) {
	/* Checking received UID for relevance. */
	
	if ((data[0] == (uint8_t)(_uid >> 24)) && (data[1] == (uint8_t)(_uid >> 16)) && (data[2] == (uint8_t)(_uid >> 8)) && (data[3] == (uint8_t)_uid)) {
		return 1;
		} else {
		return 0;
	}
}

void startApp() {
	unsigned char temp;
	cli();
	temp = MCUCR;
	MCUCR = temp | (1<<IVCE);
	MCUCR = temp & ~(1<<IVSEL);
	#if defined(__AVR_ATmega2560__)
	EIND = 0;
	#endif
	start();
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

	for (i=0; i<SPM_PAGESIZE; i+=2)
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

void main(void)
{
	serial_nbr = (uint16_t)(boot_signature_byte_get(22) << 8) | boot_signature_byte_get(23);
	can_uid = BASE_UID + serial_nbr;
	hash = generateHash(can_uid);
	
	unsigned char temp = MCUCR;
	MCUCR = temp | (1<<IVCE);
	MCUCR = temp | (1<<IVSEL);
		
	/* init CAN */
	mcp2515_init();
	
	// Setup ext. interrupt on falling edge:
	#if defined(__AVR_ATmega328P__)
	DDRD &= ~(1 << 2);
	PORTD |= (1 << 2);
	EICRA |= (1 << ISC00);
	EIMSK |= (1 << INT0);
	#elif defined(__AVR_ATmega2560__)
	EICRA |= (1 << ISC21);
	EICRA &= ~(1 << ISC20);
	EIMSK |= (1 << INT2);
	DDRD &= ~(1 << 2);
	PORTD |= (1 << 2);
	#endif
	
	#ifdef UART
		uart_init(UART_BAUD_SELECT(115200, F_CPU));
	#endif
	
	// Setup heartbeat timer:
	TCCR0A |= (1 << WGM01); // Timer 0 clear time on compare match
	OCR0A = 249; // Timer 0 compare value
	TIMSK0 |= (1 << OCIE0A); // Timer interrupt
	sei();
	TCCR0B |= ((1 << CS01)|(1 << CS00)); // Set timer 0 prescaler to 1
	
	frame_out.cmd = 0x40;
	frame_out.resp = 1;
	frame_out.hash = hash;
	frame_out.dlc = 6;
	frame_out.data[0] = (can_uid >> 24);
	frame_out.data[1] = (can_uid >> 16);
	frame_out.data[2] = (can_uid >> 8);
	frame_out.data[3] = can_uid;
	frame_out.data[6] = 0;
	frame_out.data[7] = 0;
	

	#ifdef UART	
		uart_puts("\n\rMaeCAN Bootloader startup\n\r");
		uart_puts(DEVICE);
		uart_puts(", UID 0x");
		char uid_string[8];
		ltoa(can_uid, uid_string, 16);
		uart_puts(uid_string);
		uart_puts("\n\r");
	#endif

	LED_DDR |= (1 << LED_PIN);
	LED_PORT |= (1 << LED_PIN);
	
	if (eeprom_read_byte(1023) == 0x01) {
		// Bootloader triggered:
		frame_out.data[4] = 0x01;
		frame_out.data[5] = TYPE;
		sendCanFrame(&frame_out);
		
		frame_out.dlc = 7;
		frame_out.data[4]++;
		frame_out.data[5] = SPM_PAGESIZE >> 8;
		frame_out.data[6] = SPM_PAGESIZE;
		sendCanFrame(&frame_out);
		
		frame_out.data[4]++;
		frame_out.data[5] = PAGE_COUNT >> 8;
		frame_out.data[6] = PAGE_COUNT;
		sendCanFrame(&frame_out);
		
		eeprom_update_byte(1023, 0);
	}
    
	while (1) 
    {
		if (new_frame == 1) {
			frame_in = frame_in_buffer;
			new_frame = 0;
			
			if (frame_in.cmd == CMD_PING && frame_in.resp == 0) {
				sendPingFrame(can_uid, hash, 0, TYPE);
			} else if (frame_in.cmd == CMD_CONFIG && frame_in.resp == 0 && compareUID(frame_in.data, can_uid) && frame_in.data[4] == 0) {
				sendDeviceInfo(can_uid, hash, serial_nbr, 0, 0, ITEM, NAME);
			} else if (frame_in.cmd == 0x40) {
				if (updating == 0) {
					// Bootloader in den Update-Modus versetzen, wenn Updater Ping-Antwort best�tigt:
					if (compareUID(frame_in.data, can_uid)) {
						if (frame_in.dlc == 7 && frame_in.resp == 1 && frame_in.data[4] == 0x01){
							if (frame_in.data[6] == 1) {
								updating = 1;
								update_start_millis = millis;
								eeprom_update_byte(1023, 0);
								#ifdef UART
									uart_puts("Updating firmware...\n\r");
								#endif
							}
						} else if (frame_in.dlc == 4) {
							eeprom_update_byte(1023, 1);
						}
					}
				} else {
					if (frame_in.dlc == 8) {
						// Daten-Frame in Puffer schreiben:
						frame_index_check++;
						uint8_t frame_index = (frame_in.hash - 0x301);
						memcpy(&page_buffer[frame_index * 8], frame_in.data, 8);
						#ifdef UART
							uart_putc('.');
						#endif
						
					} else if (compareUID(frame_in.data, can_uid)) {
						switch (frame_in.data[4]) {
							case 0x04 : {
								// Page-Start:
								page_index = frame_in.data[5];
								#ifdef UART
									uart_puts("Page ");
									char _page_index;
									itoa(page_index, _page_index, 10);
									uart_puts(_page_index);
								#endif
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
									#ifdef UART
										uart_puts(" error, new attempt.\n\r");
									#endif
								} else {
									// Page schreiben:
									programPage(frame_in.data[5] * SPM_PAGESIZE, page_buffer);
									frame_out.data[6] = 1;
									#ifdef UART
										uart_puts(" written.\n\r");
									#endif
								}
								sendCanFrame(&frame_out);
								frame_index_check = 0;
								break;
							}
							case 0x06 : {
								// Update-Abbruch:
								#ifdef UART
									uart_puts("Aborting update.\n\r");
								#endif
								memset(page_buffer, 0xff, SPM_PAGESIZE);
								programPage(0, page_buffer);
								updating = 0;
								break;
							}
							case 0x07 : {
								// Update-Ende:
								#ifdef UART
									uart_puts("Firmware update done after ");
									char update_time[7];
									ltoa(millis - update_start_millis, update_time, 10);
									uart_puts(update_time);
									uart_puts("ms!\n\r\n\r");
									_delay_ms(400);
								#endif
								updating = 0;
								break;
							}
							default : {
								// Unbekannter Befehl, abbrechen:
								#ifdef UART
									uart_puts("Unknown command. Aborting update.\n\r");
								#endif
								memset(page_buffer, 0xff, SPM_PAGESIZE);
								programPage(0, page_buffer);
								updating = 0;
								break;
							}
						}
					}
				}
			}
			
			
			
		}
		
		#ifdef UART
			char c = uart_getc();
			if(c > 0){
				uart_putc(c);
				uart_puts("\n\r");
				
				if (c == 's') {
					
					uart_puts("Jumping to address 0x0000...\n\r\n\r");
					_delay_ms(1000);
					
					startApp();
				}
			}
		#endif
		
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
ISR(INTVECT) {
	/* process CAN-frame */
	getCanFrame(&frame_in_buffer);
	new_frame = 1;
}
