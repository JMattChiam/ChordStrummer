#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "mpr121.h"
#include "chord.h"
#include "bsp/board.h"
#include "tusb.h"

static uint8_t rootNote;
static enum ChordQuality chordQuality;
static struct mpr121_sensor mpr121;
static uint16_t plucked = 0x0000;
static uint16_t previousTouch = 0x0000;
static uint16_t currentTouch = 0x0000; 
static uint32_t noteTimers [12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t previousNote [12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

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


    chordQuality = MAJ;
    rootNote = MIDINotes[0];

    while (1) 
    {
        tud_task();
        chord_select_task();
        midi_task();
    }
}

//Reads MPR121 and sends midi messages 
//Note ON only once per touch
//Note OFF after NOTE_DURATION has elapsed 
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
                note = getNote(rootNote, electrode, chordQuality);
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
        if ((board_millis() - noteTimers[i]) > NOTE_DURATION && (noteTimers[i] != 0))
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
uint8_t getNote(int root, int string, int chordQuality)
{
    uint8_t note;
    note = root + chordVoicings[chordQuality][string] + 12;
    return note;
}

//Scan keyboard matrix and identify the chord being selected
//Bits 0 - 21 are the readings of the 22 keyswitches. Other bits are unused.
void chord_select_task()
{  
    uint32_t keyState = 0x00000000;
    uint32_t noteKeypress = 0x00000000;
    uint32_t qualityKeypress = 0x00000000;
    uint8_t columnState;

    //Turn on columns in order and read the rows
    //Saved into keyState variable
    //See chord.h for positions of each keyswitch
    for (int i = 0; i < 5; i++)
    {
        columnState = 0x00;
        gpio_put(columnPins[i], 1);
        for (int j = 0; j < 5; j++)
        {
            columnState = columnState | (gpio_get(rowPins[j]) << j);
        }
        keyState = (keyState << (i * 5)) | columnState;
    }

    noteKeypress = keyState & KEYS_NOTES_MASK;
    qualityKeypress = keyState & KEYS_QUALITY_MASK;
    switch (noteKeypress)
    {
        case KEY_C  : rootNote = C;
        case KEY_Cs : rootNote = Cs;
        case KEY_D  : rootNote = D;
        case KEY_Ds : rootNote = Ds; 
        case KEY_E  : rootNote = E;
        case KEY_F  : rootNote = F;
        case KEY_Fs : rootNote = Fs;
        case KEY_G  : rootNote = G;
        case KEY_Gs : rootNote = Gs;
        case KEY_A  : rootNote = A;
        case KEY_As : rootNote = As;
        case KEY_B  : rootNote = B;
        default     : rootNote = rootNote;  //Do not change note if >1 note key pressed simultaneously
    }

    switch (qualityKeypress)
    {
        case KEY_MAJ  : chordQuality = MAJ;
        case KEY_MIN  : chordQuality = MIN;
        case KEY_MAJ7 : chordQuality = MAJ7;
        case KEY_MIN7 : chordQuality = MIN7;
        case KEY_MAJ9 : chordQuality = MAJ9;
        case KEY_MIN9 : chordQuality = MIN9;
        case KEY_DOM7 : chordQuality = DOM7;
        case KEY_DOM9 : chordQuality = DOM9;
        case KEY_DIM  : chordQuality = DIM;
        case KEY_AUG  : chordQuality = AUG;
        default       : chordQuality = chordQuality;    //Do not change quality if >1 quality key pressed simultaneously
    }
    
}

void gpio_initialise()
{
    gpio_init(rowPins[0]);
    gpio_init(rowPins[1]);
    gpio_init(rowPins[2]);
    gpio_init(rowPins[3]);
    gpio_init(rowPins[4]);
    gpio_init(columnPins[0]);
    gpio_init(columnPins[1]);
    gpio_init(columnPins[2]);
    gpio_init(columnPins[3]);
    gpio_init(columnPins[4]);

    gpio_set_dir(rowPins[0], GPIO_IN);
    gpio_set_dir(rowPins[1], GPIO_IN);
    gpio_set_dir(rowPins[2], GPIO_IN);
    gpio_set_dir(rowPins[3], GPIO_IN);
    gpio_set_dir(rowPins[4], GPIO_IN);
    gpio_set_dir(columnPins[0], GPIO_OUT);
    gpio_set_dir(columnPins[1], GPIO_OUT);
    gpio_set_dir(columnPins[2], GPIO_OUT);
    gpio_set_dir(columnPins[3], GPIO_OUT);
    gpio_set_dir(columnPins[4], GPIO_OUT);
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
