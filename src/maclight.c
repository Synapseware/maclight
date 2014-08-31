#include "maclight.h"

volatile uint8_t	_tick		= 0;
volatile uint16_t	_delay		= 0;


// ---------------------------------------------------------------------------------
// Sets up Timer1 to count in 1MS ticks
void setupMSTimer(void)
{
	TCCR1	=	(1<<CTC1)	|		// Clear on OCR1C match
				(0<<PWM1A)	|
				(0<<COM1A1)	|
				(0<<COM1A0)	|
				(0<<CS13)	|		// CLK/64
				(1<<CS12)	|
				(1<<CS11)	|
				(1<<CS10);

	GTCCR	|=	(0<<PWM1B)	|
				(0<<COM1B1)	|
				(0<<COM1B0)	|
				(0<<FOC1B)	|
				(0<<FOC1A)	|
				(0<<PSR1);

	TIMSK	|=	(1<<OCIE1A)	|
				(0<<OCIE1B)	|
				(0<<TOIE1);

	PLLCSR	=	(0<<LSM)	|
				(0<<PCKE)	|
				(0<<PLLE)	|
				(0<<PLOCK);

	OCR1C	=	249;
}

// ---------------------------------------------------------------------------------
// Prepare the board
void setup(void)
{
	setupMSTimer();

	// setup the LED output port
	DDRB	|=	(1<<LED_IO); 		// LED debug pin

	sei();
}

// ---------------------------------------------------------------------------------
// Toggles the LED pin
inline void toggleLED(void)
{

	PINB |= (1<<LED_IO);
}

// ---------------------------------------------------------------------------------
// Test function to turn on a specific LED
void toggleMatrixLed(uint8_t index)
{
	if (index > CHALEY_PLEX_MATRIX_LEN - 1)
		return;

	uint8_t
		sel,
		dir,
		mtx		= pgm_read_byte(&CHALEY_PLEX_MATRIX[index]);

	sel = (mtx >> 4) & 0x0F;
	dir = mtx & 0x0F;

	DDRB	= dir | (DDRB & 0xF0);
	PORTB	= sel | (PORTB & 0xF0);
}

// ---------------------------------------------------------------------------------
// cycles through the 12 LEDs
void cycleMatrixLed(void)
{
	static uint8_t led = 0;
	toggleMatrixLed(led);
	if (led++ > CHALEY_PLEX_MATRIX_LEN - 1)
		led = 0;
}

// ---------------------------------------------------------------------------------
// Main
int main(void)
{
	setup();

	while(1)
	{
		// do events
		if (_tick)
		{
			toggleLED();

			cycleMatrixLed();
			
			_tick = 0;
		}
	}

	return 0;
}

// ---------------------------------------------------------------------------------
// Timer0 compare A interrupt handler
ISR(TIM1_COMPA_vect)
{
	if (_delay++ > DELAY - 1)
	{
		_delay = 0;
		_tick = 1;
	}
}
