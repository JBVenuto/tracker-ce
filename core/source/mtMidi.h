#ifndef SOURCE_MTMIDI_H_
#define SOURCE_MTMIDI_H_

#include <stdint.h>

typedef uint8_t byte;

constexpr uint32_t MIDI_REFRESH_TIME_US = 500;

static constexpr int kMidiPitchbendMin = -8192;
static constexpr int kMidiPitchbendMax = 8191;

static constexpr int kMaxMidiVelocity = 127;

enum enClockOut
{
    clockOut_Off,
    clockOut_usb,
    clockOut_jack,
    clockOut_usb_and_jack,
    clockOut_count
};

enum enNotesIn
{
    notesIn_None,
    notesIn_usb,
    notesIn_jack,
    notesIn_usb_and_jack,
};

enum enCCIn
{
    ccIn_None,
    ccIn_usb,
    ccIn_jack,
    ccIn_usb_and_jack,
};

enum enProgramChangeIn
{
    programChangeIn_None,
    programChangeIn_usb,
    programChangeIn_jack,
    programChangeIn_usb_and_jack,
};

enum enNotesOut
{
    notesOut_mode_None,
    notesOut_mode_usb,
    notesOut_mode_jack,
    notesOut_mode_usb_and_jack,
};

enum enClockIn
{
    clockIn_Internal,
    clockIn_Usb,
    clockIn_Jack,

};

constexpr uint8_t ALL_CC_CHANNELS = 0;
constexpr uint8_t ALL_PROGRAM_CHANGE_CHANNELS = 0;
class UartMidi;
class UsbMidi;

void midiInit(UartMidi* uartMidi, UsbMidi* usbMidi);
void midiUpdate();
void handleJackNoteOn(byte channel, byte pitch, byte velocity);
void handleJackNoteOff(byte channel, byte pitch, byte velocity);

void handleUsbNoteOn(byte channel, byte pitch, byte velocity);
void handleUsbNoteOff(byte channel, byte pitch, byte velocity);

// hocki clocki
void midiSetAudioBlockStartUs(uint32_t blockStartUs);
void midiSetClockSubBlockOffsetUs(uint32_t offsetUs);
void sendMidiStart();
void sendMidiClock();
void sendMidiStop();

void receiveUsbRealtime(uint8_t type); // includes Clock/Start/Stop/Continue
void receiveUsbStart();
void receiveUsbStop();
void receiveJackStart();
void receiveJackStop();
void receiveJackContinue();
void receiveJackClock();
void handleUsbSongPosition(uint16_t beats);
void handleJackSongPosition(unsigned int);

void handleJackControlChange(byte channel, byte number, byte value);
void handleUSBControlChange(byte channel, byte number, byte value);

void handleJackProgramChange(byte channel, byte program);
void handleUsbProgramChange(byte channel, byte program);

void receiveClock();
void receiveStart();
void receiveContinue();
void receiveStop();

void sendCC(uint8_t index, uint8_t value, uint8_t channel, bool toSynth);
void sendProgramChange(uint8_t value, uint8_t channel, bool toSynth);
void sendAfterTouch(uint8_t value, uint8_t channel, bool toSynth);
void sendPolyPressure(uint8_t note, uint8_t value, uint8_t channel, bool toSynth);
void sendPitchbend(int16_t value, int8_t channel, bool toSynth);
void sendMidiNoteOn(uint8_t note, uint8_t velo, uint8_t channel, bool toSynth);
void sendMidiNoteOff(uint8_t note, uint8_t velo, uint8_t channel, bool toSynth);

void resetMidi();
void setHandleNoteOnAction(void (*action)(uint8_t, uint8_t, uint8_t));
void sendNoteOnRaw(uint8_t note, uint8_t velo, uint8_t channel);

#endif /* SOURCE_MTMIDI_H_ */
