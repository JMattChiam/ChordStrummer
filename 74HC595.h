/*
Library for interfacing with 74HC595 8-bit Shift Registers
No reference timings for 3.3V logic on the datasheet so using VCC = 2V timing.
*/
#ifndef _74HC595_H
#define _74HC595_H

#include <stdint.h>

#define CLOCK_INTERVAL 200
#define LATCH_HOLD_INTERVAL 200
#define DATA_HOLD_INTERVAL 200

struct ShiftRegister_74HC595
{
    int serialData_pin;
    int clock_pin;
    int latch_pin;
};

//Constructor
struct ShiftRegister_74HC595 * newShiftRegister(int dataPin, int clockPin, int latchPin);

void shiftOutByte(struct ShiftRegister_74HC595* shift, uint8_t data);
void latchRegister(struct ShiftRegister_74HC595* shift);
void clock_signal(int clockPin);

#endif