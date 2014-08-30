#include "maclight.h"



volatile uint8_t _tick = 0;



// ---------------------------------------------------------------------------------
// Configures timer0 on a 1KHz interrupt cycle
void setupTimer0(void)
{
    TCCR0A  =   (0<<COM0A1) |
                (0<<COM0A0) |
                (0<<COM0B1) |
                (0<<COM0B0) |
                (1<<WGM01)  |       // ctc
                (0<<WGM00);

    TCCR0B  =   (0<<FOC0A)  |
                (0<<FOC0B)  |
                (0<<WGM02)  |
                (0<<CS02)   |
                (0<<CS01)   |       // clk/64 (16MHz/64 = 250KHz / 250 = 1KHz)
                (1<<CS00);

    TIMSK   |=  (1<<OCIE0A) |
                (1<<OCIE0B) |
                (0<<TOIE0);

    OCR0A   =   (250 - 1);
    OCR0B   =   (250 / 2) - 1;
}

// ---------------------------------------------------------------------------------
// Prepare the board
void init(void)
{
    setupTimer0();

    // setup the LED output port
    DDRB |= (1<<PB4);

    sei();
}

// ---------------------------------------------------------------------------------
// Toggles the LED pin
void ToggleLED(void)
{
    PINB |= (1<<PB4);
}

// ---------------------------------------------------------------------------------
// Main
int main(void)
{
    init();

    while(1)
    {
        // do events
        if (_tick)
        {
            ToggleLED();
            _tick = 0;
        }
    }

    return 0;
}

// ---------------------------------------------------------------------------------
// Timer0 compare A interrupt handler
ISR(TIM0_COMPA_vect)
{
    //_tick = 1;
}

ISR(TIM0_COMPB_vect)
{
    _tick = 1;
}