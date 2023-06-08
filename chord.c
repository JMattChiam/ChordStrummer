#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "mpr121.h"

#include "bsp/board.h"
#include "tusb.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define MPR121_I2C_ADDR 0x5A
#define MPR121_I2C_FREQ 400000

#define MPR121_TOUCH_THRESHOLD 16
#define MPR121_RELEASE_THRESHOLD 10
//Chordnames
enum ChordQuality {
    MAJOR   = 0,
    MINOR   = 1,
    MAJOR7  = 2,
    MINOR7  = 3,
    DOM7    = 4,
    MAJOR9  = 5,
    MINOR9  = 6,
    DOM9    = 7,
    DIM     = 8,
    AUG     = 9
};

//Note numbers are the MIDI note numbers starting from 36 for C1
enum Notes {
    C   = 0,
    Db  = 1,
    D   = 2,
    Eb  = 3,
    E   = 4,
    F   = 5,
    Gb  = 6,
    G   = 7,
    Ab  = 8,
    A   = 9,
    Bb  = 10, 
    B   = 11
};

uint8_t ChordButtonPins [] = {16, 17, 18, 19, 20, 21, 22, 26, 27, 28};
uint8_t NoteButtonPins [] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
uint8_t MIDINotes [] = {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 33, 35};

//Formulae for how to form chords
//Number of semitones from the root note
int8_t chordFormulas[10][12] = 
{
    {0, 7, 12, 16, 19, 24, 28, 31, 36, 40, 43, 48}, //MAJ
    {0, 7, 12, 15, 19, 24, 27, 31, 36, 39, 43, 48}, //MIN
    {0, 7, 12, 16, 19, 23, 24, 28, 31, 35, 36, 43}, //MAJ7
    {0, 7, 12, 15, 19, 22, 24, 27, 31, 34, 36, 43}, //MIN7
    {0, 7, 12, 16, 19, 22, 24, 28, 31, 34, 36, 43}, //DOM7
    {0, 7, 12, 19, 23, 24, 26, 28, 35, 36, 38, 43}, //MAJ9
    {0, 7, 12, 19, 22, 24, 26, 28, 34, 36, 38, 43}, //MIN9
    {0, 7, 12, 19, 22, 24, 26, 27, 34, 38, 39, 43}, //DOM9
    {0, 6, 12, 15, 18, 24, 27, 30, 36, 39, 43, 48}, //DIM
    {0, 8, 12, 16, 19, 24, 28, 32, 36, 40, 44, 48}  //AUG
};

//function declarations
void gpio_initialise(void);
void mpr121_initialise(void);
void midi_task(void);
void chord_select_task(void);                     
uint8_t getNote(int root, int string);

static uint8_t rootNote;
static enum ChordQuality chordQuality;
static struct mpr121_sensor mpr121;
static uint16_t plucked = 0x0000;
static uint16_t previousTouch = 0x0000;
static uint16_t currentTouch = 0x0000; 
static uint32_t noteTimers [12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t previousNote [12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const uint16_t noteDuration = 250;

int main() 
{
    board_init();
    tusb_init();
    gpio_initialise();

    i2c_init(I2C_PORT, MPR121_I2C_FREQ);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Initialise and autoconfigure the touch sensor.
    mpr121_init(I2C_PORT, MPR121_I2C_ADDR, &mpr121);
    mpr121_set_thresholds(MPR121_TOUCH_THRESHOLD,
                          MPR121_RELEASE_THRESHOLD, &mpr121);


    chordQuality = MAJOR;
    rootNote = MIDINotes[0];

    while (1) 
    {
        tud_task();
        chord_select_task();
        midi_task();
    }
}

//Detect chord quality from pushbuttons
//To implement faster algorithm
void chord_select_task()
{   
    for (int i = 0; i < 10; i++)
    {
        if (gpio_get(ChordButtonPins[i]) != 0) 
        {
            chordQuality = i;
        }
    }
    for (int i = 0; i < 12; i++)
    {
        if (gpio_get(NoteButtonPins[i]) != 0)
        {
            rootNote = MIDINotes[i];
        }
    }
}

void midi_task()
{
         
    uint8_t const cable_num = 0;
    uint8_t const channel = 0;
    uint8_t note;

    mpr121_touched(&currentTouch, &mpr121);

    //Figure out which "strings" have been played
    if (currentTouch > 0)
    {
        gpio_put(25, 1);
        for (int electrode = 0; electrode < 12; electrode++)
        {
            if (((previousTouch >> electrode) & 1) == 0 && ((currentTouch >> electrode) & 1) == 1)
            {
                note = getNote(rootNote, electrode);
                uint8_t note_on[3] = { 0x90 | channel, note, 127 };
                tud_midi_stream_write(cable_num, note_on, 3);
                noteTimers[electrode] = board_millis();
                previousNote[electrode] = note;

            }
            
        }
    }
    else
    {
        gpio_put(25, 0);
    }

    //Turn off notes after duration has elasped
    for (int i = 0; i < 12; i++)
    {
        if ((board_millis() - noteTimers[i]) > noteDuration && (noteTimers[i] != 0))
            {
                uint8_t note_off[3] = { 0x80 | channel, previousNote[i], 0};
                tud_midi_stream_write(cable_num, note_off, 3);
                noteTimers[i] = 0;
                previousNote[i] = 0;
            }
    }
    previousTouch = currentTouch;
}

//Returns the MIDI Note number to play
uint8_t getNote(int root, int string)
{
    uint8_t note;
    note = root + chordFormulas[chordQuality][string];
    return note;
}

void gpio_initialise()
{
    gpio_init(NoteButtonPins[0]);
    gpio_init(NoteButtonPins[1]);
    gpio_init(NoteButtonPins[2]);
    gpio_init(NoteButtonPins[3]);
    gpio_init(NoteButtonPins[4]);
    gpio_init(NoteButtonPins[5]);
    gpio_init(NoteButtonPins[6]);
    gpio_init(NoteButtonPins[7]);
    gpio_init(NoteButtonPins[8]);
    gpio_init(NoteButtonPins[9]);
    gpio_init(NoteButtonPins[10]);
    gpio_init(NoteButtonPins[11]);
    gpio_init(ChordButtonPins[0]);
    gpio_init(ChordButtonPins[1]);
    gpio_init(ChordButtonPins[2]);
    gpio_init(ChordButtonPins[3]);
    gpio_init(ChordButtonPins[4]);
    gpio_init(ChordButtonPins[5]);
    gpio_init(ChordButtonPins[6]);
    gpio_init(ChordButtonPins[7]);
    gpio_init(ChordButtonPins[8]);
    gpio_init(ChordButtonPins[9]);

    gpio_set_dir(NoteButtonPins[0], GPIO_IN);
    gpio_set_dir(NoteButtonPins[1], GPIO_IN);
    gpio_set_dir(NoteButtonPins[2], GPIO_IN);
    gpio_set_dir(NoteButtonPins[3], GPIO_IN);
    gpio_set_dir(NoteButtonPins[4], GPIO_IN);
    gpio_set_dir(NoteButtonPins[5], GPIO_IN);
    gpio_set_dir(NoteButtonPins[6], GPIO_IN);
    gpio_set_dir(NoteButtonPins[7], GPIO_IN);
    gpio_set_dir(NoteButtonPins[8], GPIO_IN);
    gpio_set_dir(NoteButtonPins[9], GPIO_IN);
    gpio_set_dir(NoteButtonPins[10],GPIO_IN);
    gpio_set_dir(NoteButtonPins[11], GPIO_IN);
    gpio_set_dir(ChordButtonPins[0], GPIO_IN);
    gpio_set_dir(ChordButtonPins[1], GPIO_IN);
    gpio_set_dir(ChordButtonPins[2], GPIO_IN);
    gpio_set_dir(ChordButtonPins[3], GPIO_IN);
    gpio_set_dir(ChordButtonPins[4], GPIO_IN);
    gpio_set_dir(ChordButtonPins[5], GPIO_IN);
    gpio_set_dir(ChordButtonPins[6], GPIO_IN);
    gpio_set_dir(ChordButtonPins[7], GPIO_IN);
    gpio_set_dir(ChordButtonPins[8], GPIO_IN);
    gpio_set_dir(ChordButtonPins[9], GPIO_IN);
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {}

// Invoked when device is unmounted
void tud_umount_cb(void) {}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {}
