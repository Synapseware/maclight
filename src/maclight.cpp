#include "maclight.h"

volatile	uint8_t		_tick			= 0;
volatile	uint16_t	_delay			= 0;
volatile	uint8_t		_matrixOffset	= 0;
volatile	uint8_t		_frameLed		= 0;
volatile	uint8_t*	_pattern		= 0;
volatile	uint8_t		_step			= 0;

volatile	uint8_t		_frame[LED_MTX_SIZE];
volatile	uint8_t		_diff[LED_MTX_SIZE];

static		Events		_events(MAX_EVENT_RECORDS);


// ---------------------------------------------------------------------------------
// Sets up Timer1 to count in 1MS ticks
static void setupEvents(void)
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

	_events.setTimeBase(EVENT_BASE);
}

// ---------------------------------------------------------------------------------
// Prepare the board
static void setup(void)
{
	setupEvents();

	// setup the LED output port
	LED_DDR	|=	(1<<LED_IO); 		// LED debug pin

	sei();
}

// ---------------------------------------------------------------------------------
// Toggles the LED pin
static void toggleLed(eventState_t state)
{

	LED_PINS |= (1<<LED_IO);
}

// ---------------------------------------------------------------------------------
// Returns the active pixel from the current frame index
inline uint8_t getPixelOffset(uint8_t frameIndex)
{
	uint8_t result = frameIndex + _matrixOffset;
	if (result > LED_MTX_SIZE - 1)
		result -= LED_MTX_SIZE;

	return result;
}

// ---------------------------------------------------------------------------------
// Refreshes the charley plex LED frame
static void matrixRefreshFrame(void)
{
	cli();

	static uint8_t
		brt			= 0;
	uint8_t
		sel			= 0,
		dir			= 0,
		mtx			= 0,
		ddr			= 0,
		prt			= 0,
		val			= 0;

	ddr		= LED_MTX_DDR & 0xF0;
	prt		= LED_MTX_PRT & 0xF0;

	// cycle through the frame positions
	for (uint8_t i = 0; i < LED_MTX_SIZE; i++)
	{
		// read the CPLEX pattern for the specific index
		mtx = pgm_read_byte(&CHALEY_PLEX_MATRIX[i]);
		sel = (mtx / 16) & 0x0F;
		dir = mtx & 0x0F;
		val = _frame[i];

		// determine if this pixel in the frame should be on,
		// based on a brightness comparison
		if (brt < val)
		{
			LED_MTX_PRT	= sel | (prt & 0xF0);
			LED_MTX_DDR	= dir | (ddr & 0xF0);
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
// Computes the differences required to move between frames in the specified pattern
static void calcFrameDifferences(void)
{
	uint8_t
		pval		= 0,
		fval		= 0,
		diff		= 0;

	for (uint8_t index = 0; index < LED_MTX_SIZE; index++)
	{
		pval = pgm_read_byte(&_pattern[getPixelOffset(index)]);
		fval = _frame[index];

		if (pval > fval)
			diff = (pval - fval) / META_FRAME_STEPS;
		else if (fval > pval)
			diff = (fval - pval) / META_FRAME_STEPS;
		else
			diff = 0;

		// store the magnitude as a difference
		_diff[index] = diff;
	}
}

// ---------------------------------------------------------------------------------
// Moves the offset to the next position
static void nextFrame(eventState_t state)
{
	// reset the step counter
	_step = 0;

	// adjust the offset
	_matrixOffset++;
	if (_matrixOffset > LED_MTX_SIZE - 1)
		_matrixOffset = 0;

	calcFrameDifferences();
}

// ---------------------------------------------------------------------------------
// sets the frame pattern and does the one-time delta calculations
static void setFramePattern(const uint8_t * pframe)
{
	// copy the pointer
	_pattern = (volatile uint8_t*) pframe;

	calcFrameDifferences();
}

// ---------------------------------------------------------------------------------
// updates the frame so it can reach the desired pattern by adding the difference
// value to each frame cell
static void aliasFrame(eventState_t state)
{
	if (_step >= META_FRAME_STEPS - 1)
	{
		// quit if we've reached our step count
		return;
	}

	// increment the step pointer
	_step++;

	uint8_t
		pval		= 0,
		fval		= 0,
		index		= 0;

	for (index = 0; index < LED_MTX_SIZE; index++)
	{
		pval = pgm_read_byte(&_pattern[getPixelOffset(index)]);
		fval = _frame[index];

		if (fval > pval)
			fval -= _diff[index];
		else if (pval > fval)
			fval += _diff[index];

		_frame[index] = fval;
	}
}

// ---------------------------------------------------------------------------------
// Changes between some different patterns
void changePattern(eventState_t state)
{
	static uint8_t next = 0;
	switch (next)
	{
		case 0:
			setFramePattern(FRAME_PATTERN_A);
			next++;
			break;
		case 1:
			setFramePattern(FRAME_PATTERN_B);
			next++;
			break;
		case 2:
			setFramePattern(FRAME_PATTERN_C);
			next = 0;
			break;
		case 3:
			setFramePattern(FRAME_PATTERN_D);
			next = 0;
			break;
	}
}

// ---------------------------------------------------------------------------------
// Main
int main(void)
{
	setup();

	setFramePattern(FRAME_PATTERN_B);

	// blink the debug LED @ 1Hz
	_events.registerEvent(toggleLed, EVENT_BASE / 2, 0);

	// number of times to move 
	_events.registerEvent(aliasFrame, EVENT_BASE / META_FRAME_STEPS / FRAME_SPEED, 0);

	// toggle an frame cycle step
	_events.registerEvent(nextFrame, EVENT_BASE / FRAME_SPEED, 0);

	// toggle an frame cycle step
	_events.registerEvent(changePattern, EVENT_BASE * 10, 0);

	while(1)
	{
		// always refresh the frame
		matrixRefreshFrame();

		// do events
		_events.doEvents();
	}

	return 0;
}

// ---------------------------------------------------------------------------------
// Timer0 compare A interrupt handler
ISR(TIMER1_COMPA_vect)
{
	// mark event sync
	_events.sync();
}
