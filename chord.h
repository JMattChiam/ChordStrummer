#ifndef _CHORD_H
#define _CHORD_H

#include <stdint.h>

#define SR_CLOCK_PIN 9
#define SR_LATCH_PIN 10
#define SR_SERIAL_PIN 11
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define MPR121_I2C_ADDR 0x5A
#define MPR121_I2C_FREQ 400000
#define MPR121_TOUCH_THRESHOLD 16
#define MPR121_RELEASE_THRESHOLD 10 
#define NOTE_DURATION 50
#define KEYS_SCAN_INTERVAL 1
#define TOUCH_SCAN_INTERVAL 1

//function declarations
void gpio_initialise(void);
void mpr121_initialise(void);
void midi_task(void);
void chord_select_task(void);
void updateChord(uint32_t noteKeypress, uint32_t qualityKeypress);
void update_leds();
uint8_t getMIDINote(int root, int string, int chordQuality);

//Chordnames
enum ChordQuality {
    MAJ   = 0,
    MIN   = 1,
    MAJ7  = 2,
    MIN7  = 3,
    DOM7  = 4,
    MAJ9  = 5,
    MIN9  = 6,
    DOM9  = 7,
    DIM   = 8,
    AUG   = 9
};


enum Notes {
    C   = 0,
    Cs  = 1,
    D   = 2,
    Ds  = 3,
    E   = 4,
    F   = 5,
    Fs  = 6,
    G   = 7,
    Gs  = 8,
    A   = 9,
    As  = 10, 
    B   = 11
};

uint8_t MIDINotes [] = {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47};

//Chord voicings
//Number of semitones from the root note
int8_t chordVoicings[10][12] = 
{
    {0, 7, 12, 16, 19, 24, 28, 31, 36, 40, 43, 48}, //MAJ 
    {0, 7, 12, 15, 19, 24, 27, 31, 36, 39, 43, 48}, //MIN
    {0, 7, 12, 16, 19, 23, 24, 31, 35, 36, 40, 47}, //MAJ7
    {0, 7, 12, 15, 19, 22, 24, 27, 34, 36, 39, 46}, //MIN7
    {0, 7, 12, 16, 19, 22, 24, 28, 34, 36, 40, 46}, //DOM7
    {0, 7, 12, 16, 23, 26, 28, 35, 36, 38, 47, 50}, //MAJ9
    {0, 7, 12, 15, 22, 26, 27, 34, 38, 39, 46, 50}, //MIN9
    {0, 7, 12, 16, 22, 26, 28, 34, 38, 40, 46, 50}, //DOM9
    {0, 6, 12, 15, 18, 24, 27, 30, 36, 39, 42, 48}, //DIM
    {0, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48}  //AUG
};

uint8_t rowPins[] = {28, 27, 26, 22, 21};
uint8_t columnPins[] = {18, 17, 16, 19, 20};

//Mappings for keypresses
#define KEYS_NOTES_MASK   0x00000fff
#define KEYS_QUALITY_MASK 0x003ff000
uint32_t keyMappings [12] = {0x00000001,0x00000020,0x00000002,0x00000040,0x00000004,0x00000008,0x00000080,0x00000010,0x00000100,0x00000400,0x00000200,0x00000800};
uint32_t qualityMappings[10] = {0x00001000,0x00020000,0x00002000,0x00040000,0x00008000,0x00004000,0x00080000,0x00010000,0x00100000,0x00200000};


//Mappings for LEDs
//Note that the LED data is shifted out serially to 3x 8-bit shift registers LSB first. Lower LED numbers (in the schematic) are at MSBs.
//data is shifted out in blocks of 32 bits. Lower 10 bits are unused.
uint32_t ledNotesMappings[12] = {0x80000000,0x40000000,0x20000000,0x10000000,0x08000000,0x04000000,0x02000000,0x01000000,0x00800000,0x00400000,0x00200000,0x00100000};
uint32_t ledQualityMappings[10] = {0x00040000,0x00080000,0x00020000,0x00000800,0x00002000,0x00010000,0x00000400,0x00001000,0x00008000,0x00004000};

#endif