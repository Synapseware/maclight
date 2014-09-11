#ifndef _MACLIGHT_H_
#define _MACLIGHT_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/delay.h>


// ---------------------------------------------------------------------------------
// PORTSB & D are used for output

typedef struct
{
	uint8_t		red;
	uint8_t		green;
	uint8_t		blue;
} rgb_t;


// 
#define	EVENTS_SEC		1000
#define DELAY			EVENTS_SEC/2

#define	DO				PB1
#define LED_IO          PB0

// neopixel constants
#define NEO_PIXEL_CLK	800000
#define NEO_BIT_CLK		20
#define NEO_BIT_0		13
#define NEO_BIT_1		6

#endif
