#include "UsbMidiImpl.h"
#include "log.h"
#include "usb_midi.h"

bool UsbMidiImpl::isReadAvailable()
{
    return midiUsbAvailable();
}

void UsbMidiImpl::read()
{
    MidiUsbMessage message;

    midiUsbReceive(&message);

    switch (message.type)
    {
        case MessageType::kClock:
        case MessageType::kStart:
        case MessageType::kStop:
        case MessageType::kContinue:
            midiReceiveActions.realTime(message.type);
            break;
        case MessageType::kNoteOff:
            midiReceiveActions.noteOff(message.channel, message.data1, message.data2);
            break;
        case MessageType::kNoteOn:
            midiReceiveActions.noteOn(message.channel, message.data1, message.data2);
            break;
        case MessageType::kControlChange:
            midiReceiveActions.controlChange(message.channel, message.data1, message.data2);
            break;
        case MessageType::kProgramChange:
            if (midiReceiveActions.programChange != nullptr)
            {
                midiReceiveActions.programChange(message.channel, message.data1);
            }
            break;
        case MessageType::kSongPosition:
            midiReceiveActions.songPosition(
              (uint16_t)((message.data1 & 0x7f) | ((message.data2 & 0x7f) << 7)));
            break;
        default:
            LOGF("Unknown midi message: %x, %x, %x, %x",
                 message.type,
                 message.channel,
                 message.data1,
                 message.data2);
            break;
    }
}

void UsbMidiImpl::sendRealTime(MessageType messageType)
{
    switch (messageType)
    {
        case kClock:
        case kStart:
        case kContinue:
        case kStop:
        case kActiveSensing:
        case kSystemReset:
            send(messageType, 0, 0, 0);
            break;
        default:
            break;
    }
}

void UsbMidiImpl::sendControlChange(uint8_t index, uint8_t value, uint8_t channel)
{
    send(0xB0, channel, index, value);
}

void UsbMidiImpl::sendProgramChange(uint8_t programNumber, uint8_t channel)
{
    send(0xC0, channel, programNumber, 0);
}

void UsbMidiImpl::sendAfterTouch(uint8_t velocity, uint8_t channel)
{
    send(0xD0, channel, velocity, 0);
}

void UsbMidiImpl::sendPolyPressure(uint8_t note, uint8_t value, uint8_t channel)
{
    send(0xA0, channel, note, value);
}

void UsbMidiImpl::sendPitchBend(int value, uint8_t channel)
{
    if (value < -8192)
    {
        value = -8192;
    }
    else if (value > 8191)
    {
        value = 8191;
    }
    value += 8192;
    send(0xE0, channel, value, value >> 7);
}

void UsbMidiImpl::sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel)
{
    send(0x90, channel, note, velocity);
}

void UsbMidiImpl::sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel)
{
    send(0x80, channel, note, velocity);
}

void UsbMidiImpl::setReceiveActions(MidiReceiveActions* receiveActions)
{
    midiReceiveActions = *receiveActions;
}
