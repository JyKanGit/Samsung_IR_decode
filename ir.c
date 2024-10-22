#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sfr_defs.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ir.h"
#include "pwm.h"

#ifndef FOSC
#define FOSC 8000000
#endif

#define TIMER_PRESCALER 1024
#define TIMER_TICK_uS ((TIMER_PRESCALER * 1000000.0) / FOSC)

#define START_BIT_LENGTH (4500.0 / TIMER_TICK_uS)
#define START_BIT_TOLERANCE (1000.0 / TIMER_TICK_uS)
#define START_BIT_LLIMIT_TICKS ((int)(START_BIT_LENGTH - START_BIT_TOLERANCE))
#define START_BIT_HLIMIT_TICKS ((int)(START_BIT_LENGTH + START_BIT_TOLERANCE))

// if high time is >1200us it is identified as a one bit
#define ON_BIT_THRESHOLD ((int)(1200.0 / TIMER_TICK_uS))

// start bit + 32 data bits + end bit
#define MSG_LENGTH 2 + 32 * 2 + 1

volatile uint8_t ticks[MSG_LENGTH];
volatile uint8_t ir_tick_number;

enum
{
    StartBitLow  = 0,
    StarBitHigh,
    StopBit      = 66,
};

///////////////////////////////////////////////

//      Private functions

///////////////////////////////////////////////

bool ir_is_timer_overflow()
{
    return TIFR & _BV(TOV1);
}

void ir_clear_timer_overflow()
{
    TIFR |= _BV(TOV1);
}

void ir_reset_timer()
{
    if (ir_is_timer_overflow())
    {
        ir_clear_timer_overflow();
    }
    TCNT1 = 0;
}

uint8_t ir_read_timer()
{
    return TCNT1;
}

ISR(PCINT0_vect)
{
    // if the timer has overflown, the message is not captured correctly or
    // it is the start of the message. Either way it must be abandoned.
    if (ir_is_timer_overflow())
    {
        ir_reset_timer();
        ir_reset_msg();
        return;
    }

    uint8_t timer = ir_read_timer();
    if (ir_tick_number < MSG_LENGTH)
    {
        ticks[ir_tick_number] = timer;
    }
    ir_tick_number++;
    ir_reset_timer();
}

bool is_start_bit_valid(uint8_t ticks)
{
    return (START_BIT_LLIMIT_TICKS < ticks && ticks < START_BIT_HLIMIT_TICKS);
}

bool ir_message_valid()
{
    if (!is_start_bit_valid(ticks[StartBitLow])
        || !is_start_bit_valid(ticks[StarBitHigh]))
    {
        return false;
    }
    if (ticks[StopBit] > ON_BIT_THRESHOLD)
    {
        return false;
    }
    return true;
}

void ir_decipher_msg(uint16_t *data, uint16_t *address)
{
    uint32_t message = 0;
    int data_index = 0;
    int i = 3;
    // we are only interested in the UP times which are odd indices
    // data starts at the fourth indice
    for (i = 3; i < MSG_LENGTH; i += 2)
    {
        if (ticks[i] > ON_BIT_THRESHOLD)
        {
            message |= (uint32_t)(1UL << data_index);
        }
        data_index++;
    }

    *address = message;
    *data = message >> 16;
}

///////////////////////////////////////////////

//      Public functions

///////////////////////////////////////////////


bool ir_new_message_received()
{
    if (ir_tick_number < MSG_LENGTH)
    {
        return false;
    }

    if (!ir_message_valid())
    {
        ir_reset_msg();
        return false;
    }

    return true;
}

void ir_reset_msg()
{
    memset(ticks, 0, MSG_LENGTH * sizeof(uint8_t) - 1);
    ir_tick_number = 0;
}

void ir_init()
{
    // set pin as input
    DDRB &= ~_BV(PB4);

    // enable interrupts in pins 5..0
    GIMSK |= _BV(PCIE);
    // enable interrupt in pin 4
    PCMSK |= _BV(PCINT4);

    // set clk/1024
    TCCR1 |= _BV(CS13) | _BV(CS11) | _BV(CS10);
    sei();
}