# Polyend Tracker Custom Synth Engine SDK - Developer Guide

The **Polyend Tracker Custom Synth Engine SDK** ([`synth/include/PolySynth/sdk/`](../synth/include/PolySynth/sdk/)) lets developers create, register, and customize polyphonic real-time synthesizer engines on **Polyend Tracker+** and **Polyend Tracker Mini** (`NXP i.MX RT1064`, ARM Cortex-M7 @ 600 MHz) without requiring the proprietary `PolyDSP` source code.

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [SDK Headers Reference](#2-sdk-headers-reference)
3. [Voice Lifecycle & Audio Rendering Model](#3-voice-lifecycle--audio-rendering-model)
4. [Step-by-Step Tutorial: Creating a Custom Synth Engine](#4-step-by-step-tutorial-creating-a-custom-synth-engine)
   - [Step 1: Add an `InstrumentTypeId` Enum Value](#step-1-add-an-instrumenttypeid-enum-value)
   - [Step 2: Define Your Parameter Enum](#step-2-define-your-parameter-enum)
   - [Step 3: Implement Your Voice Class](#step-3-implement-your-voice-class)
   - [Step 4: Define Parameters, UI Sections & Default Macros](#step-4-define-parameters-ui-sections--default-macros)
   - [Step 5: Register the Engine in `core/synth/registry.cpp`](#step-5-register-the-engine-in-coresynthregistrycpp)
   - [Step 6: Add UI Badges & Wire into the Tracker UI](#step-6-add-ui-badges--wire-into-the-tracker-ui)
   - [Step 7: Build & Flash `.ptf` Firmware](#step-7-build--flash-ptf-firmware)
5. [Patch Metadata, Curves, Macros & UI Layout Reference](#5-patch-metadata-curves-macros--ui-layout-reference)
6. [DSP Utilities & Value Formatters](#6-dsp-utilities--value-formatters)
7. [Real-Time Audio, CPU & Memory Guidelines](#7-real-time-audio-cpu--memory-guidelines)
8. [Genesis-Style 4-Operator FM: `GEN` (`GenesisSynthVoice`)](#8-genesis-style-4-operator-fm-gen-genesisynthvoice)
9. [Reference Implementation: `CHIP` (`ChiptuneSynthVoice`)](#9-reference-implementation-chip-chiptunesynthvoice)

---

## 1. Architecture Overview

The Tracker synthesizer subsystem uses a hybrid architecture that combines precompiled factory libraries with an open-source C++20 SDK:

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         Tracker Application                             │
│  ┌─────────────────────────┐  ┌──────────────────────────────────────┐  │
│  │ core/synth/Synth.cpp │  │ SynthEditor.cpp / SynthPatchEditor   │  │
│  └────────────┬────────────┘  └──────────────────┬───────────────────┘  │
│               │                                  │                      │
│  ┌────────────▼──────────────────────────────────▼───────────────────┐  │
│  │               core/synth/registry.cpp (Open Source)            │  │
│  │   Registers Factory Engines (ACD, FAT, VAP, WTFM, PERC, DIRT)     │  │
│  │   + Open-Source Custom Engines (CHIP, GEN, your custom engines...)│  │
│  └────────────┬──────────────────────────────────┬───────────────────┘  │
└───────────────┼──────────────────────────────────┼──────────────────────┘
                │                                  │
┌───────────────▼────────────────┐ ┌───────────────▼──────────────────────┐
│  Precompiled Factory Archives  │ │      Open-Source Synth Engines       │
│  • libPolySynth.a (no registry)│ │  • synth/include/PolySynth/sdk/  │
│  • libPolyDSP.a                │ │  • ChiptuneSynthVoice.cpp (CHIP)     │
│  (Stripped, ABI-stable)        │ │  • GenesisSynthVoice.cpp (GEN)       │
│                                │ │  • YourCustomSynthVoice.cpp          │
└────────────────────────────────┘ └──────────────────────────────────────┘
```

- **Factory Synth Libraries (`libPolySynth.a` + `libPolyDSP.a`)**: Provide the core `PolySynth` engine, voice allocator, patch serializer, macro modulation matrix, and the 6 factory synth engines (`ACD`, `FAT`, `VAP`, `WTFM`, `PERC`, `DIRT`).
- **Open-Source Registration ([`core/synth/registry.cpp`](../core/synth/registry.cpp))**: `registry.o` is extracted from `libPolySynth.a` and compiled directly inside `Tracker`, giving firmware developers full control over which synth engines are registered.
- **Zero `PolyDSP` Dependency**: Custom synth engines only include [`PolySynth/sdk/SynthVoiceSDK.h`](../synth/include/PolySynth/sdk/SynthVoiceSDK.h).

---

## 2. SDK Headers Reference

All SDK headers live in [`synth/include/PolySynth/sdk/`](../synth/include/PolySynth/sdk/):

| Header | Purpose |
| :--- | :--- |
| [`InstrumentType.h`](../synth/include/PolySynth/sdk/InstrumentType.h) | Defines `polysynth::InstrumentType` (`name`, `createMetadata`, `createVoice`). |
| [`InstrumentVoice.h`](../synth/include/PolySynth/sdk/InstrumentVoice.h) | Pure virtual voice interface (`onMidiEvent`, `onParamChange`, `renderVoiceAudio`, `renderPostProcessAudio`, note query methods). |
| [`InstrumentTypeRegistry.h`](../synth/include/PolySynth/sdk/InstrumentTypeRegistry.h) | Defines `InstrumentTypeRegistry::addInstrumentType(int id, std::unique_ptr<InstrumentType>)` and `registerInstrumentTypes()`. |
| [`SynthVoiceSDK.h`](../synth/include/PolySynth/sdk/SynthVoiceSDK.h) | Header-only C++20 SDK base class [`CustomSynthVoice`](../synth/include/PolySynth/sdk/SynthVoiceSDK.h#L202-L371), [`ParamHelper`](../synth/include/PolySynth/sdk/SynthVoiceSDK.h#L166-L196), DSP math utilities, standard [`MidiCCs`](../synth/include/PolySynth/sdk/SynthVoiceSDK.h#L80-L112), and value string formatters. |

Supporting public headers in [`synth/include/PolySynth/`](../synth/include/PolySynth/):
- [`Core.h`](../synth/include/PolySynth/Core.h): Fundamental types (`sample_t = float`, `note_t = uint8_t`, `AudioBufferRef`, `AudioBufferStereoRef`, constants `kNoteA4 = 69`, `kMaxMacroCount = 8`, `kUserMacroCount = 6`).
- [`Patch.h`](../synth/include/PolySynth/Patch.h): `Patch` and `Patch::Metadata` (`Param`, `Macro`, `EditorLayout`, `OutputChannels`).
- [`MidiEvent.h`](../synth/include/PolySynth/MidiEvent.h): `MidiEvent` structure passed to `onMidiEvent`.
- [`config.h`](../synth/include/PolySynth/config.h): `InstrumentTypeId` enum.

---

## 3. Voice Lifecycle & Audio Rendering Model

The Tracker audio engine supports **3 active synth slots** (`SYN 1`, `SYN 2`, `SYN 3`) with up to **8 voices total** allocated dynamically across those 3 slots.

### How Voices Are Allocated and Called

1. **Engine Selection**:
   When a user selects your synth engine on `SYN 1`–`SYN 3` (or loads a project/patch using it), `PolySynth`:
   - Calls your static `createMetadata(const Patch&, Patch::Metadata&)` function once to build the parameter table, UI layout, and default macros.
   - Calls your `createVoice()` factory up to `metadata.maxVoices` times (up to 8 voices) to instantiate your `CustomSynthVoice` subclass on the heap.
   - Sends initial `onParamChange(paramId, defaultValue)` calls for every parameter so your voice state is synchronized with the patch.

2. **Parameter & Macro Updates (`onParamChange`)**:
   - Whenever the user turns a knob, changes a macro, sends MIDI CC, or triggers a step FX locking a macro/param, `PolySynth` computes the final modulated parameter value (base parameter + all active macro modifiers + per-voice velocity/aftertouch modifiers) and calls `onParamChange(paramId, newValue)`.
   - [`CustomSynthVoice`](../synth/include/PolySynth/sdk/SynthVoiceSDK.h#L246-L253) automatically dispatches `onParamChange` in $O(1)$ time via a static table of 4-byte function pointers (`paramHandlers`) registered in `createMetadata()`.

3. **Note & Transport Events (`onMidiEvent`)**:
   [`CustomSynthVoice`](../synth/include/PolySynth/sdk/SynthVoiceSDK.h#L213-L244) handles `MidiEvent` messages, maintains a per-voice held-note stack (`heldNotes`, supporting mono/legato note priority and polyphonic voice stealing), tracks `pitchBendSemitones` ($\pm 2$ semitones), and calls your virtual hooks:
   - `onNoteOn(const InstrumentVoiceContext& ctx, note_t note, float velocity, bool isLegatoStep)`:
     - `note`: MIDI note number (`0`–`127`, where `60` = C4 and `69` = A4 = `440 Hz`).
     - `velocity`: Normalized velocity in `[0.0f, 1.0f]`.
     - `isLegatoStep`: `true` when `playMode == PlayMode::kLegato` and a previous note was still held on this voice (allowing you to glide pitch without retriggering envelopes).
   - `onNoteOff(const InstrumentVoiceContext& ctx, note_t note)`:
     - Called when the last held note on this voice is released. If an earlier note was still held (in mono/legato overlap), `CustomSynthVoice` automatically retriggers `onNoteOn` for the previously held note instead!
   - `onStart()` / `onClock()`:
     - Optional hooks for MIDI Start and 24 PPQN MIDI Clock ticks.

4. **Per-Voice Audio Rendering (`renderVoiceAudio`)**:
   Called once per audio block (`128` samples at `44100 Hz`, i.e. every `2.902 ms`) for each allocated voice of the synth slot:
   ```cpp
   void renderVoiceAudio(const InstrumentVoiceContext& ctx,
                         std::optional<AudioBufferStereoRef> inputBuffer,
                         AudioBufferStereoRef outputBuffer,
                         std::span<AudioBufferRef> auxOutBuffer) override;
   ```
   - `ctx.tempo`: Current project tempo in BPM.
   - `outputBuffer`: Pair of `std::span<float>` (`outputBuffer[0]` = Left / Mono channel, `outputBuffer[1]` = Right channel), each `128` samples long.
     - If `metadata.outputChannels == Patch::Metadata::OutputChannels::kMono`, write your voice output to `outputBuffer[0]` (`PolySynth` automatically pans/copies mono voices to stereo at the instrument mixer stage).
     - If `metadata.outputChannels == Patch::Metadata::OutputChannels::kStereo`, write to both `outputBuffer[0]` and `outputBuffer[1]`.
     - **Important**: Always overwrite every sample in `outputBuffer[0]` (and `outputBuffer[1]` if stereo). If `!isActive()`, fill the output buffer with `0.0f` and return.
   - `auxOutBuffer`: Optional per-voice send buffers (`auxOutBuffer[0]` = Delay send, `auxOutBuffer[1]` = Reverb send), only used when `metadata.rendersAuxSends = true` (e.g. `PERC` drum synth where each drum voice has independent send levels). Standard synths leave `metadata.rendersAuxSends = false`, and the Tracker mixer handles Delay/Reverb sends automatically.

5. **Post-Mixdown Audio Rendering (`renderPostProcessAudio`)**:
   ```cpp
   void renderPostProcessAudio(const InstrumentVoiceContext& ctx,
                               AudioBufferStereoRef outputBuffer) override;
   ```
   - Called **once per audio block on the first voice (`voice[0]`) only**, after all voices of the instrument have been summed together into `outputBuffer`.
   - Useful for shared master saturation, master filtering, or bus effects across all voices of the synth.

---

## 4. Step-by-Step Tutorial: Creating a Custom Synth Engine

Below is the complete end-to-end workflow for adding a new synth engine (e.g. `kMySynth` / `"MY"`).

### Step 1: Add an `InstrumentTypeId` Enum Value

Edit [`synth/include/PolySynth/config.h`](../synth/include/PolySynth/config.h) and add your new enum entry immediately before `kLastSynth`, then update `kLastSynth`:

```cpp
enum class InstrumentTypeId
{
    kMidiOut,
    kSimpleAnalogSynth,
    kFatAnalogSynth,
    kWaveTableSynth,
    kVASynth,
    kFMSynth,
    kDrumSynth,
    kShapingSynth,
    kChiptuneSynth,
    kMySynth,
    kLastSynth = kMySynth,
    // Keep FX IDs at 1000+
    kDelay = 1000,
    kReverb,
    kLastFx = kReverb
};
```

> **Compatibility Rule**: Never reorder existing `InstrumentTypeId` values above your entry. Existing project and `.pti` / `.pts` patch files serialize the integer `InstrumentTypeId`.

---

### Step 2: Define Your Parameter Enum

Create your header `core/synth/engines/MySynthVoice.h` and implementation `core/synth/engines/MySynthVoice.cpp`.

Define a contiguous, zero-based `enum class` for your parameters:

```cpp
enum class MySynthParamId : int
{
    // Oscillator
    kWaveform = 0,
    kDetune,
    // Filter
    kCutoff,
    kResonance,
    // Amplifier
    kAttack,
    kRelease,
    // Voice
    kPolyphony,
    kVolume,
};
```

> **Important (`enumutil::EnumMax` Limit)**:
> `CustomSynthVoice` uses `enumutil::EnumMax<ParamIdEnum>::value` to automatically size the static `paramHandlers` array at compile time. By default, `enum_util.h` scans enum values in the range `[-5, 64]`. Keep your enum values contiguous starting at `0` and `<= 64`.

---

### Step 3: Implement Your Voice Class

Inherit from [`polysynth::sdk::CustomSynthVoice<MySynthVoice, MySynthParamId>`](../synth/include/PolySynth/sdk/SynthVoiceSDK.h#L202-L371):

```cpp
// core/synth/engines/MySynthVoice.h
#pragma once

#include "PolySynth/sdk/InstrumentType.h"
#include <memory>

namespace polysynth
{
std::unique_ptr<InstrumentType> createMySynth();
} // namespace polysynth
```

```cpp
// core/synth/engines/MySynthVoice.cpp
#include "MySynthVoice.h"
#include "config.h"

#ifdef FEATURE_SYNTH

#include "PolySynth/sdk/SynthVoiceSDK.h"
#include <algorithm>
#include <cmath>

namespace polysynth
{
namespace
{

enum class Waveform : int
{
    kSaw = 0,
    kSquare = 1,
    kTriangle = 2,
};

const char* toString(Waveform w)
{
    switch (w)
    {
        case Waveform::kSaw:      return "Saw";
        case Waveform::kSquare:   return "Square";
        case Waveform::kTriangle: return "Triangle";
    }
    return "";
}

enum class PolyphonyMode : int
{
    kMono = 0,
    kLegato = 1,
    kPoly = 2,
};

const char* toString(PolyphonyMode m)
{
    switch (m)
    {
        case PolyphonyMode::kMono:   return "Mono";
        case PolyphonyMode::kLegato: return "Legato";
        case PolyphonyMode::kPoly:   return "Poly";
    }
    return "";
}

} // namespace

enum class MySynthParamId : int
{
    kWaveform = 0,
    kDetune,
    kCutoff,
    kResonance,
    kAttack,
    kRelease,
    kPolyphony,
    kVolume,
};

class MySynthVoice : public sdk::CustomSynthVoice<MySynthVoice, MySynthParamId>
{
  public:
    MySynthVoice() = default;

    bool isActive() final { return envStage != EnvStage::kIdle; }

    void onNoteOn(const InstrumentVoiceContext& /*ctx*/,
                  note_t note,
                  float velocity,
                  bool isLegatoStep) final
    {
        currentNote = note;
        noteVelocity = velocity;
        if (!isLegatoStep || envStage == EnvStage::kIdle)
        {
            envStage = EnvStage::kAttack;
        }
    }

    void onNoteOff(const InstrumentVoiceContext& /*ctx*/, note_t /*note*/) final
    {
        if (envStage != EnvStage::kIdle)
        {
            envStage = EnvStage::kRelease;
        }
    }

    void renderVoiceAudio(const InstrumentVoiceContext& /*ctx*/,
                          std::optional<AudioBufferStereoRef> /*inputBuffer*/,
                          AudioBufferStereoRef outputBuffer,
                          std::span<AudioBufferRef> /*auxOutBuffer*/) final
    {
        auto out = outputBuffer[0];
        if (!isActive())
        {
            std::fill(out.begin(), out.end(), 0.0f);
            return;
        }

        const float freq =
          sdk::noteToFreq(currentNote, pitchBendSemitones + detuneCents * 0.01f);
        const float dt = std::clamp(freq / sdk::kSampleRate, 0.0001f, 0.49f);
        const float atkStep = 1.0f / (std::max(0.001f, attackSec) * sdk::kSampleRate);
        const float relStep = 1.0f / (std::max(0.002f, releaseSec) * sdk::kSampleRate);
        const float f = std::clamp(
          2.0f * std::sin(3.14159265f * std::min(cutoffHz, 18000.0f) / sdk::kSampleRate),
          0.005f,
          0.95f);
        const float q = std::clamp(1.0f - resonance * 0.85f, 0.15f, 1.0f);

        for (auto& sample : out)
        {
            // Envelope
            if (envStage == EnvStage::kAttack)
            {
                envLevel += atkStep;
                if (envLevel >= 1.0f)
                {
                    envLevel = 1.0f;
                    envStage = EnvStage::kSustain;
                }
            }
            else if (envStage == EnvStage::kRelease)
            {
                envLevel -= relStep;
                if (envLevel <= 0.0f)
                {
                    envLevel = 0.0f;
                    envStage = EnvStage::kIdle;
                }
            }

            // Oscillator
            phase += dt;
            if (phase >= 1.0f)
            {
                phase -= 1.0f;
            }

            float sig = 0.0f;
            switch (waveform)
            {
                case Waveform::kSaw:
                    sig = 2.0f * phase - 1.0f;
                    break;
                case Waveform::kSquare:
                    sig = (phase < 0.5f) ? 1.0f : -1.0f;
                    break;
                case Waveform::kTriangle:
                    sig = 1.0f - 4.0f * std::fabs(phase - 0.5f);
                    break;
            }

            // 2-pole Chamberlin SVF Lowpass
            svfLow += f * svfBand;
            float high = sig - svfLow - q * svfBand;
            svfBand += f * high;

            sample = std::clamp(svfLow * envLevel * noteVelocity * volumeGain, -1.0f, 1.0f);
        }
    }

    static void createMetadata(Patch& patch, Patch::Metadata& metadata);

  private:
    enum class EnvStage : uint8_t
    {
        kIdle,
        kAttack,
        kSustain,
        kRelease,
    };

    Waveform waveform = Waveform::kSaw;
    float detuneCents = 0.0f;
    float cutoffHz = 8000.0f;
    float resonance = 0.1f;
    float attackSec = 0.005f;
    float releaseSec = 0.25f;
    float volumeGain = sdk::dbToAmplitude(-6.0f);

    note_t currentNote = 60;
    float noteVelocity = 1.0f;
    float phase = 0.0f;
    float envLevel = 0.0f;
    EnvStage envStage = EnvStage::kIdle;
    float svfLow = 0.0f;
    float svfBand = 0.0f;
};
```

---

### Step 4: Define Parameters, UI Sections & Default Macros

Still inside `MySynthVoice.cpp`, implement `MySynthVoice::createMetadata` and the factory function `createMySynth()`:

```cpp
void MySynthVoice::createMetadata(Patch& patch, Patch::Metadata& metadata)
{
    using ParamId = MySynthParamId;
    using Param = Patch::Metadata::Param;
    using EditorSection = Patch::Metadata::EditorLayout::EditorSection;
    using EditorElement = Patch::Metadata::EditorLayout::EditorElement;

    constexpr auto kParamCount = enumutil::EnumMax<ParamId>::value + 1;
    metadata.instrumentTypeId = static_cast<int>(InstrumentTypeId::kMySynth);
    metadata.outputChannels = Patch::Metadata::OutputChannels::kMono;
    metadata.params.resize(kParamCount);
    paramHandlers.resize(kParamCount);

    sdk::ParamHelper<MySynthVoice> helper(metadata, paramHandlers);

    helper.addParam(
      static_cast<int>(ParamId::kWaveform),
      {
        .displayName = "Waveform",
        .defaultValue = static_cast<float>(Waveform::kSaw),
        .minValue = 0,
        .maxValue = enumutil::EnumMax<Waveform>::value,
        .isEnum = true,
        .midiCC = static_cast<uint8_t>(sdk::MidiCCs::kOscParam0),
        .valueFormatter =
          [](const Patch&, Patch::paramvalue_t v)
        { return toString(static_cast<Waveform>(std::lround(v))); },
      },
      [](MySynthVoice& v, Patch::optional_paramvalue_t val)
      { v.waveform = static_cast<Waveform>(std::lround(*val)); });

    helper.addParam(
      static_cast<int>(ParamId::kDetune),
      {
        .displayName = "Fine Tune",
        .defaultValue = 0.0f,
        .minValue = -100.0f,
        .maxValue = 100.0f,
        .midiCC = static_cast<uint8_t>(sdk::MidiCCs::kOscParam0) + 1,
        .valueFormatter = sdk::createCentsFormatter(),
      },
      [](MySynthVoice& v, Patch::optional_paramvalue_t val) { v.detuneCents = *val; });

    helper.addParam(
      static_cast<int>(ParamId::kCutoff),
      {
        .displayName = "Cutoff",
        .defaultValue = 8000.0f,
        .minValue = 40.0f,
        .maxValue = 18000.0f,
        .editCurve = Param::Curve::kExponential,
        .macroOperation = Param::MacroOperation::kMult,
        .midiCC = static_cast<uint8_t>(sdk::MidiCCs::kFilterCutoff),
        .valueFormatter = sdk::createFrequencyFormatter(),
      },
      [](MySynthVoice& v, Patch::optional_paramvalue_t val) { v.cutoffHz = *val; });

    helper.addParam(
      static_cast<int>(ParamId::kResonance),
      {
        .displayName = "Resonance",
        .defaultValue = 0.1f,
        .minValue = 0.0f,
        .maxValue = 1.0f,
        .midiCC = static_cast<uint8_t>(sdk::MidiCCs::kFilterResonance),
        .valueFormatter = sdk::createPercentFormatter(),
      },
      [](MySynthVoice& v, Patch::optional_paramvalue_t val) { v.resonance = *val; });

    helper.addParam(
      static_cast<int>(ParamId::kAttack),
      {
        .displayName = "Attack",
        .defaultValue = 0.005f,
        .minValue = 0.001f,
        .maxValue = 4.0f,
        .editCurve = Param::Curve::kExponential,
        .midiCC = static_cast<uint8_t>(sdk::MidiCCs::kAmpEnvAttack),
        .valueFormatter = sdk::createTimeFormatter(),
      },
      [](MySynthVoice& v, Patch::optional_paramvalue_t val) { v.attackSec = *val; });

    helper.addParam(
      static_cast<int>(ParamId::kRelease),
      {
        .displayName = "Release",
        .defaultValue = 0.25f,
        .minValue = 0.005f,
        .maxValue = 8.0f,
        .editCurve = Param::Curve::kExponential,
        .midiCC = static_cast<uint8_t>(sdk::MidiCCs::kAmpEnvRelease),
        .valueFormatter = sdk::createTimeFormatter(),
      },
      [](MySynthVoice& v, Patch::optional_paramvalue_t val) { v.releaseSec = *val; });

    helper.addParam(
      static_cast<int>(ParamId::kPolyphony),
      {
        .displayName = "Play Mode",
        .defaultValue = static_cast<float>(PolyphonyMode::kPoly),
        .minValue = 0,
        .maxValue = enumutil::EnumMax<PolyphonyMode>::value,
        .isEnum = true,
        .invalidatesOtherMetadata = true, // Re-evaluates maxVoices when changed!
        .valueFormatter =
          [](const Patch&, Patch::paramvalue_t v)
        { return toString(static_cast<PolyphonyMode>(std::lround(v))); },
      },
      [](MySynthVoice& v, Patch::optional_paramvalue_t val)
      {
          auto mode = static_cast<PolyphonyMode>(std::lround(*val));
          v.playMode = (mode == PolyphonyMode::kLegato) ? PlayMode::kLegato : PlayMode::kAlways;
      });

    helper.addParam(
      static_cast<int>(ParamId::kVolume),
      {
        .displayName = "Gain",
        .defaultValue = -6.0f,
        .minValue = -30.0f,
        .maxValue = 6.0f,
        .valueFormatter = [](const Patch&, Patch::paramvalue_t v)
        { return fmt::format("{:.1f} dB", v); },
      },
      [](MySynthVoice& v, Patch::optional_paramvalue_t val)
      { v.volumeGain = sdk::dbToAmplitude(*val); });

    // Dynamic maxVoices based on kPolyphony parameter
    if (auto poly = patch.getParamValue(static_cast<int>(ParamId::kPolyphony)))
    {
        auto mode = static_cast<PolyphonyMode>(std::lround(*poly));
        metadata.maxVoices = (mode == PolyphonyMode::kPoly) ? 8 : 1;
    }

    // Patch Editor UI Sections (layoutFlat)
    auto paramElem = [](ParamId id)
    {
        return EditorElement{ .type = EditorElement::Type::kParam, .id = static_cast<int>(id) };
    };

    metadata.layoutFlat.sections = {
        EditorSection{
          .name = "Oscillator",
          .shortName = "Osc",
          .elements = { paramElem(ParamId::kWaveform), paramElem(ParamId::kDetune) },
        },
        EditorSection{
          .name = "Filter",
          .shortName = "Filter",
          .elements = { paramElem(ParamId::kCutoff), paramElem(ParamId::kResonance) },
        },
        EditorSection{
          .name = "Amplifier",
          .shortName = "Amp",
          .elements = { paramElem(ParamId::kAttack), paramElem(ParamId::kRelease) },
        },
        EditorSection{
          .name = "Voice",
          .shortName = "Voice",
          .elements = { paramElem(ParamId::kPolyphony), paramElem(ParamId::kVolume) },
        },
    };
    metadata.layout23 = metadata.layoutFlat;

    // Default Macros (6 user macros: indices 0..5, plus system velocity macro)
    metadata.initDefaultMacros = [](std::vector<Patch::Metadata::Macro>& macros)
    {
        macros[0] = {
            .displayName = "Cutoff",
            .modifiers = { { static_cast<int>(ParamId::kCutoff), 0.5f } },
        };
        macros[1] = {
            .displayName = "Resonance",
            .modifiers = { { static_cast<int>(ParamId::kResonance), 0.5f } },
        };
        macros[2] = {
            .displayName = "Envelope",
            .modifiers = {
              { static_cast<int>(ParamId::kAttack), 0.3f },
              { static_cast<int>(ParamId::kRelease), 0.5f },
            },
        };
        macros[static_cast<int>(Patch::SystemMacroId::kVelocity)].modifiers = {
            { static_cast<int>(ParamId::kVolume), 1.0f },
        };
    };
}

std::unique_ptr<InstrumentType> createMySynth()
{
    return std::make_unique<InstrumentType>(
      InstrumentType{ .name = "MY",
                      .createMetadata = &MySynthVoice::createMetadata,
                      .createVoice = [](int /*voiceIdx*/) -> std::unique_ptr<InstrumentVoice>
                      { return std::make_unique<MySynthVoice>(); } });
}

} // namespace polysynth
#endif
```

---

### Step 5: Register the Engine in `core/synth/registry.cpp`

Open [`core/synth/registry.cpp`](../core/synth/registry.cpp), include your header, and add your engine to `InstrumentTypeRegistry::registerInstrumentTypes`:

```cpp
#include "engines/MySynthVoice.h"
// ...
void InstrumentTypeRegistry::registerInstrumentTypes(
  InstrumentTypeRegistry& registry,
  [[maybe_unused]] const Synth::Allocator& largeBuffersAlloc)
{
    registry.addInstrumentType(static_cast<int>(InstrumentTypeId::kMidiOut), createMidiOut());
    registry.addInstrumentType(static_cast<int>(InstrumentTypeId::kSimpleAnalogSynth),
                               createSimpleAnalogSynth());
    registry.addInstrumentType(static_cast<int>(InstrumentTypeId::kFatAnalogSynth),
                               createFatAnalogSynth());
    registry.addInstrumentType(static_cast<int>(InstrumentTypeId::kVASynth), createVASynth());
    registry.addInstrumentType(static_cast<int>(InstrumentTypeId::kFMSynth), createFMSynth());
    registry.addInstrumentType(static_cast<int>(InstrumentTypeId::kDrumSynth), createDrumSynth());
    registry.addInstrumentType(static_cast<int>(InstrumentTypeId::kShapingSynth),
                               createShapingSynth());
    registry.addInstrumentType(static_cast<int>(InstrumentTypeId::kChiptuneSynth),
                               createChiptuneSynth());
    registry.addInstrumentType(static_cast<int>(InstrumentTypeId::kMySynth),
                               createMySynth());
}
```

> **Note**: Any `.cpp` file placed in `core/synth/engines/` is automatically discovered and compiled by [`core/makefile`](../core/makefile) via `rwildcard`.

---

### Step 6: Add UI Badges & Wire into the Tracker UI

To expose the new engine in the Tracker's Synth model selector and Patch Editor:

1. **Add to `Synth::kSynthTypes`** in [`core/synth/Synth.h`](../core/synth/Synth.h):
   ```cpp
   static constexpr std::array kSynthTypes = {
       polysynth::InstrumentTypeId::kSimpleAnalogSynth,
       polysynth::InstrumentTypeId::kFatAnalogSynth,
       polysynth::InstrumentTypeId::kVASynth,
       polysynth::InstrumentTypeId::kFMSynth,
       polysynth::InstrumentTypeId::kDrumSynth,
       polysynth::InstrumentTypeId::kShapingSynth,
       polysynth::InstrumentTypeId::kChiptuneSynth,
       polysynth::InstrumentTypeId::kMySynth,
   };
   ```

2. **Create or Reuse Badge Bitmaps (`L2` 2-bit Grayscale Format)**:
   Each synth uses two small `L2` (2 bits per pixel, 4-level grayscale, MSB-first packed) bitmaps in `core/libs/images/`:
   - **Mini Badge (`80x28` pixels, `560` bytes, stride `20`)**: Displayed in the Instrument Parameter screen (`SynthEditor`). See [`chip_mini_nomark_80x28_L2.h`](../core/libs/images/chip_mini_nomark_80x28_L2.h).
   - **Medium Badge (`108x40` pixels, `1080` bytes, stride `27`)**: Displayed in the Patch Editor (`SynthPatchEditor`). See [`chip_med_108x40_L2.h`](../core/libs/images/chip_med_108x40_L2.h).
   - Register the bitmap enum IDs in [`core/libs/NewDisplay/displayStructs.h`](../core/libs/NewDisplay/displayStructs.h#L51-L85) (`enDisplayBitmaps`) and add their FT812 `RAM_G` entries in [`core/libs/NewDisplay/display.cpp`](../core/libs/NewDisplay/display.cpp#L172-L211) (`bitmaps[]`).

3. **Add Name & Bitmap Entries to the Synth Editors**:
   - In [`core/interface/instrumentEditor/SynthEditor.cpp`](../core/interface/instrumentEditor/SynthEditor.cpp#L24-L33):
     Append your engine's short name to `synthModelNames` and its mini bitmap ID to `synthModelListImages`.
   - In [`core/interface/instrumentEditor/SynthPatchEditor.cpp`](../core/interface/instrumentEditor/SynthPatchEditor.cpp#L19-L22):
     Append its medium bitmap ID to `synthModelListImages`.

---

### Step 7: Build & Flash `.ptf` Firmware

From the repository root, build the firmware using the top-level [`Makefile`](../Makefile):

```bash
# Build all three devices (Original Tracker, Tracker+, Tracker Mini)
make all

# Or build only Tracker+ or Tracker Mini:
make plus
make mini
```

The flashable `.ptf` files are generated in `build/`:
- `build/TrackerPlus_CE_1.0.0.ptf`
- `build/TrackerMini_CE_1.0.0.ptf`

Copy the `.ptf` file to the `/Firmware/` directory on your Tracker's SD card and flash via **Config → Firmware → Firmware update**.

---

## 5. Patch Metadata, Curves, Macros & UI Layout Reference

### Parameter Definition (`Patch::Metadata::Param`)

Every parameter in `metadata.params` is configured via [`Patch::Metadata::Param`](../synth/include/PolySynth/Patch.h#L114-L141):

| Field | Type | Description |
| :--- | :--- | :--- |
| `displayName` | `std::string` | Label shown in the Patch Editor parameter list (keep `<= 18` chars for clean UI fitting). |
| `defaultValue` | `optional_paramvalue_t` | Initial value on `Init Patch` and when resetting via **Delete** button. |
| `minValue` | `__fp16` / `float` | Minimum parameter value (note: stored as IEEE 754 half-precision `__fp16` on ARM, max $\pm 65504$). |
| `maxValue` | `__fp16` / `float` | Maximum parameter value (keep within $[-65500, +65500]$). |
| `editCurve` | `Param::Curve` | Response curve for knob turning and macro modulation (`kLinear`, `kExponential`, `kExponentialSteep`, `kPower`). |
| `macroOperation` | `Param::MacroOperation` | How macro modifiers combine with the parameter (`kAdd` = additive in normalized curve space; `kMult` = multiplicative scaling, ideal for filter cutoff). |
| `isEnum` | `bool` | Set `true` for discrete integer options (`0..N`). The UI will step by `1` on knob turns. |
| `midiCC` | `tiny::optional<uint8_t, 0xff>` | Optional MIDI CC number (see [`sdk::MidiCCs`](../synth/include/PolySynth/sdk/SynthVoiceSDK.h#L80-L112)). |
| `invalidatesOtherMetadata` | `bool` | Set `true` if changing this parameter alters `metadata.maxVoices` or dynamic parameter names (triggers `createMetadata` refresh). |
| `valueFormatter` | `std::function<std::string(const Patch&, float)>` | Formats the current parameter value into a human-readable string for the Tracker display. |

### Patch Editor Section Icons (`layoutFlat.sections`)

The Tracker Patch Editor displays a **2x6 grid of tiles**: the **top 6 tiles** represent your engine's parameter sections (`metadata.layoutFlat.sections`, up to 6 sections), and the **bottom 6 tiles** represent the 6 user Macros.

[`SynthPatchEditor.cpp`](../core/interface/instrumentEditor/SynthPatchEditor.cpp#L1178-L1287) automatically selects the tile icon and grid label by matching a substring in `EditorSection::name`:

| `EditorSection::name` Substring | Tile Short Label | Tile Icon | Typical Use |
| :--- | :--- | :--- | :--- |
| `"Oscillator"` | `Osc` | `kBitmapOsc` (Waveform icon) | Main oscillators, wave models, tuning, sub-osc |
| `"Modulation"` | `Mod` | `kBitmapMod` (Mod matrix icon) | LFOs, PWM, vibrato, pitch sweeps, arpeggiator |
| `"Amplifier"` | `Amp` | `kBitmapEnv` (ADSR envelope icon) | Amp ADSR envelope, velocity sensitivity |
| `"Amp Envelope"` | `Amp Env` | `kBitmapEnv` (ADSR envelope icon) | Amplitude ADSR envelope |
| `"Envelope"` | `Env 1`, `Env 2`... | `kBitmapEnv` (Auto-numbered) | Auxiliary/modulation envelopes |
| `"LFO"` | `LFO 1`, `LFO 2`... | `kBitmapLfo` (Auto-numbered) | Dedicated LFO sections |
| `"Filter"` | `Filter` | `kBitmapFilter` (Filter curve icon) | Lowpass/highpass/bandpass filter, bitcrusher |
| `"Shaper"` | `Shp` | `kBitmapShaper` (Waveshaper icon) | Distortion, wavefolder, drive (`DIRT`) |
| `"Voice"` | `Voice` | `kBitmapGear` (Gear icon) | Play mode (`Mono`/`Legato`/`Poly`), glide, gain |
| `"FX"` | `FX` | `kBitmapFX` | Built-in voice/synth effects |
| `"Mixer"` | `Mixer` | `kBitmapMixer` | Oscillator or drum channel mixer |
| `"Kick"`, `"Tom"`, `"Snare"`, `"Hihat"`, `"Cymbal"`, `"Percussion"` | Same as name | Drum icons | Drum synth voices (`PERC`) |

> **Tip**: Always include one of the substrings above in each `EditorSection::name` (and keep `metadata.layoutFlat.sections.size() <= 6`) so every tile in the top half of the 2x6 Patch Editor grid has a matching hardware icon and label.

---

## 6. DSP Utilities & Value Formatters

[`synth/include/PolySynth/sdk/SynthVoiceSDK.h`](../synth/include/PolySynth/sdk/SynthVoiceSDK.h) provides header-only helpers in namespace `polysynth::sdk`:

### Constants & Math Helpers
- `sdk::kSampleRate` (`44100.0f`): Audio engine sample rate in Hz.
- `sdk::kSilenceGainDb` (`-90.0f`): Silence threshold in dB.
- `sdk::dbToAmplitude(float db)`: Converts dB to linear amplitude (`0.0f` at or below `-90 dB`).
- `sdk::amplitudeToDb(float amplitude)`: Converts linear amplitude to dB.
- `sdk::noteToFreq(note_t note, float detuneSemitones = 0.0f)`: Converts MIDI note + fractional semitone offset to frequency in Hz ($A_4 = 440\text{ Hz}$).
- `sdk::detuneMult(float detuneSemitones)`: Returns $2^{\Delta / 12}$ frequency multiplier.
- `sdk::saturateSoftClip(std::span<sample_t> buf, float gainDb)`: Applies pre-gain in dB followed by cubic soft-clipping ($y = x - x^3 / 6.72$, clamped to $[-1, 1]$).

### Built-In Value Formatters
- `sdk::createPercentFormatter(bool showPercentSign = true)`: Formats `[0.0, 1.0]` as `"0 %"` .. `"100 %"`.
- `sdk::createTimeFormatter()`: Formats seconds as `"0.05 s"` (`< 1 s`) or `"2.5 s"` (`>= 1 s`).
- `sdk::createFrequencyFormatter(float max)`: Formats Hz/kHz automatically (`"8.50 Hz"`, `"440 Hz"`, `"12.5 kHz"`).
- `sdk::createSemitoneFormatter()`: Formats integer semitones with sign (`"-12 st"`, `"0 st"`, `"+7 st"`).
- `sdk::createCentsFormatter()`: Formats integer cents with sign (`"-50 ct"`, `"0 ct"`, `"+25 ct"`).

---

## 7. Real-Time Audio, CPU & Memory Guidelines

1. **Stateless Parameter Handlers (`static_assert(std::is_empty_v<Fn>)`)**:
   `sdk::ParamHelper::addParam` stores each parameter handler as a 4-byte C function pointer (`void (*)(VoiceType&, Patch::optional_paramvalue_t)`).
   - Handler lambdas **must not capture variables** (`[]` only).
   - Because of this design, a custom synth engine's static `.bss` footprint in `SRAM_DTC` is only **~100 bytes** (`4 bytes * numParams`), leaving plenty of `SRAM_DTC` headroom for multiple custom engines.

2. **Zero Heap Allocations on the Audio Thread**:
   - Never call `new`, `malloc`, `std::vector::push_back`, or `std::string` inside `renderVoiceAudio()`, `renderPostProcessAudio()`, `onNoteOn()`, `onNoteOff()`, or `onParamChange()`.
   - Allocate all per-voice state directly as value members of your `CustomSynthVoice` subclass. Voice objects are only allocated on the heap when your engine is actively selected on one of the 3 synth slots.

3. **Keep `minValue` / `maxValue` Within `__fp16` Range**:
   On ARM targets, `Patch::Metadata::Param::minValue` and `maxValue` are stored as 16-bit half-precision floats (`__fp16`, max magnitude `65504.0f`). Never use `FLT_MAX` or values above `65500.0f` for parameter bounds.

4. **CPU Budget (`600 MHz` Cortex-M7 with Hardware Single-Precision FPU)**:
   - Use `float` (`0.0f`, `std::sinf`, `std::powf`, `std::fabsf`) everywhere, never `double`.
   - Hoist per-block coefficient calculations (such as filter coefficients, envelope increments, and `noteToFreq` base pitch when not pitch-modulated at audio rate) outside the `128`-sample `for (auto& sample : out)` loop whenever possible.
   - Use `PolyBLEP` or band-limiting on sharp discontinuities (saw/pulse resets) to keep high notes alias-free at `44.1 kHz`.

---

## 8. Genesis-Style 4-Operator FM: `GEN` (`GenesisSynthVoice`)

The open-source **`GEN`** engine is a self-contained, per-voice four-operator FM synth implemented with the same SDK pattern:

- **Header**: [`core/synth/engines/GenesisSynthVoice.h`](../core/synth/engines/GenesisSynthVoice.h)
- **Implementation**: [`core/synth/engines/GenesisSynthVoice.cpp`](../core/synth/engines/GenesisSynthVoice.cpp)
- Provides eight operator-routing algorithms, operator multiplier/detune/level and envelope controls, channel feedback, and LFO modulation.

## 9. Reference Implementation: `CHIP` (`ChiptuneSynthVoice`)

For a full-featured production example built exclusively with `PolySynth/sdk/SynthVoiceSDK.h`, study the open-source **`CHIP`** synth engine:

- **Header**: [`core/synth/engines/ChiptuneSynthVoice.h`](../core/synth/engines/ChiptuneSynthVoice.h)
- **Implementation**: [`core/synth/engines/ChiptuneSynthVoice.cpp`](../core/synth/engines/ChiptuneSynthVoice.cpp)
- **Memory Footprint**: `21.4 KB` Flash (`QSPI_FLASH`), `105 B` static RAM (`SRAM_DTC`), `224 B` per active voice on the heap.

### What `ChiptuneSynthVoice.cpp` Demonstrates
- **8 Distinct Synthesis Models in One Voice Class**:
  1. `2A03 Pulse` (PolyBLEP variable-duty pulse with LFO PWM)
  2. `SID Saw/Tri` (Combined saw/triangle with sub-octave ring modulation)
  3. `Noise / LFSR` (15-bit NES/GameBoy pitch-tracked LFSR with white & metallic periodic modes)
  4. `FM Bell` (2-operator quantized OPL-style FM with 8 carrier:modulator ratios)
  5. `Super Square` (3-oscillator detuned pulse unison stack)
  6. `Zap / Laser` (Arcade pitch-drop percussion/lead)
  7. `Arp Chord` (4-step 50/60 Hz chiptune chord arpeggiator with 7 chord tables)
  8. `Chip Drum` (Stepped triangle pitch sweep + LFSR noise burst morphing from Kick to Snare to Hat)
- **Dynamic Voice Allocation**: Switching `Play Mode` (`Mono` / `Legato` / `Poly`) uses `.invalidatesOtherMetadata = true` to dynamically switch `metadata.maxVoices` between `1` and `8`.
- **5 Patch Editor Sections** (`Oscillator`, `Modulation`, `Amplifier`, `Filter`, `Voice`) and **6 Pre-Mapped Performance Macros** (`Timbre / PWM`, `Arp / Chord`, `Sweep / Vibrato`, `Env Decay / Rel`, `Crush / Lo-Fi`, `Filter Cutoff`).
