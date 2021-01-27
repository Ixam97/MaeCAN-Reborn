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
 * [2021-01-27.1]
 */

#include "slcan.h"

const char asciilookup[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	
void sendSLCAN(canFrame *pFrame) {
	uint32_t _ID = (((uint32_t)pFrame->cmd << 17) | ((uint32_t)pFrame->resp << 16) | pFrame->hash);
		
	char message[] = {
		'T',
		/* i */ asciilookup[((_ID >> 28) & 0b1111)],
		/* i */ asciilookup[((_ID >> 24) & 0b1111)],
		/* i */ asciilookup[((_ID >> 20) & 0b1111)],
		/* i */ asciilookup[((_ID >> 16) & 0b1111)],
		/* i */ asciilookup[((_ID >> 12) & 0b1111)],
		/* i */ asciilookup[((_ID >> 8) & 0b1111)],
		/* i */ asciilookup[((_ID >> 4) & 0b1111)],
		/* i */ asciilookup[(_ID & 0b1111)],
		/* l */ asciilookup[pFrame->dlc],
		/* d */ asciilookup[((pFrame->data[0] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[0] & 0b1111)],
		/* d */ asciilookup[((pFrame->data[1] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[1] & 0b1111)],
		/* d */ asciilookup[((pFrame->data[2] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[2] & 0b1111)],
		/* d */ asciilookup[((pFrame->data[3] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[3] & 0b1111)],
		/* d */ asciilookup[((pFrame->data[4] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[4] & 0b1111)],
		/* d */ asciilookup[((pFrame->data[5] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[5] & 0b1111)],
		/* d */ asciilookup[((pFrame->data[6] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[6] & 0b1111)],
		/* d */ asciilookup[((pFrame->data[7] >> 4) & 0b1111)],
		/* d */ asciilookup[(pFrame->data[7] & 0b1111)]};
	// Tiiiiiiiildddddddddddddddd\r
	
	uart_putc(message[0]);
	uart_putc(message[1]);
	uart_putc(message[2]);
	uart_putc(message[3]);
	uart_putc(message[4]);
	uart_putc(message[5]);
	uart_putc(message[6]);
	uart_putc(message[7]);
	uart_putc(message[8]);
	uart_putc(message[9]);
	
	for (uint8_t i = 10; i < 10 + pFrame->dlc * 2; i++) {
		uart_putc(message[i]);
	}
	
	uart_putc(0x0d);
	#ifdef NEWLINE
	uart_putc(0x0a);
	#endif // NEWLINE
	
	//uart_puts(message);
}

uint8_t decodeAscii(char character) {
	switch (character) {
		case '1': return 0x01;
		case '2': return 0x02;
		case '3': return 0x03;
		case '4': return 0x04;
		case '5': return 0x05;
		case '6': return 0x06;
		case '7': return 0x07;
		case '8': return 0x08;
		case '9': return 0x09;
		case 'A': return 0x0a;
		case 'B': return 0x0b;
		case 'C': return 0x0c;
		case 'D': return 0x0d;
		case 'E': return 0x0e;
		case 'F': return 0x0f;
		case 'a': return 0x0a;
		case 'b': return 0x0b;
		case 'c': return 0x0c;
		case 'd': return 0x0d;
		case 'e': return 0x0e;
		case 'f': return 0x0f;
		case '0': return 0x00;
		default: return 0;
		
	}
}

void getSLCAN(canFrame *frame) {
	uint32_t rxID = 0;
	
	rxID += (uint32_t)decodeAscii(slcan_buffer[0]) << 28;
	rxID += (uint32_t)decodeAscii(slcan_buffer[1]) << 24;
	rxID += (uint32_t)decodeAscii(slcan_buffer[2]) << 20;
	rxID += (uint32_t)decodeAscii(slcan_buffer[3]) << 16;
	rxID += (uint32_t)decodeAscii(slcan_buffer[4]) << 12;
	rxID += (uint32_t)decodeAscii(slcan_buffer[5]) << 8;
	rxID += (uint32_t)decodeAscii(slcan_buffer[6]) << 4;
	rxID += (uint32_t)decodeAscii(slcan_buffer[7]);
	frame->dlc = decodeAscii(slcan_buffer[8]);
	
	frame->cmd = (uint8_t)(rxID >> 17);
	frame->resp = (uint8_t)(rxID >> 16) & 0x01;
	frame->hash = (uint16_t)(rxID & 0xffff);
	
	for (uint8_t i = 0; i < frame->dlc; i++) {
		frame->data[i] = (decodeAscii(slcan_buffer[9+2*i]) << 4) + (decodeAscii(slcan_buffer[10+2*i]) & 0b1111);
	}
}

uint8_t SLCANFrameReady(void) {
	uint16_t uart_in = uart_getc();
	
	if (uart_in >> 8 == 0 && (char)uart_in == 'T') {
		slcan_buffer_index = 0;
		} else if (uart_in >> 8 == 0 && ((char)uart_in == 0x0d || (char)uart_in == 0x0a) && slcan_buffer_index < 26) {
		if (slcan_buffer_index >= 9) {
			slcan_buffer_index = 25;
			} else {
			slcan_buffer_index = 26;
		}
		} else if (uart_in >> 8 == 0 && slcan_buffer_index < 25) {
		slcan_buffer[slcan_buffer_index] = (uint8_t)uart_in;
		slcan_buffer_index++;
		} else if (slcan_buffer_index == 25) {
		slcan_buffer_index++;
		return 1;
	}
	return 0;
}