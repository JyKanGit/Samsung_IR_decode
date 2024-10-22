# Samsung IR remote decoding on AVR ATtiny85

Program for receiving Samsung IR remote signals on an ATtiny which is then used to control a led strip. The decoding of the signal is done by using interrupts on pin 4 in which an IR sensor is connected. To decode the signal, time between the edges is measured using the internal counter and from that the command is decoded. The value is then used to control a led with PWM. The IR sensor uses timer1 and the PWM led control uses timer0.

The Samsung IR protocol consists of the following:
- **Start Bit**: 4.5ms ON, 4.5ms OFF
- **Data Bits**: 32 bits (data 16 bits and address 16 bits)
  - Logical `1`: 590µs ON, 1690µs OFF
  - Logical `0`: 590µs ON, 590µs OFF
- **Stop Bit**: 590µs ON, 590µs OFF

The signal is reversed when capturing it with the IR sensor.

## Features
- Light interrupt handling
- Minimal global variables
- Easily adaptable to different AVR microcontrollers

## Setup, Compiling and programming the ATtiny using Arduino Uno as ISP

LED pin = PB0 <br />
IR sensor pin = PB4

Connect the Arduino to the ATtiny and run the `build.sh` script.