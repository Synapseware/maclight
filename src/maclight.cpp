#include "maclight.h"

volatile uint8_t	_tick			= 0;
volatile uint16_t	_delay			= 0;
volatile uint8_t	_matrixOffset	= 0;
volatile uint8_t	_frameLed		= 0;
volatile uint8_t*	_pattern		= 0;
volatile uint8_t	_step			= 0;

volatile uint8_t	_frame[LED_MTX_SIZE];
volatile int8_t		_offsets[LED_MTX_SIZE];

// ---------------------------------------------------------------------------------
// Sets up Timer1 to count in 1MS ticks
void setupEvents(void)
{
	TCCR1A	=	(0<<COM1A1)	|
				(0<<COM1A0)	|
				(0<<COM1B1)	|
				(0<<COM1B0)	|
				(0<<WGM11)	|
				(0<<WGM10);

	TCCR1B	=	(0<<ICNC1)	|
				(0<<ICES1)	|
				(0<<WGM13)	|
				(1<<WGM12)	|	// CTC
				(0<<CS12)	|
				(1<<CS11)	|	// clk/64/10 = 20MHz/64/10 = 31250
				(1<<CS10);

	TCCR1C	=	(0<<FOC1A)	|
				(0<<FOC1B);

	TIMSK1	=	(0<<ICIE1)	|
				(0<<OCIE1B)	|
				(1<<OCIE1A)	|	// Compare A int
				(0<<TOIE1);

	OCR1A	=	CLK_DIV - 1;

	setTimeBase(EVENT_BASE);
}

// ---------------------------------------------------------------------------------
// Prepare the board
void setup(void)
{
	setupEvents();

	// setup the LED output port
	LED_DDR	|=	(1<<LED_IO); 		// LED debug pin

	sei();
}

// ---------------------------------------------------------------------------------
// Toggles the LED pin
void toggleLed(eventState_t state)
{

	LED_PINS |= (1<<LED_IO);
}

// ---------------------------------------------------------------------------------
// Returns the active pixel from the current frame index
inline uint8_t getPixelOffset(uint8_t fromFrameIndex)
{
	uint8_t result = fromFrameIndex + _matrixOffset;
	if (result > LED_MTX_SIZE - 1)
		result -= LED_MTX_SIZE;

	return result;
}

// ---------------------------------------------------------------------------------
// Refreshes the charley plex LED frame
void matrixRefreshFrame(void)
{
	cli();

	static uint8_t
		brt			= 0,
		i			= 0,
		sel			= 0,
		dir			= 0,
		mtx			= 0,
		ddr			= 0,
		prt			= 0;

	ddr		= LED_MTX_DDR & 0xF0;
	prt		= LED_MTX_PRT & 0xF0;

	// cycle through the frame positions
	for (i = 0; i < LED_MTX_SIZE; i++)
	{
		// read the CPLEX pattern for the specific index
		mtx = pgm_read_byte(&CHALEY_PLEX_MATRIX[i]);
		sel = (mtx >> 4) & 0x0F;
		dir = mtx & 0x0F;

		// determine if this pixel in the frame should be on,
		// based on a brightness comparison
		if (_frame[i] > 0 && _frame[i] <= brt)
		{
			LED_MTX_DDR	= dir | (ddr & 0xF0);
			LED_MTX_PRT	= sel | (prt & 0xF0);
		}
	}

	// shut off matrix after frame refresh
	LED_MTX_DDR	= ddr;
	LED_MTX_PRT = prt;

	// move to the next brightness level
	brt += 4;
	sei();
}

// ---------------------------------------------------------------------------------
// Ensures that the value does not exceed a specified maximum
inline uint8_t limitIndex(uint8_t index)
{
	if (index > LED_MTX_SIZE - 1)
		index = 0;

	return index;
}

// ---------------------------------------------------------------------------------
// Computes the offsets for each frame from the current frame
void calcFrameDiffs(void)
{
	// compute the offsets for each frame cell
	// offsets are computed based on cell+1 migration

	uint8_t
		offset	= 1,
		index	= 0;
	int8_t
		diff	= 0;

	offset = limitIndex(_matrixOffset + 1);
	for (index = 0; index < LED_MTX_SIZE; index++)
	{
		diff = (pgm_read_byte(&_pattern[offset]) - _frame[index]) / META_FRAME_STEPS;

		// save the delta
		_offsets[index] = diff;

		offset = limitIndex(offset++);
	}
}

// ---------------------------------------------------------------------------------
// Moves the offset to the next position
void nextFrame(eventState_t state)
{
	// reset the step counter
	_step = 0;

	// adjust the offset
	_matrixOffset++;
	if (_matrixOffset > LED_MTX_SIZE - 1)
		_matrixOffset = 0;

	calcFrameDiffs();
}

// ---------------------------------------------------------------------------------
// sets the frame pattern and does the one-time delta calculations
void setFramePattern(const uint8_t * pframe)
{
	// copy the pointer
	_pattern = (volatile uint8_t*) pframe;

	calcFrameDiffs();
}

// ---------------------------------------------------------------------------------
// updates the frame so it can reach the desired pattern by adding the difference
// value to each frame cell
void updateFrame(eventState_t state)
{
	if (_step > LED_MTX_SIZE - 1)
	{
		// quit if we've reached our step count
		return;
	}

	// increment the step pointer
	_step++;

	uint8_t
		frame		= 0,
		pval		= 0,
		index		= 0;
	int8_t
		diff		= 0;

	frame = _matrixOffset;
	for (index = 0; index < LED_MTX_SIZE; index++)
	{
		pval = pgm_read_byte(&_pattern[frame]);

		// get the pattern value we're trying to reach
		if (_frame[index] != pval)
			_frame[index] += _offsets[index];

		frame = limitIndex(frame++);
	}
}

// ---------------------------------------------------------------------------------
// Main
int main(void)
{
	setup();

	setFramePattern(FRAME_PATTERN_A);

	// blink the debug LED @ 1Hz
	registerEvent(toggleLed, EVENT_BASE / 2, 0);

	// number of times to move 
	registerEvent(updateFrame, EVENT_BASE / (META_FRAME_STEPS * 2), 0);

	// toggle an frame cycle step
	registerEvent(nextFrame, EVENT_BASE, 0);

	while(1)
	{
		// always refresh the frame
		matrixRefreshFrame();

		// do events
		eventsDoEvents();
	}

	return 0;
}

// ---------------------------------------------------------------------------------
// Timer0 compare A interrupt handler
ISR(TIMER1_COMPA_vect)
{
	// mark event sync
	eventSync();
}
