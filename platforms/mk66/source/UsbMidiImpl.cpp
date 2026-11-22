#include "UsbMidiImpl.h"
#include "usb_midi.h"

bool UsbMidiImpl::isReadAvailable()
{
    return usb_midi_available();
}

void UsbMidiImpl::read()
{
    usbMIDI.read();
}

void UsbMidiImpl::sendRealTime(MessageType messageType)
{
    usbMIDI.sendRealTime((uint8_t)messageType);
    usbMIDI.send_now();
}

void UsbMidiImpl::sendControlChange(uint8_t index, uint8_t value, uint8_t channel)
{
    usbMIDI.sendControlChange(index, value, channel);
}

void UsbMidiImpl::sendProgramChange(uint8_t programNumber, uint8_t channel)
{
    usbMIDI.sendProgramChange(programNumber, channel);
}

void UsbMidiImpl::sendAfterTouch(uint8_t velocity, uint8_t channel)
{
    usbMIDI.sendAfterTouch(velocity, channel);
}

void UsbMidiImpl::sendPolyPressure(uint8_t note, uint8_t value, uint8_t channel)
{
    usbMIDI.sendPolyPressure(note, value, channel);
}

void UsbMidiImpl::sendPitchBend(int value, uint8_t channel)
{
    usbMIDI.sendPitchBend(value, channel);
}

void UsbMidiImpl::sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel)
{
    usbMIDI.sendNoteOn(note, velocity, channel);
}

void UsbMidiImpl::sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel)
{
    usbMIDI.sendNoteOff(note, velocity, channel);
}

void UsbMidiImpl::setReceiveActions(MidiReceiveActions* receiveActions)
{
    usbMIDI.setHandleRealTimeSystem(receiveActions->realTime);
    usbMIDI.setHandleNoteOn(receiveActions->noteOn);
    usbMIDI.setHandleNoteOff(receiveActions->noteOff);
    usbMIDI.setHandleSongPosition(receiveActions->songPosition);
    usbMIDI.setHandleControlChange(receiveActions->controlChange);
    usbMIDI.setHandleProgramChange(receiveActions->programChange);
}
