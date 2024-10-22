set -e

mkdir -p build

(
    cd build

    avr-gcc -Os -DF_CPU=8000000 -mmcu=attiny85 -c ../main.c -o main.o
    avr-gcc -Os -DF_CPU=8000000 -mmcu=attiny85 -c ../ir.c -o ir.o
    avr-gcc -Os -DF_CPU=8000000 -mmcu=attiny85 -c ../pwm.c -o pwm.o
    avr-gcc -DF_CPU=8000000 -mmcu=attiny85 -o main.elf main.o pwm.o ir.o
    avr-objcopy -O ihex main.elf main.hex
    avrdude -c stk500v1 -p attiny85 -P /dev/ttyUSB0 -b 19200 -U flash:w:main.hex
)