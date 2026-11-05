#include "config.h"
#ifdef FEATURE_SYNTH

#include "GenesisSynthVoice.h"
#include "PolySynth/sdk/SynthVoiceSDK.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

namespace polysynth
{
namespace
{

enum class GenesisParamId : int
{
    kAlgorithm = 0,
    kFeedback,
    kLfoRate,
    kFrequencyModulation,
    kAmplitudeModulation,
    kTune,
    kFine,
    kVolume,
    kOp1Multiplier,
    kOp1Detune,
    kOp1TotalLevel,
    kOp1Attack,
    kOp1Decay,
    kOp1SustainRate,
    kOp1SustainLevel,
    kOp1Release,
    kOp2Multiplier,
    kOp2Detune,
    kOp2TotalLevel,
    kOp2Attack,
    kOp2Decay,
    kOp2SustainRate,
    kOp2SustainLevel,
    kOp2Release,
    kOp3Multiplier,
    kOp3Detune,
    kOp3TotalLevel,
    kOp3Attack,
    kOp3Decay,
    kOp3SustainRate,
    kOp3SustainLevel,
    kOp3Release,
    kOp4Multiplier,
    kOp4Detune,
    kOp4TotalLevel,
    kOp4Attack,
    kOp4Decay,
    kOp4SustainRate,
    kOp4SustainLevel,
    kOp4Release,
};

enum class OperatorParam : uint8_t
{
    kMultiplier,
    kDetune,
    kTotalLevel,
    kAttack,
    kDecay,
    kSustainRate,
    kSustainLevel,
    kRelease,
};

enum class EnvelopeStage : uint8_t
{
    kIdle,
    kAttack,
    kDecay,
    kSustain,
    kRelease,
};

static constexpr std::array<const char*, 8> kAlgorithmNames = {
    "Serial",
    "2 Mod -> 1 Car",
    "Split -> 1 Car",
    "2 Mod -> 1 Car",
    "2 x 2-op",
    "1 Mod -> 3 Car",
    "1 Mod + 3 Car",
    "4 Carriers",
};

static constexpr std::array<const char*, 4> kOperatorNames = {
    "OP1", "OP2", "OP3", "OP4",
};

static constexpr std::array<const char*, 8> kDetuneNames = {
    "0", "+1", "+2", "+3", "0", "-1", "-2", "-3",
};

class GenesisSynthVoice : public sdk::CustomSynthVoice<GenesisSynthVoice, GenesisParamId>
{
  public:
    GenesisSynthVoice()
    {
        for (auto& op : operators)
        {
            updateEnvelopeRates(op);
            updateFrequencyRatio(op);
        }
    }

    bool isActive() final
    {
        return std::any_of(operators.begin(), operators.end(), [](const auto& op)
        { return op.stage != EnvelopeStage::kIdle; });
    }

    void onNoteOn(const InstrumentVoiceContext& /*ctx*/,
                  note_t note,
                  float velocity,
                  bool isLegatoStep) final
    {
        currentNote = static_cast<float>(note);
        noteVelocity = velocity;
        if (isLegatoStep)
        {
            return;
        }

        feedbackHistory = {};
        lfoPhase = 0.0f;
        for (auto& op : operators)
        {
            op.phase = 0.0f;
            op.envelope = 0.0f;
            op.stage = EnvelopeStage::kAttack;
        }
    }

    void onNoteOff(const InstrumentVoiceContext& /*ctx*/, note_t /*note*/) final
    {
        for (auto& op : operators)
        {
            if (op.stage != EnvelopeStage::kIdle)
            {
                op.stage = EnvelopeStage::kRelease;
            }
        }
    }

    void renderVoiceAudio(const InstrumentVoiceContext& /*ctx*/,
                          [[maybe_unused]] std::optional<AudioBufferStereoRef> inputBuffer,
                          AudioBufferStereoRef outputBuffer,
                          std::span<AudioBufferRef> /*auxOutBuffer*/) final
    {
        auto out = outputBuffer[0];
        if (!isActive())
        {
            std::fill(out.begin(), out.end(), 0.0f);
            return;
        }

        constexpr float sampleRate = sdk::kSampleRate;
        constexpr float invSampleRate = 1.0f / sampleRate;
        constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;
        constexpr float modulationIndex = 5.0f;
        const float feedbackAmount =
          (feedback == 0) ? 0.0f : std::pow(2.0f, static_cast<float>(feedback - 1)) * 0.1f;
        const float pitchModDepth = static_cast<float>(frequencyModulation) * (2.0f / 7.0f);
        const float baseNote = currentNote + tuneSemitones + (fineCents * 0.01f) +
                               pitchBendSemitones;
        const float noteFrequency = sdk::noteToFreq(0, baseNote);
        static constexpr std::array<float, 4> kAmplitudeModDepth = { 0.0f, 0.05f, 0.12f, 0.25f };

        for (auto& sample : out)
        {
            lfoPhase += lfoRate * invSampleRate;
            if (lfoPhase >= 1.0f)
            {
                lfoPhase -= std::floor(lfoPhase);
            }
            const float lfo = std::sin(twoPi * lfoPhase);
            const float pitchLfoRatio =
              (pitchModDepth > 0.0f) ? sdk::detuneMult(lfo * pitchModDepth) : 1.0f;
            const float baseFrequency = std::clamp(noteFrequency * pitchLfoRatio, 16.0f, 18000.0f);
            const float amplitudeMod =
              1.0f - kAmplitudeModDepth[static_cast<size_t>(amplitudeModulation)] *
                       std::max(0.0f, lfo);

            for (auto& op : operators)
            {
                updateEnvelope(op);
            }

            const float feedbackMod =
              (feedbackHistory[0] + feedbackHistory[1]) * feedbackAmount;
            const float op1 = renderOperator(0, baseFrequency, feedbackMod, amplitudeMod);
            feedbackHistory[0] = feedbackHistory[1];
            feedbackHistory[1] = op1;

            const float op2Mod = (algorithm == 1 || algorithm == 2 || algorithm == 7)
                                   ? 0.0f
                                   : op1 * modulationIndex;
            const float op2 = renderOperator(1, baseFrequency, op2Mod, amplitudeMod);

            float op3Mod = 0.0f;
            switch (algorithm)
            {
                case 0:
                case 2:
                    op3Mod = op2 * modulationIndex;
                    break;
                case 1:
                    op3Mod = (op1 + op2) * modulationIndex;
                    break;
                case 5:
                    op3Mod = op1 * modulationIndex;
                    break;
                default:
                    break;
            }
            const float op3 = renderOperator(2, baseFrequency, op3Mod, amplitudeMod);

            float op4Mod = 0.0f;
            switch (algorithm)
            {
                case 0:
                case 1:
                    op4Mod = op3 * modulationIndex;
                    break;
                case 2:
                    op4Mod = (op1 + op3) * modulationIndex;
                    break;
                case 3:
                    op4Mod = (op2 + op3) * modulationIndex;
                    break;
                case 4:
                    op4Mod = op3 * modulationIndex;
                    break;
                case 5:
                    op4Mod = op1 * modulationIndex;
                    break;
                default:
                    break;
            }
            const float op4 = renderOperator(3, baseFrequency, op4Mod, amplitudeMod);

            float mixed = 0.0f;
            int carrierCount = 0;
            switch (algorithm)
            {
                case 0:
                case 1:
                case 2:
                case 3:
                    mixed = op4;
                    carrierCount = 1;
                    break;
                case 4:
                    mixed = op2 + op4;
                    carrierCount = 2;
                    break;
                case 5:
                    mixed = op2 + op3 + op4;
                    carrierCount = 3;
                    break;
                case 6:
                    mixed = op2 + op3 + op4;
                    carrierCount = 3;
                    break;
                case 7:
                    mixed = op1 + op2 + op3 + op4;
                    carrierCount = 4;
                    break;
            }
            sample = (mixed / static_cast<float>(carrierCount)) * noteVelocity * volume;
        }
    }

    static void createMetadata(const Patch& /*patch*/, Patch::Metadata& metadata)
    {
        using namespace polysynth::sdk;
        metadata.params.resize(enumutil::EnumMax<GenesisParamId>::value + 1);
        metadata.params.shrink_to_fit();

        if (paramHandlers.empty())
        {
            paramHandlers.resize(enumutil::EnumMax<GenesisParamId>::value + 1);
        }
        ParamHelper<GenesisSynthVoice> helper(metadata, paramHandlers);

        helper.addParam(
          static_cast<int>(GenesisParamId::kAlgorithm),
          {
            .displayName = "Algorithm",
            .defaultValue = 0,
            .minValue = 0,
            .maxValue = 7,
            .isEnum = true,
            .midiCC = *MidiCCs::kOscParam0,
            .valueFormatter =
              [](const Patch& /*patch*/, Patch::paramvalue_t value)
            {
                const int idx = std::clamp(static_cast<int>(std::lround(value)), 0, 7);
                return std::string(kAlgorithmNames[static_cast<size_t>(idx)]);
            },
          },
          [](auto& voice, Patch::optional_paramvalue_t value)
          { voice.algorithm = std::clamp(static_cast<int>(std::lround(*value)), 0, 7); });

        helper.addParam(
          static_cast<int>(GenesisParamId::kFeedback),
          {
            .displayName = "Feedback",
            .defaultValue = 0,
            .minValue = 0,
            .maxValue = 7,
            .midiCC = *MidiCCs::kOscParam0 + 1,
          },
          [](auto& voice, Patch::optional_paramvalue_t value)
          { voice.feedback = std::clamp(static_cast<int>(std::lround(*value)), 0, 7); });

        helper.addParam(
          static_cast<int>(GenesisParamId::kLfoRate),
          {
            .displayName = "LFO Rate",
            .defaultValue = 5.0f,
            .minValue = 0.1f,
            .maxValue = 20.0f,
            .editCurve = Patch::Metadata::Param::Curve::kExponential,
            .midiCC = *MidiCCs::kLfo1Frequency,
            .valueFormatter = createFrequencyFormatter(),
          },
          [](auto& voice, Patch::optional_paramvalue_t value) { voice.lfoRate = *value; });

        helper.addParam(
          static_cast<int>(GenesisParamId::kFrequencyModulation),
          {
            .displayName = "Pitch LFO",
            .defaultValue = 0,
            .minValue = 0,
            .maxValue = 7,
            .midiCC = *MidiCCs::kOscParam0 + 2,
          },
          [](auto& voice, Patch::optional_paramvalue_t value)
          {
              voice.frequencyModulation =
                std::clamp(static_cast<int>(std::lround(*value)), 0, 7);
          });

        helper.addParam(
          static_cast<int>(GenesisParamId::kAmplitudeModulation),
          {
            .displayName = "Amp LFO",
            .defaultValue = 0,
            .minValue = 0,
            .maxValue = 3,
            .midiCC = *MidiCCs::kOscParam0 + 3,
          },
          [](auto& voice, Patch::optional_paramvalue_t value)
          {
              voice.amplitudeModulation =
                std::clamp(static_cast<int>(std::lround(*value)), 0, 3);
          });

        helper.addParam(
          static_cast<int>(GenesisParamId::kTune),
          {
            .displayName = "Tune",
            .defaultValue = 0,
            .minValue = -24,
            .maxValue = 24,
            .midiCC = *MidiCCs::kOscParam0 + 4,
            .valueFormatter = createSemitoneFormatter(),
          },
          [](auto& voice, Patch::optional_paramvalue_t value)
          { voice.tuneSemitones = *value; });

        helper.addParam(
          static_cast<int>(GenesisParamId::kFine),
          {
            .displayName = "Fine",
            .defaultValue = 0,
            .minValue = -100,
            .maxValue = 100,
            .midiCC = *MidiCCs::kOscParam0 + 5,
            .valueFormatter = createCentsFormatter(),
          },
          [](auto& voice, Patch::optional_paramvalue_t value) { voice.fineCents = *value; });

        helper.addParam(
          static_cast<int>(GenesisParamId::kVolume),
          {
            .displayName = "Volume",
            .defaultValue = 1.0f,
            .minValue = 0.0f,
            .maxValue = 2.0f,
            .macroOperation = Patch::Metadata::Param::MacroOperation::kMult,
            .midiCC = *MidiCCs::kOscParam0 + 6,
            .valueFormatter = createPercentFormatter(),
          },
          [](auto& voice, Patch::optional_paramvalue_t value) { voice.volume = *value; });

        for (size_t op = 0; op < 4; ++op)
        {
            const int base = static_cast<int>(GenesisParamId::kOp1Multiplier) +
                             static_cast<int>(op) * 8;
            const std::string prefix = kOperatorNames[op];
            switch (op)
            {
                case 0:
                    addOperatorParam<0, OperatorParam::kMultiplier>(helper, base, prefix + " MUL");
                    addOperatorParam<0, OperatorParam::kDetune>(helper, base + 1, prefix + " DT");
                    addOperatorParam<0, OperatorParam::kTotalLevel>(helper, base + 2, prefix + " TL");
                    addOperatorParam<0, OperatorParam::kAttack>(helper, base + 3, prefix + " AR");
                    addOperatorParam<0, OperatorParam::kDecay>(helper, base + 4, prefix + " DR");
                    addOperatorParam<0, OperatorParam::kSustainRate>(helper, base + 5, prefix + " SR");
                    addOperatorParam<0, OperatorParam::kSustainLevel>(helper, base + 6, prefix + " SL");
                    addOperatorParam<0, OperatorParam::kRelease>(helper, base + 7, prefix + " RR");
                    break;
                case 1:
                    addOperatorParam<1, OperatorParam::kMultiplier>(helper, base, prefix + " MUL");
                    addOperatorParam<1, OperatorParam::kDetune>(helper, base + 1, prefix + " DT");
                    addOperatorParam<1, OperatorParam::kTotalLevel>(helper, base + 2, prefix + " TL");
                    addOperatorParam<1, OperatorParam::kAttack>(helper, base + 3, prefix + " AR");
                    addOperatorParam<1, OperatorParam::kDecay>(helper, base + 4, prefix + " DR");
                    addOperatorParam<1, OperatorParam::kSustainRate>(helper, base + 5, prefix + " SR");
                    addOperatorParam<1, OperatorParam::kSustainLevel>(helper, base + 6, prefix + " SL");
                    addOperatorParam<1, OperatorParam::kRelease>(helper, base + 7, prefix + " RR");
                    break;
                case 2:
                    addOperatorParam<2, OperatorParam::kMultiplier>(helper, base, prefix + " MUL");
                    addOperatorParam<2, OperatorParam::kDetune>(helper, base + 1, prefix + " DT");
                    addOperatorParam<2, OperatorParam::kTotalLevel>(helper, base + 2, prefix + " TL");
                    addOperatorParam<2, OperatorParam::kAttack>(helper, base + 3, prefix + " AR");
                    addOperatorParam<2, OperatorParam::kDecay>(helper, base + 4, prefix + " DR");
                    addOperatorParam<2, OperatorParam::kSustainRate>(helper, base + 5, prefix + " SR");
                    addOperatorParam<2, OperatorParam::kSustainLevel>(helper, base + 6, prefix + " SL");
                    addOperatorParam<2, OperatorParam::kRelease>(helper, base + 7, prefix + " RR");
                    break;
                case 3:
                    addOperatorParam<3, OperatorParam::kMultiplier>(helper, base, prefix + " MUL");
                    addOperatorParam<3, OperatorParam::kDetune>(helper, base + 1, prefix + " DT");
                    addOperatorParam<3, OperatorParam::kTotalLevel>(helper, base + 2, prefix + " TL");
                    addOperatorParam<3, OperatorParam::kAttack>(helper, base + 3, prefix + " AR");
                    addOperatorParam<3, OperatorParam::kDecay>(helper, base + 4, prefix + " DR");
                    addOperatorParam<3, OperatorParam::kSustainRate>(helper, base + 5, prefix + " SR");
                    addOperatorParam<3, OperatorParam::kSustainLevel>(helper, base + 6, prefix + " SL");
                    addOperatorParam<3, OperatorParam::kRelease>(helper, base + 7, prefix + " RR");
                    break;
            }
        }

        using EditorElement = Patch::Metadata::EditorLayout::EditorElement;
        metadata.layoutFlat = Patch::Metadata::EditorLayout{
            .sections = {
              {
                .name = "Channel",
                .shortName = "Chan",
                .elements = {
                  { .type = EditorElement::Type::kParam, .id = static_cast<int>(GenesisParamId::kAlgorithm) },
                  { .type = EditorElement::Type::kParam, .id = static_cast<int>(GenesisParamId::kFeedback) },
                  { .type = EditorElement::Type::kParam, .id = static_cast<int>(GenesisParamId::kLfoRate) },
                  { .type = EditorElement::Type::kParam, .id = static_cast<int>(GenesisParamId::kFrequencyModulation) },
                  { .type = EditorElement::Type::kParam, .id = static_cast<int>(GenesisParamId::kAmplitudeModulation) },
                  { .type = EditorElement::Type::kParam, .id = static_cast<int>(GenesisParamId::kTune) },
                  { .type = EditorElement::Type::kParam, .id = static_cast<int>(GenesisParamId::kFine) },
                  { .type = EditorElement::Type::kParam, .id = static_cast<int>(GenesisParamId::kVolume) },
                },
              },
              {
                .name = "Operator 1",
                .shortName = "OP1",
                .elements = operatorElements(0),
              },
              {
                .name = "Operator 2",
                .shortName = "OP2",
                .elements = operatorElements(1),
              },
              {
                .name = "Operator 3",
                .shortName = "OP3",
                .elements = operatorElements(2),
              },
              {
                .name = "Operator 4",
                .shortName = "OP4",
                .elements = operatorElements(3),
              },
            },
        };
        metadata.layout23 = metadata.layoutFlat;
        metadata.initDefaultMacros = [](auto& macros)
        {
            macros[0] = Patch::Metadata::Macro{
                .displayName = "Feedback",
                .modifiers = { { static_cast<int>(GenesisParamId::kFeedback), 1.0f } },
            };
            macros[1] = Patch::Metadata::Macro{
                .displayName = "Pitch LFO",
                .modifiers = { { static_cast<int>(GenesisParamId::kFrequencyModulation), 1.0f } },
            };
            macros[2] = Patch::Metadata::Macro{
                .displayName = "OP1 Level",
                .modifiers = { { static_cast<int>(GenesisParamId::kOp1TotalLevel), -0.5f } },
                .bipolar = true,
            };
            macros[3] = Patch::Metadata::Macro{
                .displayName = "OP2 Level",
                .modifiers = { { static_cast<int>(GenesisParamId::kOp2TotalLevel), -0.5f } },
                .bipolar = true,
            };
            macros[4] = Patch::Metadata::Macro{
                .displayName = "OP3 Level",
                .modifiers = { { static_cast<int>(GenesisParamId::kOp3TotalLevel), -0.5f } },
                .bipolar = true,
            };
            macros[5] = Patch::Metadata::Macro{
                .displayName = "OP4 Level",
                .modifiers = { { static_cast<int>(GenesisParamId::kOp4TotalLevel), -0.5f } },
                .bipolar = true,
            };
            macros[*Patch::SystemMacroId::kVelocity].modifiers = {
                { static_cast<int>(GenesisParamId::kVolume), 1.0f },
            };
        };

        metadata.instrumentTypeId = static_cast<int>(InstrumentTypeId::kGenesisSynth);
        metadata.outputChannels = Patch::Metadata::OutputChannels::kMono;
    }

  private:
    struct Operator
    {
        float phase = 0.0f;
        float envelope = 0.0f;
        float levelGain = 1.0f;
        float sustainLevelGain = 0.0f;
        float frequencyRatio = 1.0f;
        float attackCoefficient = 0.0f;
        float decayCoefficient = 0.0f;
        float sustainRateCoefficient = 0.0f;
        float releaseCoefficient = 0.0f;
        int multiplier = 1;
        int detune = 0;
        int totalLevel = 0;
        int attack = 31;
        int decay = 8;
        int sustainRate = 0;
        int sustainLevel = 0;
        int release = 8;
        EnvelopeStage stage = EnvelopeStage::kIdle;
    };

    template<size_t OperatorIndex, OperatorParam Parameter>
    static void addOperatorParam(sdk::ParamHelper<GenesisSynthVoice>& helper,
                                 int id,
                                 std::string displayName)
    {
        if constexpr (Parameter == OperatorParam::kMultiplier)
        {
            helper.addParam(
              id,
              {
                .displayName = std::move(displayName),
                .defaultValue = 1,
                .minValue = 0,
                .maxValue = 15,
                .valueFormatter =
                  [](const Patch& /*patch*/, Patch::paramvalue_t value)
                {
                    const int multiplier =
                      std::clamp(static_cast<int>(std::lround(value)), 0, 15);
                    return (multiplier == 0) ? std::string("0.5x")
                                             : std::to_string(multiplier) + "x";
                  },
              },
              [](auto& voice, Patch::optional_paramvalue_t value)
              { voice.setOperatorParameter(OperatorIndex, Parameter, *value); });
        }
        else if constexpr (Parameter == OperatorParam::kDetune)
        {
            helper.addParam(
              id,
              {
                .displayName = std::move(displayName),
                .defaultValue = 0,
                .minValue = 0,
                .maxValue = 7,
                .isEnum = true,
                .valueFormatter =
                  [](const Patch& /*patch*/, Patch::paramvalue_t value)
                {
                    const int detune = std::clamp(static_cast<int>(std::lround(value)), 0, 7);
                    return std::string(kDetuneNames[static_cast<size_t>(detune)]);
                  },
              },
              [](auto& voice, Patch::optional_paramvalue_t value)
              { voice.setOperatorParameter(OperatorIndex, Parameter, *value); });
        }
        else
        {
            constexpr int maxValue = Parameter == OperatorParam::kTotalLevel
                                       ? 127
                                       : ((Parameter == OperatorParam::kSustainLevel ||
                                           Parameter == OperatorParam::kRelease)
                                            ? 15
                                            : 31);
            constexpr float defaultValue =
              Parameter == OperatorParam::kTotalLevel ? 0.0f :
              Parameter == OperatorParam::kAttack ? 31.0f :
              Parameter == OperatorParam::kSustainRate ? 0.0f :
              Parameter == OperatorParam::kSustainLevel ? 0.0f : 8.0f;
            helper.addParam(
              id,
              {
                .displayName = std::move(displayName),
                .defaultValue = defaultValue,
                .minValue = 0,
                .maxValue = maxValue,
                .valueFormatter =
                  [](const Patch& /*patch*/, Patch::paramvalue_t value)
                { return std::to_string(static_cast<int>(std::lround(value))); },
              },
              [](auto& voice, Patch::optional_paramvalue_t value)
              { voice.setOperatorParameter(OperatorIndex, Parameter, *value); });
        }
    }

    static std::vector<Patch::Metadata::EditorLayout::EditorElement> operatorElements(size_t op)
    {
        using EditorElement = Patch::Metadata::EditorLayout::EditorElement;
        const int base = static_cast<int>(GenesisParamId::kOp1Multiplier) +
                         static_cast<int>(op) * 8;
        std::vector<EditorElement> elements;
        elements.reserve(8);
        for (int param = 0; param < 8; ++param)
        {
            elements.push_back(
              { .type = EditorElement::Type::kParam, .id = base + param });
        }
        return elements;
    }

    static float rateCoefficient(int rate, float slowestSeconds, float fastestSeconds)
    {
        if (rate <= 0)
        {
            return 0.0f;
        }
        const float normalized = static_cast<float>(rate - 1) / 30.0f;
        const float seconds =
          slowestSeconds * std::pow(fastestSeconds / slowestSeconds, normalized);
        return 1.0f - std::exp(-1.0f / (seconds * sdk::kSampleRate));
    }

    static void updateEnvelopeRates(Operator& op)
    {
        op.attackCoefficient = rateCoefficient(op.attack, 5.0f, 0.0025f);
        op.decayCoefficient = rateCoefficient(op.decay, 6.0f, 0.015f);
        op.sustainRateCoefficient = rateCoefficient(op.sustainRate, 6.0f, 0.015f);
        op.sustainLevelGain =
          (op.sustainLevel == 15)
            ? 0.000032f
            : std::pow(10.0f, -6.0f * static_cast<float>(op.sustainLevel) / 20.0f);
        const float releaseSeconds = 4.0f * std::pow(0.76f, static_cast<float>(op.release));
        op.releaseCoefficient =
          1.0f - std::exp(-1.0f / (std::max(0.015f, releaseSeconds) * sdk::kSampleRate));
    }

    static void updateFrequencyRatio(Operator& op)
    {
        static constexpr std::array<float, 8> kDetuneCents = {
            0.0f, 7.5f, 15.0f, 22.5f, 0.0f, -7.5f, -15.0f, -22.5f,
        };
        const float multiplier = (op.multiplier == 0) ? 0.5f : static_cast<float>(op.multiplier);
        op.frequencyRatio =
          multiplier * sdk::detuneMult(kDetuneCents[static_cast<size_t>(op.detune)] / 100.0f);
    }

    void setOperatorParameter(size_t index, OperatorParam parameter, float value)
    {
        auto& op = operators[index];
        switch (parameter)
        {
            case OperatorParam::kMultiplier:
                op.multiplier = std::clamp(static_cast<int>(std::lround(value)), 0, 15);
                updateFrequencyRatio(op);
                break;
            case OperatorParam::kDetune:
                op.detune = std::clamp(static_cast<int>(std::lround(value)), 0, 7);
                updateFrequencyRatio(op);
                break;
            case OperatorParam::kTotalLevel:
                op.totalLevel = std::clamp(static_cast<int>(std::lround(value)), 0, 127);
                op.levelGain = std::pow(10.0f, -0.75f * static_cast<float>(op.totalLevel) / 20.0f);
                break;
            case OperatorParam::kAttack:
                op.attack = std::clamp(static_cast<int>(std::lround(value)), 0, 31);
                break;
            case OperatorParam::kDecay:
                op.decay = std::clamp(static_cast<int>(std::lround(value)), 0, 31);
                break;
            case OperatorParam::kSustainRate:
                op.sustainRate = std::clamp(static_cast<int>(std::lround(value)), 0, 31);
                break;
            case OperatorParam::kSustainLevel:
                op.sustainLevel = std::clamp(static_cast<int>(std::lround(value)), 0, 15);
                break;
            case OperatorParam::kRelease:
                op.release = std::clamp(static_cast<int>(std::lround(value)), 0, 15);
                break;
        }
        updateEnvelopeRates(op);
    }

    static void updateEnvelope(Operator& op)
    {
        switch (op.stage)
        {
            case EnvelopeStage::kAttack:
                op.envelope += (1.0f - op.envelope) * op.attackCoefficient;
                if (op.envelope >= 0.999f)
                {
                    op.envelope = 1.0f;
                    op.stage = EnvelopeStage::kDecay;
                }
                break;
            case EnvelopeStage::kDecay:
                op.envelope += (op.sustainLevelGain - op.envelope) * op.decayCoefficient;
                if (std::abs(op.envelope - op.sustainLevelGain) < 0.001f)
                {
                    op.envelope = op.sustainLevelGain;
                    op.stage = EnvelopeStage::kSustain;
                }
                break;
            case EnvelopeStage::kSustain:
                op.envelope -= op.envelope * op.sustainRateCoefficient;
                break;
            case EnvelopeStage::kRelease:
                op.envelope -= op.envelope * op.releaseCoefficient;
                if (op.envelope < 0.0001f)
                {
                    op.envelope = 0.0f;
                    op.stage = EnvelopeStage::kIdle;
                }
                break;
            case EnvelopeStage::kIdle:
                break;
        }
    }

    float renderOperator(size_t index, float baseFrequency, float modulation, float amplitudeMod)
    {
        auto& op = operators[index];
        const float frequency = baseFrequency * op.frequencyRatio;
        op.phase += std::min(frequency, 18000.0f) / sdk::kSampleRate;
        if (op.phase >= 1.0f)
        {
            op.phase -= std::floor(op.phase);
        }
        return std::sin(2.0f * std::numbers::pi_v<float> * op.phase + modulation) * op.envelope *
               op.levelGain * amplitudeMod;
    }

    std::array<Operator, 4> operators{};
    std::array<float, 2> feedbackHistory{};
    int algorithm = 0;
    int feedback = 0;
    int frequencyModulation = 0;
    int amplitudeModulation = 0;
    float lfoRate = 5.0f;
    float tuneSemitones = 0.0f;
    float fineCents = 0.0f;
    float volume = 1.0f;
    float currentNote = 60.0f;
    float noteVelocity = 1.0f;
    float lfoPhase = 0.0f;
};

} // namespace

std::unique_ptr<InstrumentType> createGenesisSynth()
{
    return std::make_unique<InstrumentType>(
      InstrumentType{ .name = "GEN",
                      .createMetadata = &GenesisSynthVoice::createMetadata,
                      .createVoice = [](int /*voiceIdx*/) -> std::unique_ptr<InstrumentVoice>
                      { return std::make_unique<GenesisSynthVoice>(); } });
}

} // namespace polysynth

#endif
