#include <stdint.h>
#include <stdbool.h>

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

void ir_reset_msg();

bool ir_new_message_received();

void ir_init();
