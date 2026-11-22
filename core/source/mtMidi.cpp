#include "mtSequencer.h"

#include "configEditor/configEditor.h"
#include "instrumentEditor/instrumentEditor.h"
#include "patternEditor/patternEditor.h"
#include "sampleEditor/sampleEditor.h"
#include "samplePlayback/samplePlayback.h"
#include "sampleRecorder/sampleRecorder.h"
#include "songEditor/songEditor.h"

#include "Platform.h"
#include "TrackerImpl.h"
#include "core/interface.h"
#include "util_math.h"
#include <cmath>

extern cPatternEditor patternEditor;

static cSamplePlayback* SP = &samplePlayback;
static cSampleRecorder* SR = &sampleRecorder;
static cSampleEditor* SE = &sampleEditor;
static cPatternEditor* PTE = &patternEditor;
static cInstrumentEditor* IE = &instrumentEditor;
static UartMidi* uartMidi = nullptr;
static UsbMidi* usbMidi = nullptr;

extern void playSongFromSelectedPattern();
extern void playSongFromSelectedPattern_stop();

void peformanceModeOnMidiCC(uint8_t cc_idx, uint8_t cc_value);
void changeMixerOnMidiCC(uint8_t cc_idx, uint8_t cc_value);

elapsedMillis timerMidiDelay = 0;

void midiInit(UartMidi* uartMidi, UsbMidi* usbMidi)
{
    ::uartMidi = uartMidi;
    ::usbMidi = usbMidi;
    // jack

    UartMidi::MidiReceiveActions uartMidiReceiveActions;
    uartMidiReceiveActions.noteOn = handleJackNoteOn;
    uartMidiReceiveActions.noteOff = handleJackNoteOff;
    uartMidiReceiveActions.clock = receiveJackClock;
    uartMidiReceiveActions.start = receiveJackStart;
    uartMidiReceiveActions.stop = receiveJackStop;
    uartMidiReceiveActions.songPosition = handleJackSongPosition;
    uartMidiReceiveActions.continueMessage = receiveJackContinue;
    uartMidiReceiveActions.controlChange = handleJackControlChange;
    uartMidiReceiveActions.programChange = handleJackProgramChange;
    uartMidi->setReceiveActions(&uartMidiReceiveActions);

    // usb
    UsbMidi::MidiReceiveActions usbMidiReceiveActions;
    usbMidiReceiveActions.realTime = receiveUsbRealtime;
    usbMidiReceiveActions.noteOn = handleUsbNoteOn;
    usbMidiReceiveActions.noteOff = handleUsbNoteOff;
    usbMidiReceiveActions.songPosition = handleUsbSongPosition;
    usbMidiReceiveActions.controlChange = handleUSBControlChange;
    usbMidiReceiveActions.programChange = handleUsbProgramChange;

    usbMidi->setReceiveActions(&usbMidiReceiveActions);
}

void midiUpdate()
{
    if (sequencer.isRenderMode())
        return;

    if (uartMidi != nullptr)
    {
        while (uartMidi->isReadAvailable())
        {
            uartMidi->read();
        }
    }

    if (usbMidi != nullptr)
    {
        while (usbMidi->isReadAvailable())
        {
            usbMidi->read();
        }
    }
}

uint8_t isIncomingChannelDesired(uint8_t chan)
{
    return (mtConfig.midi.notesInChannel == chan || mtConfig.midi.notesInChannel == 0);
}

void handleJackNoteOn(byte channel, byte pitch, byte velocity)
{
    //	Serial.printf("ON\tch: %d, pitch: %d, velo: %d \r\n", channel, pitch,
    //					velocity);
    //	MIDI.sendNoteOn(pitch, velocity, channel);

    if (velocity == 0)
    {
        handleJackNoteOff(channel, pitch, 0);
        return;
    }

    if (mtConfig.midi.notesInMode == notesIn_jack ||
        mtConfig.midi.notesInMode == notesIn_usb_and_jack)
    {

        if (isIncomingChannelDesired(channel))
        {

            int8_t transp = 12 * (mtConfig.midi.middleC - 2);
            pitch = clamp(pitch + transp, 0, 127);

            if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_sequencer)
            {
                sendSelection();

                sequencer.handleNoteOn(
                  Sequencer::GRID_OUTSIDE_PATTERN, pitch, velocity, pitch + 100);

                patternEditor.refreshPatternFromOutsideModule();
            }
            else if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_SamplePlayback)
            {
                SP->noteOnHandle(channel, pitch, velocity, pitch + 100);
            }
            else if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_SampleEditor)
            {
                SE->noteOnHandle(channel, pitch, velocity, pitch + 100);
            }
            else if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_SampleRecorder)
            {
                SR->noteOnHandle(channel, pitch, velocity, pitch + 100);
            }
        }
    }
}
void handleUsbNoteOn(byte channel, byte pitch, byte velocity)
{
    //	Serial.printf("ON\tch: %d, pitch: %d, velo: %d \r\n", channel, pitch,
    //					velocity);
    //	MIDI.sendNoteOn(pitch, velocity, channel);

    if (velocity == 0)
    {
        handleUsbNoteOff(channel, pitch, 0);
        return;
    }

    if (mtConfig.midi.notesInMode == notesIn_usb ||
        mtConfig.midi.notesInMode == notesIn_usb_and_jack)
    {
        if (isIncomingChannelDesired(channel))
        {
            int8_t transp = 12 * (mtConfig.midi.middleC - 2);
            pitch = clamp(pitch + transp, 0, 127);

            if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_sequencer)
            {
                sendSelection();

                sequencer.handleNoteOn(
                  Sequencer::GRID_OUTSIDE_PATTERN, pitch, velocity, pitch + 100);

                patternEditor.refreshPatternFromOutsideModule();
            }
            else if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_SamplePlayback)
            {
                SP->noteOnHandle(channel, pitch, velocity, pitch + 100);
            }
            else if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_SampleEditor)
            {
                SE->noteOnHandle(channel, pitch, velocity, pitch + 100);
            }
            else if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_SampleRecorder)
            {
                SR->noteOnHandle(channel, pitch, velocity, pitch + 100);
            }
        }
    }
}

void handleJackNoteOff(byte channel, byte pitch, [[maybe_unused]] byte velocity)
{

    //	Serial.printf("OFF\tch: %d, pitch: %d, velo: %d\r\n", channel, pitch,
    //					velocity);

    if (mtConfig.midi.notesInMode == notesIn_jack ||
        mtConfig.midi.notesInMode == notesIn_usb_and_jack)
    {
        if (isIncomingChannelDesired(channel))
        {
            int8_t transp = 12 * (mtConfig.midi.middleC - 2);
            pitch = clamp(pitch + transp, 0, 127);

            if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_sequencer)
            {
                sendSelection();
                sequencer.handleNoteOff(Sequencer::GRID_OUTSIDE_PATTERN, pitch, 0, pitch + 100);

                if (isEditMode())
                {
                    //					if (isMultiSelectionOnOneLine() &&
                    // sequencer.noMoreRecOpen())
                    if (sequencer.noMoreRecOpen())
                    {
                        PTE->moveCursorByStepParallel();
                    }
                }
                patternEditor.refreshPatternFromOutsideModule();
            }
            else if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_SamplePlayback)
            {
                SP->noteOffHandle(channel, pitch, 0, pitch + 100);
            }
            else if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_SampleEditor)
            {
                SE->noteOffHandle(channel, pitch, 0, pitch + 100);
            }
            else if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_SampleRecorder)
            {
                SR->noteOffHandle(channel, pitch, 0, pitch + 100);
            }
        }
    }
}

void handleUsbNoteOff(byte channel, byte pitch, [[maybe_unused]] byte velocity)
{

    //	Serial.printf("OFF\tch: %d, pitch: %d, velo: %d\r\n", channel, pitch,
    //					velocity);

    if (mtConfig.midi.notesInMode == notesIn_usb ||
        mtConfig.midi.notesInMode == notesIn_usb_and_jack)
    {
        if (isIncomingChannelDesired(channel))
        {
            int8_t transp = 12 * (mtConfig.midi.middleC - 2);
            pitch = clamp(pitch + transp, 0, 127);

            if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_sequencer)
            {
                sendSelection();
                sequencer.handleNoteOff(Sequencer::GRID_OUTSIDE_PATTERN, pitch, 0, pitch + 100);

                if (isEditMode())
                {
                    if (sequencer.noMoreRecOpen())
                    {
                        PTE->moveCursorByStepParallel();
                    }
                }
                patternEditor.refreshPatternFromOutsideModule();
            }
            else if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_SamplePlayback)
            {
                SP->noteOffHandle(channel, pitch, 0, pitch + 100);
            }
            else if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_SampleEditor)
            {
                SE->noteOffHandle(channel, pitch, 0, pitch + 100);
            }
            else if (sequencer.getMidiInVoiceMode() == sequencer.midiInVoiceMode_SampleRecorder)
            {
                SR->noteOffHandle(channel, pitch, 0, pitch + 100);
            }
        }
    }
}

void handleJackControlChange(byte channel, byte number, byte value)
{

    if ((mtConfig.midi.ccInMode == ccIn_None) || (mtConfig.midi.ccInMode == ccIn_usb))
        return;

    if ((channel == mtConfig.midi.ccInChannel) || (mtConfig.midi.ccInChannel == ALL_CC_CHANNELS))
    {
        if (mtInterface.isPerformanceModeScreenActive())
            peformanceModeOnMidiCC(number, value);

        changeMixerOnMidiCC(number, value);

        IE->onControlChangeReceive(number, value);
        SP->onControlChangeReceive(number, value);
    }
}

void handleUSBControlChange(byte channel, byte number, byte value)
{
    if ((mtConfig.midi.ccInMode == ccIn_None) || (mtConfig.midi.ccInMode == ccIn_jack))
        return;

    if ((channel == mtConfig.midi.ccInChannel) || (mtConfig.midi.ccInChannel == ALL_CC_CHANNELS))
    {
        if (mtInterface.isPerformanceModeScreenActive())
            peformanceModeOnMidiCC(number, value);

        changeMixerOnMidiCC(number, value);

        IE->onControlChangeReceive(number, value);
        SP->onControlChangeReceive(number, value);
    }
}

static void handleProgramChange(byte channel, byte program)
{
    if ((channel != mtConfig.midi.programChangeInChannel) &&
        (mtConfig.midi.programChangeInChannel != ALL_PROGRAM_CHANGE_CHANNELS))
    {
        return;
    }

    uint8_t targetPattern = (program & 0x7F) + 1;

    if (sequencer.isPlay())
    {
        sequencer.sequencialSwitch_queuePattern(targetPattern);
    }
    else
    {
        sequencer.requestStoppedPatternSwitch(targetPattern);
    }
}

void handleJackProgramChange(byte channel, byte program)
{
    if ((mtConfig.midi.programChangeInMode == programChangeIn_None) ||
        (mtConfig.midi.programChangeInMode == programChangeIn_usb))
    {
        return;
    }

    handleProgramChange(channel, program);
}

void handleUsbProgramChange(byte channel, byte program)
{
    if ((mtConfig.midi.programChangeInMode == programChangeIn_None) ||
        (mtConfig.midi.programChangeInMode == programChangeIn_jack))
    {
        return;
    }

    handleProgramChange(channel, program);
}

static uint32_t s_audioBlockStartUs = 0;
static uint32_t s_clockSubBlockOffsetUs = 0;

AUDIO_CORE_RAM void midiSetAudioBlockStartUs(uint32_t blockStartUs)
{
    s_audioBlockStartUs = blockStartUs;
}

AUDIO_CORE_RAM void midiSetClockSubBlockOffsetUs(uint32_t offsetUs)
{
    s_clockSubBlockOffsetUs = offsetUs;
}

static void sendMidiClockImmediate()
{
    if (sequencer.isRenderMode())
        return;
    switch (mtConfig.midi.clkOut)
    {
        case clockOut_usb: usbMidi->sendRealTime(Midi::MessageType::kClock); break;

        case clockOut_usb_and_jack:
            usbMidi->sendRealTime(Midi::MessageType::kClock);
            uartMidi->sendRealTime(Midi::MessageType::kClock);
            break;

        case clockOut_jack: uartMidi->sendRealTime(Midi::MessageType::kClock); break;

        default: break;
    }
}

AUDIO_CORE_RAM void sendMidiStart()
{
    if (sequencer.isRenderMode())
        return;
    switch (mtConfig.midi.transportOut)
    {
        case clockOut_usb: usbMidi->sendRealTime(Midi::MessageType::kStart); break;

        case clockOut_usb_and_jack:
            usbMidi->sendRealTime(Midi::MessageType::kStart);
            uartMidi->sendRealTime(Midi::MessageType::kStart);
            break;

        case clockOut_jack: uartMidi->sendRealTime(Midi::MessageType::kStart); break;

        default: break;
    }
}
void sendMidiStop()
{
    if (sequencer.isRenderMode())
        return;
    if (Platform* platform = TrackerImpl::getInstance()->getPlatform(); platform != nullptr)
    {
        platform->getTimer()->cancelMidiClock();
    }
    switch (mtConfig.midi.transportOut)
    {
        case clockOut_usb: usbMidi->sendRealTime(Midi::MessageType::kStop); break;

        case clockOut_usb_and_jack:
            usbMidi->sendRealTime(Midi::MessageType::kStop);
            uartMidi->sendRealTime(Midi::MessageType::kStop);
            break;

        case clockOut_jack: uartMidi->sendRealTime(Midi::MessageType::kStop); break;

        default: break;
    }
}

AUDIO_CORE_RAM void sendMidiClock()
{
    if (sequencer.isRenderMode() || mtConfig.midi.clkOut == clockOut_Off)
        return;

    Platform* platform = TrackerImpl::getInstance()->getPlatform();
    if (platform == nullptr)
        return;

    Timer* timer = platform->getTimer();
    uint32_t elapsedUs = timer->micros() - s_audioBlockStartUs;
    uint32_t delayUs =
      (s_clockSubBlockOffsetUs > elapsedUs) ? (s_clockSubBlockOffsetUs - elapsedUs) : 0U;
    timer->scheduleMidiClockUs(delayUs, sendMidiClockImmediate);
}

void receiveUsbRealtime(uint8_t type)
{
    switch (type)
    {
        case Midi::MessageType::kClock:
            if (mtConfig.midi.clkIn == clockIn_Usb)
            {
                receiveClock();
            }
            break;

        case Midi::MessageType::kStart:
            if (mtConfig.midi.transportIn == clockIn_Usb)
            {
                //			Serial.printf("usb start\n");
                //			Serial.send_now();
                receiveStart();
            }
            break;

        case Midi::MessageType::kContinue:
            if (mtConfig.midi.transportIn == clockIn_Usb)
            {
                //			Serial.printf("usb continue\n");
                //			Serial.send_now();
                receiveContinue();
            }
            break;

        case Midi::MessageType::kStop:
            if (mtConfig.midi.transportIn == clockIn_Usb)
            {
                //			Serial.printf("usb stop\n");
                //			Serial.send_now();
                receiveStop();
            }
            break;

        default: break;
    }
}

void receiveJackStart()
{
    if (mtConfig.midi.transportIn == clockIn_Jack)
    {
        receiveStart();
    }
}

void receiveJackStop()
{
    if (mtConfig.midi.transportIn == clockIn_Jack)
    {
        receiveStop();
    }
}

void receiveJackContinue()
{
    if (mtConfig.midi.transportIn == clockIn_Jack)
    {
        receiveContinue();
    }
}

void receiveJackClock()
{
    if (mtConfig.midi.clkIn == clockIn_Jack)
    {
        receiveClock();
    }
}

uint16_t lastSongPosition = 0;

void receiveClock()
{
    sequencer.handleMidiInClock();
}

void receiveStart()
{
    bool songMode = mtInterface.isSongScreenActive();
    sequencer.handleMidiInStart(false, songMode);
}

void receiveContinue()
{
    bool songMode = mtInterface.isSongScreenActive();
    sequencer.handleMidiInStart(true, songMode);
}

void receiveStop()
{
    sequencer.handleMidiInStop();
}

void handleUsbSongPosition(uint16_t beats)
{
    //	Serial.printf("song pos: %d beats\n", beats);
    //	sequencer.forcePosition(beats);

    if (mtConfig.midi.transportIn == clockIn_Usb)
    {
        lastSongPosition = beats;
        sequencer.handleMidiInSongPosition(beats);
    }
}

void handleJackSongPosition(unsigned int beats)
{
    //	Serial.printf("song pos: %d beasts\n", beats);
    //	sequencer.forcePosition(beats);
    if (mtConfig.midi.transportIn == clockIn_Jack)
    {
        lastSongPosition = beats;
        sequencer.handleMidiInSongPosition(static_cast<uint16_t>(beats));
    }
}

AUDIO_CORE_RAM void sendCC(uint8_t index, uint8_t value, uint8_t channel, bool toSynth)
{
    if (toSynth)
    {
#ifdef FEATURE_SYNTH
        TrackerImpl::getInstance()->getSynth().getVirtualMidiPort()->sendControlChange(
          index, value, channel);
#endif
        return;
    }

    if (sequencer.isRenderMode())
        return;

    if (mtConfig.midi.notesOutMode == notesOut_mode_usb ||
        mtConfig.midi.notesOutMode == notesOut_mode_usb_and_jack)
    {
        usbMidi->sendControlChange(index, value, channel);
    }

    if (mtConfig.midi.notesOutMode == notesOut_mode_jack ||
        mtConfig.midi.notesOutMode == notesOut_mode_usb_and_jack)
    {
        uartMidi->sendControlChange(index, value, channel);
    }
}

void sendProgramChange(uint8_t value, uint8_t channel, bool toSynth)
{
    assert(channel > 0);
    assert(channel <= 16);

    if (toSynth)
    {
#ifdef FEATURE_SYNTH
        TrackerImpl::getInstance()->getSynth().getVirtualMidiPort()->sendProgramChange(value,
                                                                                       channel);
#endif
        return;
    }

    if (sequencer.isRenderMode())
        return;

    if (mtConfig.midi.notesOutMode == notesOut_mode_usb ||
        mtConfig.midi.notesOutMode == notesOut_mode_usb_and_jack)
    {
        usbMidi->sendProgramChange(value, channel);
    }

    if (mtConfig.midi.notesOutMode == notesOut_mode_jack ||
        mtConfig.midi.notesOutMode == notesOut_mode_usb_and_jack)
    {
        uartMidi->sendProgramChange(value, channel);
    }
}

void sendAfterTouch(uint8_t value, uint8_t channel, bool toSynth)
{
    assert(channel > 0);
    assert(channel <= 16);

    if (toSynth)
    {
#ifdef FEATURE_SYNTH
        TrackerImpl::getInstance()->getSynth().getVirtualMidiPort()->sendAfterTouch(value, channel);
#endif
        return;
    }

    if (sequencer.isRenderMode())
        return;

    if (mtConfig.midi.notesOutMode == notesOut_mode_usb ||
        mtConfig.midi.notesOutMode == notesOut_mode_usb_and_jack)
    {
        usbMidi->sendAfterTouch(value, channel);
    }

    if (mtConfig.midi.notesOutMode == notesOut_mode_jack ||
        mtConfig.midi.notesOutMode == notesOut_mode_usb_and_jack)
    {
        uartMidi->sendAfterTouch(value, channel);
    }
}

void sendPolyPressure(uint8_t note, uint8_t value, uint8_t channel, bool toSynth)
{
    assert(channel > 0);
    assert(channel <= 16);

    if (toSynth)
    {
#ifdef FEATURE_SYNTH
        TrackerImpl::getInstance()->getSynth().getVirtualMidiPort()->sendPolyPressure(
          note, value, channel);
#endif
        return;
    }

    if (sequencer.isRenderMode())
        return;

    int8_t transp = 12 * (2 - mtConfig.midi.middleC);
    note = clamp(note + transp, 0, 127);

    if (mtConfig.midi.notesOutMode == notesOut_mode_usb ||
        mtConfig.midi.notesOutMode == notesOut_mode_usb_and_jack)
    {
        usbMidi->sendPolyPressure(note, value, channel);
    }

    if (mtConfig.midi.notesOutMode == notesOut_mode_jack ||
        mtConfig.midi.notesOutMode == notesOut_mode_usb_and_jack)
    {
        uartMidi->sendPolyPressure(note, value, channel);
    }
}

void sendPitchbend(int16_t value, int8_t channel, bool toSynth)
{
    assert(channel > 0);
    assert(channel <= 16);

    if (toSynth)
    {
#ifdef FEATURE_SYNTH
        TrackerImpl::getInstance()->getSynth().getVirtualMidiPort()->sendPitchBend(value, channel);
#endif
        return;
    }

    if (sequencer.isRenderMode())
        return;

    if (mtConfig.midi.notesOutMode == notesOut_mode_usb ||
        mtConfig.midi.notesOutMode == notesOut_mode_usb_and_jack)
    {

        usbMidi->sendPitchBend(value, channel);
    }

    if (mtConfig.midi.notesOutMode == notesOut_mode_jack ||
        mtConfig.midi.notesOutMode == notesOut_mode_usb_and_jack)
    {
        uartMidi->sendPitchBend(value, channel);
    }
}

void sendMidiNoteOn(uint8_t note, uint8_t velo, uint8_t channel, bool toSynth)
{
    assert(channel > 0);
    assert(channel <= 16);

    if (toSynth)
    {
#ifdef FEATURE_SYNTH
        TrackerImpl::getInstance()->getSynth().getVirtualMidiPort()->sendNoteOn(
          note, velo, channel);
#endif
        return;
    }

    if (sequencer.isRenderMode())
        return;

    int8_t transp = 12 * (2 - mtConfig.midi.middleC);
    note = clamp(note + transp, 0, 127);

    if (mtConfig.midi.notesOutMode == notesOut_mode_usb ||
        mtConfig.midi.notesOutMode == notesOut_mode_usb_and_jack)
    {
        usbMidi->sendNoteOn(note, velo, channel);
    }

    if (mtConfig.midi.notesOutMode == notesOut_mode_jack ||
        mtConfig.midi.notesOutMode == notesOut_mode_usb_and_jack)
    {
        uartMidi->sendNoteOn(note, velo, channel);
    }
}
void sendMidiNoteOff(uint8_t note, [[maybe_unused]] uint8_t velo, uint8_t channel, bool toSynth)
{
    assert(channel > 0);
    assert(channel <= 16);

    if (toSynth)
    {
#ifdef FEATURE_SYNTH
        TrackerImpl::getInstance()->getSynth().getVirtualMidiPort()->sendNoteOff(note, 0, channel);
#endif
        return;
    }

    if (sequencer.isRenderMode())
        return;

    int8_t transp = 12 * (2 - mtConfig.midi.middleC);
    note = clamp(note + transp, 0, 127);

    if (mtConfig.midi.notesOutMode == notesOut_mode_usb ||
        mtConfig.midi.notesOutMode == notesOut_mode_usb_and_jack)
    {
        usbMidi->sendNoteOff(note, 0, channel);
    }

    if (mtConfig.midi.notesOutMode == notesOut_mode_jack ||
        mtConfig.midi.notesOutMode == notesOut_mode_usb_and_jack)
    {
        uartMidi->sendNoteOff(note, 0, channel);
    }
}

void resetMidi()
{
    uartMidi->init();
}

void setHandleNoteOnAction(void (*action)(uint8_t, uint8_t, uint8_t))
{
    uartMidi->setReceiveNoteOnAction(action);
}

void sendNoteOnRaw(uint8_t note, uint8_t velo, uint8_t channel)
{
    uartMidi->sendNoteOn(note, velo, channel);
}
