#include "UartMidiImpl.h"
#include "MIDI.h"

static MIDI_NAMESPACE::SerialMIDI<HardwareSerial5> serialMIDI(Serial5);
static MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial5>> midiDriver(
  (MIDI_NAMESPACE::SerialMIDI<HardwareSerial5>&)serialMIDI);

void UartMidiImpl::init()
{
    midiDriver.begin(0);
    midiDriver.turnThruOff();
}

void UartMidiImpl::setReceiveActions(MidiReceiveActions* receiveActions)
{
    midiDriver.setHandleNoteOn(receiveActions->noteOn);
    midiDriver.setHandleNoteOff(receiveActions->noteOff);
    midiDriver.setHandleClock(receiveActions->clock);
    midiDriver.setHandleStart(receiveActions->start);
    midiDriver.setHandleStop(receiveActions->stop);
    midiDriver.setHandleSongPosition(receiveActions->songPosition);
    midiDriver.setHandleContinue(receiveActions->continueMessage);
    midiDriver.setHandleControlChange(receiveActions->controlChange);
    midiDriver.setHandleProgramChange(receiveActions->programChange);
}

bool UartMidiImpl::isReadAvailable()
{
    return Serial5.available();
}

void UartMidiImpl::read()
{
    midiDriver.read();
}

void UartMidiImpl::sendRealTime(MessageType messageType)
{
    midiDriver.sendRealTime((midi::MidiType)messageType);
}

void UartMidiImpl::sendControlChange(uint8_t index, uint8_t value, uint8_t channel)
{
    midiDriver.sendControlChange(index, value, channel);
}

void UartMidiImpl::sendProgramChange(uint8_t programNumber, uint8_t channel)
{
    midiDriver.sendProgramChange(programNumber, channel);
}

void UartMidiImpl::sendAfterTouch(uint8_t velocity, uint8_t channel)
{
    midiDriver.sendAfterTouch(velocity, channel);
}

void UartMidiImpl::sendPolyPressure(uint8_t note, uint8_t value, uint8_t channel)
{
    midiDriver.sendPolyPressure(note, value, channel);
}

void UartMidiImpl::sendPitchBend(int value, uint8_t channel)
{
    midiDriver.sendPitchBend(value, channel);
}

void UartMidiImpl::sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel)
{
    midiDriver.sendNoteOn(note, velocity, channel);
}

void UartMidiImpl::sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel)
{
    midiDriver.sendNoteOff(note, velocity, channel);
}

void UartMidiImpl::setReceiveNoteOnAction(void (*action)(uint8_t, uint8_t, uint8_t))
{
    midiDriver.setHandleNoteOn(action);
}
