#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sfr_defs.h>
#include <stdint.h>
#include <stdbool.h>

#include "pwm.h"

uint8_t rising = 1;

///////////////////////////////////////////////

//      Private functions

///////////////////////////////////////////////

ISR(TIMER0_OVF_vect)
{
    if (rising)
    {
        if (++OCR0A == UINT8_MAX)
        {
            rising = 0;
        }
    }
    else
    {
        if (--OCR0A == 0)
        {
            rising = 1;
        }
    }
}

///////////////////////////////////////////////

//      Public functions

///////////////////////////////////////////////

void pwm_brighten_led()
{
    if (OCR0A + 20 > UINT8_MAX)
    {
        OCR0A = UINT8_MAX;
        return;
    }
    OCR0A = OCR0A + 20;
}

void pwm_dimm_led()
{
    if (OCR0A - 20 < 0)
    {
        OCR0A = 0;
        return;
    }
    OCR0A = OCR0A - 20;
}

void pwm_led_on()
{
    DDRB |= _BV(PB0);
}

void pwm_led_off()
{
    DDRB &= ~_BV(PB0);
}

void pwm_led_toggle()
{
    DDRB ^= _BV(PB0);
}

void pwm_enable_breathing()
{
    // enable timer overflow interrupt
    TIMSK |= _BV(TOIE0);
}

void pwm_disable_breathing()
{
    // disable timer overflow interrupt
    TIMSK &= ~_BV(TOIE0);
}

void pwm_init()
{
    // pwm pin is PB0, OC0A
    DDRB |= _BV(PB0);

    // start with 50% duty cycle
    OCR0A |= UINT8_MAX/2;

    // set fast PWM
    // Clear OCR0A on compare match, set OC0A at BOTTOM, (non-inverting mode).
    TCCR0A |= _BV(COM0A1) | _BV(WGM02) | _BV(WGM01) | _BV(WGM00);

    // set clk/256
    TCCR0B |= _BV(CS02);
}