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


#define EPS				400000
#define CLK_DIV         F_CPU/1/EPS
#define DELAY			500

#define	DO				PB1
#define LED_IO          PB0

#endif
