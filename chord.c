#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "mpr121.h"
#include "74HC595.h"
#include "chord.h"
#include "bsp/board.h"
#include "tusb.h"


static enum Notes rootNote;
static enum ChordQuality chordQuality;
static struct mpr121_sensor mpr121;
static struct ShiftRegister_74HC595 shift;
static uint16_t plucked = 0x0000;
static uint16_t previousTouch = 0x0000;
static uint16_t currentTouch = 0x0000; 
static uint32_t noteTimers [12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t previousNote [12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint32_t scanTimer = 0;
static uint32_t mpr121ScanTimer = 0;

int main() 
{
    board_init();
    tusb_init();
    gpio_initialise();

    // Initialise and autoconfigure the touch sensor.
    i2c_init(I2C_PORT, MPR121_I2C_FREQ);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    mpr121_init(I2C_PORT, MPR121_I2C_ADDR, &mpr121);
    mpr121_set_thresholds(MPR121_TOUCH_THRESHOLD,
                          MPR121_RELEASE_THRESHOLD, &mpr121);

    //Initialise shift registers
    ShiftRegister_74HC595_init(&shift, SR_SERIAL_PIN, SR_CLOCK_PIN, SR_LATCH_PIN);

    //Initiatialise UART MIDI
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(UART_ID, true);
    uart_set_translate_crlf(UART_ID, false);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, UART_DATA_BITS, UART_STOP_BITS, UART_PARITY);


    chordQuality = MAJ;
    rootNote = C;

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
        for (int electrode = 0; electrode < 12; electrode++)
        {
            if (((previousTouch >> electrode) & 1) == 0 && ((currentTouch >> electrode) & 1) == 1)
            {
                gpio_put(25, 1);
                note = get_MIDI_note(rootNote, electrode, chordQuality);
                uint8_t note_on[3] = { 0x90 | channel, note, 127 };
                tud_midi_stream_write(cable_num, note_on, 3);
                send_MIDI_UART(1, note, 127);
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

//Returns the MIDI Note number to play according to root note and chord quality
uint8_t get_MIDI_note(int root, int string, int chordQuality)
{
    uint8_t MIDInote;
    MIDInote = MIDINotes[root] + chordVoicings[chordQuality][string] + 12;
    return MIDInote;
}

void update_leds()
{
    uint32_t leds = 0x00000000;
    uint8_t byteToShift = 0x00;
    leds = leds | ledNotesMappings[rootNote];
    leds = leds | ledQualityMappings[chordQuality];
    
    for (int i = 0; i < 4; i++)
    {
        byteToShift = byteToShift | (leds >> (i * 8));
        shiftOutByte(&shift, byteToShift);
        byteToShift = 0x00;
    }
    latchRegister(&shift);
}

//Scan keyboard matrix and identify the chord being selected
//Bits 0 - 21 are the readings of the 22 keyswitches. Other bits are unused.
void chord_select_task()
{  
    //Only scan keyswitches if scan interval has elapsed
    if (board_millis() - scanTimer < KEYS_SCAN_INTERVAL) 
        return;
    scanTimer = board_millis();

    uint8_t columnState;
    uint32_t keyState = 0x00000000;
    uint32_t noteKeypress = 0x00000000;
    uint32_t qualityKeypress = 0x00000000;

    for (int column = 0; column < 5; column++)
    {
        //clear columns
        columnState = 0x00;
        for(int k = 0; k < 5; k++)
        {
            gpio_put(columnPins[k], 0);
        }
        
        //pull one column high
        gpio_put(columnPins[column], 1);
        sleep_us(100); //Allow levels to settle before reading
        {   
            for (int j = 0; j < 5; j++)
            {
                columnState = columnState | (gpio_get(rowPins[j]) << j);
            }
            keyState = keyState | (columnState << (column*5));
        }
    }
    noteKeypress = keyState & KEYS_NOTES_MASK;
    qualityKeypress = keyState & KEYS_QUALITY_MASK;
    update_chord(noteKeypress, qualityKeypress);
    update_leds();
}

void update_chord(uint32_t noteKeypress, uint32_t qualityKeypress)
{
    for (int i = 0; i < 12; i++)
    {
        if (noteKeypress == keyMappings[i])
        {
            rootNote = i;
        }
    }
    for (int i = 0; i < 10; i++)
    {
        if (qualityKeypress == qualityMappings[i])
        {
            chordQuality = i;
        }
    }
}

//Send Note-on message
void send_MIDI_UART(int channel, int note, int velocity)
{
    unsigned char statusByte = 0x91; //Note-on, channel 1
    unsigned char dataByte1 = (unsigned char) note;
    unsigned char dataByte2 = (unsigned char) MIDI_UART_VELOCITY;
    send_UART_byte(statusByte);
    send_UART_byte(dataByte1);
    send_UART_byte(dataByte2);
}

//Blocking UART transmission
void send_UART_byte(unsigned char byteToSend)
{
    while (!uart_is_writable(UART_ID)) {}
	uart_putc(UART_ID, byteToSend);
	return;
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
