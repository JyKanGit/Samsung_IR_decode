#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sfr_defs.h>
#include <stdint.h>

#include "ir.h"
#include "pwm.h"

// ATtiny at 8MHz
#define FOSC 8000000

void init()
{
    sei();

    pwm_init();
    ir_init();
}

int main()
{
    init();
	while(1)
	{
        if (ir_new_message_received())
        {
            uint16_t data;
            uint16_t address;
            ir_decipher_msg(&data, &address);
            switch (data)
            {
                case ir_one:
                {
                    pwm_disable_breathing();
                    pwm_dimm_led();
                    break;
                }
                case ir_two:
                {
                    pwm_disable_breathing();
                    pwm_brighten_led();
                    break;
                }
                case ir_three:
                {
                    pwm_enable_breathing();
                    break;
                }
                case ir_four:
                {
                    pwm_led_off();
                    break;
                }
                case ir_five:
                {
                    pwm_led_on();
                    break;
                }
                default:
                {
                    break;
                }
            }
            ir_reset_msg();
        }
	}
}
