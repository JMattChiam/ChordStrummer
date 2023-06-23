#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "74HC595.h"
#include <stdlib.h>

void ShiftRegister_74HC595_init(struct ShiftRegister_74HC595* shift, int dataPin, int clockPin, int latchPin)
{
    shift->serialData_pin = dataPin;
    shift->clock_pin = clockPin;
    shift->latch_pin = latchPin;

    gpio_init(dataPin);
    gpio_init(clockPin);
    gpio_init(latchPin);
    gpio_set_dir(dataPin, GPIO_OUT);
    gpio_set_dir(clockPin, GPIO_OUT);
    gpio_set_dir(latchPin, GPIO_OUT);
}

void clock_signal(int clockPin)
{
    gpio_put(clockPin, 1);
    sleep_us(CLOCK_INTERVAL);
    gpio_put(clockPin, 0);
    sleep_us(CLOCK_INTERVAL);
}

void latchRegister(struct ShiftRegister_74HC595* shift)
{
    gpio_put(shift->latch_pin, 1);
    sleep_us(LATCH_INTERVAL);
    gpio_put(shift->latch_pin, 0);
}

void shiftOutByte(struct ShiftRegister_74HC595* shift, uint8_t data)
{
    uint8_t bit;
    for (int i = 0; i < 8; i++)
    {
        bit = (data >> i) & 0x01;
        gpio_put(shift->serialData_pin, bit);
        clock_signal(shift->clock_pin);
    }
}