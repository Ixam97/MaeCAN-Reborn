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
 *
 * For use with mcp2515_basic
 */

#ifndef SLCAN_H_
#define SLCAN_H_

#include "uart.h"
#include "mcp2515_basic.h"

extern const char asciilookup[16];

char slcan_buffer[25];
uint8_t slcan_buffer_index;

void sendSLCAN(canFrame *pFrame);

uint8_t decodeAscii(char character);

void getSLCAN(canFrame *frame);

uint8_t SLCANFrameReady(void);

#endif // SLCAN_H_