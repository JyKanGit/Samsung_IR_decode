#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sfr_defs.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define FOSC 16000000            // Clock Speed
#define TIMER_PRESCALER 1024
#define TIMER_TICK_uS ((TIMER_PRESCALER * 1000000.0) / FOSC)

#define START_BIT_LENGTH (4500.0 / TIMER_TICK_uS)
#define START_BIT_TOLERANCE (500.0 / TIMER_TICK_uS)
#define START_BIT_LLIMIT_TICKS ((int)(START_BIT_LENGTH - START_BIT_TOLERANCE))
#define START_BIT_HLIMIT_TICKS ((int)(START_BIT_LENGTH + START_BIT_TOLERANCE))

#define ON_BIT_THRESHOLD ((int)(1200.0 / TIMER_TICK_uS))

// start bit + 32 data bits + end bit
#define MSG_LENGTH 2 + 32 * 2 + 1

volatile uint8_t ticks[MSG_LENGTH];
volatile uint8_t tick_number;

enum
{
    ir_one   = 0xFB04,
    ir_two   = 0xFA05,
    ir_three = 0xF906,
    ir_four  = 0xF708,
    ir_five  = 0xF609,
    ir_six   = 0xF50A,
    ir_seven = 0xF30C,
    ir_eight = 0xF20D,
    ir_nine  = 0xF10E,
    ir_zero  = 0xEE11,
};

enum
{
    StartBitLow  = 0,
    StarBitHigh,
    StopBit      = 66,
};

bool is_timer_overflow()
{
    return TIFR0 & _BV(TOV0);
}

void clear_timer_overflow()
{
    TIFR0 |= _BV(TOV0);
}

void reset_timer0()
{
    if (is_timer_overflow())
    {
        clear_timer_overflow();
    }
    TCNT0 = 0;
}

uint8_t read_timer()
{
    return TCNT0;
}

void reset_msg()
{
    memset(ticks, 0, MSG_LENGTH * sizeof(unsigned int));
    tick_number = 0;
}

ISR(PCINT0_vect)
{
    // if the timer has overflown, the message is not captured correctly or
    // it is the start of the message. Either way it must be abandoned.
    if (is_timer_overflow())
    {
        reset_timer0();
        reset_msg();
        return;
    }

    uint32_t timer = read_timer();
    if (tick_number < MSG_LENGTH)
    {
        ticks[tick_number] = timer;
    }
    tick_number++;
    reset_timer0();
}

bool is_start_bit_valid(uint8_t ticks)
{
    return (START_BIT_LLIMIT_TICKS < ticks && ticks < START_BIT_HLIMIT_TICKS);
}

bool message_sane()
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

void decipher_message(uint16_t *data, uint16_t *address)
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

void init_clock()
{
    // set clk/1024
    TCCR0B |= _BV(CS02) | _BV(CS00);
}

void init_pins()
{
    // set PB0 as input
    DDRB &= ~_BV(PB0);

    // enable interrupts in pins 7..0
    PCICR |= _BV(PCIE0);
    PCMSK0 |= _BV(PCINT0);

}

void dimm_led()
{
    if (OCR2A + 20 > UINT8_MAX)
    {
        OCR2A = UINT8_MAX;
        return;
    }
    OCR2A = OCR2A + 20;
}

void brighten_led()
{
    if (OCR2A - 20 < 0)
    {
        OCR2A = 0;
        return;
    }
    OCR2A = OCR2A - 20;
}

void led_on()
{
    DDRB |= _BV(DDB3);
}

void led_off()
{
    DDRB &= ~_BV(DDB3);
}

void enable_breathing()
{
    // enable timer overflow interrupt
    TIMSK2 |= _BV(TOIE2);
}

void disable_breathing()
{
    // disable timer overflow interrupt
    TIMSK2 &= ~_BV(TOIE2);
}

uint8_t rising = 1;
ISR(TIMER2_OVF_vect)
{
    if (rising)
    {
        if (++OCR2A == UINT8_MAX)
        {
            rising = 0;
        }
    }
    else
    {
        if (--OCR2A == 0)
        {
            rising = 1;
        }
    }
}

void init_pwm_timer2()
{
    // PINB3 is the OC2A pin
    DDRB |= _BV(DDB3);

    // start with 50% duty cycle
    OCR2A |= 128;

    // set fast PWM
    // Clear OC2A on compare match, set OC2A at BOTTOM, (non-inverting mode).
    TCCR2A |= _BV(COM2A1) | _BV(WGM20) | _BV(WGM21) | _BV(WGM20);

    // set clk/64
    TCCR2B |= _BV(CS22) | _BV(CS21) | _BV(CS20);

}

void init()
{
    init_clock();
    init_pins();
    init_pwm_timer2();

    sei();
}

int main()
{
    init();
	while(true)
	{
        if (tick_number >= MSG_LENGTH)
        {
            if (message_sane())
            {
                uint16_t data;
                uint16_t address;
                decipher_message(&data, &address);
                switch (data)
                {
                    case ir_one:
                    {
                        disable_breathing();
                        dimm_led();
                        break;
                    }
                    case ir_two:
                    {
                        disable_breathing();
                        brighten_led();
                        break;
                    }
                    case ir_three:
                    {
                        enable_breathing();
                        break;
                    }
                    case ir_four:
                    {
                        led_off();
                        break;
                    }
                    case ir_five:
                    {
                        led_on();
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            reset_msg();
        }
	}
}
