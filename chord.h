#ifndef _CHORD_H
#define _CHORD_H

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
#define KEYS_SCAN_INTERVAL 5

//function declarations
void gpio_initialise(void);
void mpr121_initialise(void);
void midi_task(void);
void chord_select_task(void);
void update_leds();
uint8_t getMIDINote(int root, int string, int chordQuality);

//Chordnames
enum ChordQuality {
    MAJ   = 0,
    MIN   = 1,
    MAJ7  = 2,
    MIN7  = 3,
    DOM7    = 4,
    MAJ9  = 5,
    MIN9  = 6,
    DOM9    = 7,
    DIM     = 8,
    AUG     = 9
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

uint8_t MIDINotes [] = {36, 37, 38, 39, 40, 41, 42, 30, 31, 32, 33, 35};

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
#define KEYS_NOTES_MASK   0x01ff8c00
#define KEYS_QUALITY_MASK 0x000073e3
#define KEY_C       0x00100000 
#define KEY_Cs      0x00008000
#define KEY_D       0x00200000
#define KEY_Ds      0x00010000
#define KEY_E       0x00400000
#define KEY_F       0x00800000
#define KEY_Fs      0x00020000
#define KEY_G       0x01000000
#define KEY_Gs      0x00040000
#define KEY_A       0x00000400
#define KEY_As      0x00080000
#define KEY_B       0x00000800
#define KEY_MAJ     0x00001000
#define KEY_MAJ7    0x00002000
#define KEY_MAJ9    0x00004000
#define KEY_MIN     0x00000080
#define KEY_MIN7    0x00000100
#define KEY_MIN9    0x00000200
#define KEY_DOM7    0x00000020
#define KEY_DOM9    0x00000040
#define KEY_DIM     0x00000001
#define KEY_AUG     0x00000002

//Mappings for LEDs
//Note that the LED data is shifted out serially to 3x 8-bit shift registers LSB first. Lower LED numbers (in the schematic) are at MSBs.
//data is shifted out in blocks of 32 bits. Lower 10 bits are unused.
#define LED_C       0x80000000
#define LED_Cs      0x40000000
#define LED_D       0x20000000
#define LED_Ds      0x10000000
#define LED_E       0x08000000
#define LED_F       0x04000000
#define LED_Fs      0x02000000
#define LED_G       0x01000000
#define LED_Gs      0x00800000
#define LED_A       0x00400000
#define LED_As      0x00200000
#define LED_B       0x00100000
#define LED_MAJ     0x00080000
#define LED_MAJ7    0x00040000
#define LED_MAJ9    0x00020000
#define LED_MIN     0x00010000
#define LED_MIN7    0x00008000
#define LED_MIN9    0x00004000
#define LED_DOM7    0x00002000
#define LED_DOM9    0x00001000
#define LED_DIM     0x00000800
#define LED_AUG     0x00000400

#endif