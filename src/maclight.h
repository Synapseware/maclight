#ifndef _MACLIGHT_H_
#define _MACLIGHT_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include <events/events.h>

// ---------------------------------------------------------------------------------
// PORTSB & D are used for output

#define EVENT_BASE		100
#define CLK_DIV			F_CPU/64/EVENT_BASE
#define META_FRAME_STEPS	16

#define LED_DDR			DDRB
#define	LED_PORT		PORTB
#define LED_PINS		PINB
#define LED_IO          PB0

#define LED_MTX_PRT		PORTC
#define LED_MTX_DDR		DDRC
#define LED_MTX_SIZE	12

// define the charley plex data for each LED
// 12 LEDs in total

// high nibble is LED selection, low nibble is port direction
const static uint8_t CHALEY_PLEX_MATRIX[] PROGMEM = {
    0b00010011,				// 1
    0b00100011,				// 2
    0b00100110,				// 3
    0b01000110,				// 4
    0b01001100,				// 5
    0b10001100,				// 6
    0b00010101,				// 7
    0b01000101,				// 8
    0b00101010,				// 9
    0b10001010,				// 10
    0b00011001,				// 11
    0b10001001				// 12
};

// 12 bytes of frame patterning
const static uint8_t FRAME_PATTERN_A[] PROGMEM = {
	256 * 0.25,
	256 * 0.125,
	256 * 0.25,
	256 * 0.50,
	256 * 0.75,
	256 * 0.50,
	256 * 0.25,
	256 * 0.125,
	256 * 0.25,
	256 * 0.50,
	256 * 0.75,
	256 * 0.50
};

// 12 bytes of frame patterning
const static uint8_t FRAME_PATTERN_B[] PROGMEM = {
	255 * 1.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00
};
// 12 bytes of frame patterning
const static uint8_t FRAME_PATTERN_OFF[] PROGMEM = {
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00,
	255 * 0.00
};


#endif
