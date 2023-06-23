/*
Library for interfacing with 74HC595 8-bit Shift Registers
No reference timings for 3.3V logic on the datasheet so using VCC = 2V timing.

clock interval of 25us produces a frequency of 20kHz. 1 byte will take 400us to shift out
*/
#ifndef _74HC595_H
#define _74HC595_H

#include <stdint.h>

#define CLOCK_INTERVAL 25
#define LATCH_INTERVAL 25

typedef struct ShiftRegister_74HC595
{
    int serialData_pin;
    int clock_pin;
    int latch_pin;
} ShiftRegister_74HC595_t;

void ShiftRegister_74HC595_init(struct ShiftRegister_74HC595* shift, int dataPin, int clockPin, int latchPin);
void shiftOutByte(struct ShiftRegister_74HC595* shift, uint8_t data);
void latchRegister(struct ShiftRegister_74HC595* shift);
void clock_signal(int clockPin);

#endif