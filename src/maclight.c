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
	DDRB	|=	(1<<LED_IO) |		// LED debug pin
				(1<<DO);			// mark DO as output

	sei();
}

// ---------------------------------------------------------------------------------
// Toggles the LED pin
inline void ToggleLED(void)
{

	PINB |= (1<<LED_IO);
}

// ---------------------------------------------------------------------------------
// blah
void writePixel(const rgb_t * data)
{
	cli();

	uint8_t pinMask = (1<<DO);
	const volatile uint8_t
    	*port	= _SFR_IO_ADDR(PORTB);
	volatile uint8_t next, bit;
	volatile uint16_t
		i		= 3;				// Loop counter (3 bytes in every pixel)
	volatile uint8_t
		*ptr	= (uint8_t*)data,	// Pointer to next byte
		b		= *ptr++,			// Current byte value
		hi,							// PORT w/output bit set high
		lo,							// PORT w/output bit set low
		last	= PORTB;

	hi   = *port |  pinMask;		// Read PORTB and set the DO bit
	lo   = *port & ~pinMask;		// Read PORTB and clear the DO bit
	next = lo;
	bit  = 8;

	asm volatile
	(
	  "head20:"						"\n\t" // Clk  Pseudocode	(T =  0)
		"st   %a[port],  %[hi]"		"\n\t" // 2	PORT = hi	 (T =  2)
		"sbrc %[byte],  7"			"\n\t" // 1-2  if(b & 128)
		"mov  %[next], %[hi]"		"\n\t" // 0-1   next = hi	(T =  4)
		"dec  %[bit]"				"\n\t" // 1	bit--		 (T =  5)
		"st   %a[port],  %[next]"	"\n\t" // 2	PORT = next   (T =  7)
		"mov  %[next] ,  %[lo]"		"\n\t" // 1	next = lo	 (T =  8)
		"breq nextbyte20"			"\n\t" // 1-2  if(bit == 0) (from dec above)
		"rol  %[byte]"				"\n\t" // 1	b <<= 1	   (T = 10)
		"rjmp .+0"					"\n\t" // 2	nop nop	   (T = 12)
		"nop"						"\n\t" // 1	nop		   (T = 13)
		"st   %a[port],  %[lo]"		"\n\t" // 2	PORT = lo	 (T = 15)
		"nop"						"\n\t" // 1	nop		   (T = 16)
		"rjmp .+0"					"\n\t" // 2	nop nop	   (T = 18)
		"rjmp head20"				"\n\t" // 2	-> head20 (next bit out)
	  "nextbyte20:"					"\n\t" //					(T = 10)
		"ldi  %[bit]  ,  8"			"\n\t" // 1	bit = 8	   (T = 11)
		"ld   %[byte] ,  %a[ptr]+"	"\n\t" // 2	b = *ptr++	(T = 13)
		"st   %a[port], %[lo]"		"\n\t" // 2	PORT = lo	 (T = 15)
		"nop"						"\n\t" // 1	nop		   (T = 16)
		"sbiw %[count], 1"			"\n\t" // 2	i--		   (T = 18)
		"brne head20"				"\n"   // 2	if(i != 0) -> (next byte)

	  : [port]	"+e"		(port),
		[byte]	"+r"		(b),
		[bit]	"+r"		(bit),
		[next]	"+r"		(next),
		[count]	"+w"		(i)

	  : [ptr]	"e"			(ptr),
		[hi]	"r"			(hi),
		[lo]	"r"			(lo)
	);

	PORTB = last;

	sei();
}


// ---------------------------------------------------------------------------------
// Sends 4 x 3 bytes of data to the neopixels
void sendFrame(void)
{
	rgb_t pixel;
	pixel.red	= 0x20;
	pixel.green	= 0xAA;
	pixel.blue	= 0x03;

	writePixel(&pixel);
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
			ToggleLED();
			sendFrame();
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
