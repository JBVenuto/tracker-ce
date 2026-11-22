#include "ampLogarythmicValues.h"
#include "mtAudioEngine.h"
#include "mtSequencer.h"

using namespace mtAudioEngineConstans;

AUDIO_CORE_RAM void playerEngine::update()
{
    if (isBlockedInstrument[currentInstrument_idx])
        return;
    //**************HANDLE UPDATE END RELEASE
    // obsluga tego zdarzenia jako pierwsza zeby zapobiec zawieszaniu nut
    handleUpdateEndReleaseAction();
    //***********************
    //**************HANDLE UPDATE END PLAY DETECT
    handleUpdateEndPlayDetect();
    //***********************
    //**************HANDLE UPDATE ENVELOPE
    // stablicowanie warunkow odswiezenia envelopeow
    bool envelopeCondition[ACTIVE_ENVELOPES];
    envelopeCondition[envAmp] =
      ((mtProject.instrument[currentInstrument_idx].envelope[envAmp].enable == 1) ||
       (trackControlParameter[(int)controlType::sequencerMode][envelopesControlValue[envAmp]]) ||
       (trackControlParameter[(int)controlType::sequencerMode2][envelopesControlValue[envAmp]]) ||
       (trackControlParameter[(int)controlType::performanceMode][envelopesControlValue[envAmp]]));
    envelopeCondition[envCutoff] = (mtProject.instrument[currentInstrument_idx].filterEnable == 1);
    envelopeCondition[envPan] = true;
    envelopeCondition[envWtPos] =
      (mtProject.instrument[currentInstrument_idx].sample.type == mtSampleTypeWavetable) ||
      (mtProject.instrument[currentInstrument_idx].playMode == playModeWavetable) ||
      ((mtProject.instrument[currentInstrument_idx].sample.type == mtSampleTypeWaveFile) &&
       (mtProject.instrument[currentInstrument_idx].playMode == playModeGranular));
    envelopeCondition[envGranPos] =
      ((mtProject.instrument[currentInstrument_idx].sample.type == mtSampleTypeWaveFile) &&
       (mtProject.instrument[currentInstrument_idx].playMode == playModeGranular));
    envelopeCondition[envFinetune] = true;
    //*******
    for (uint8_t i = envAmp; i < ACTIVE_ENVELOPES; i++)
    {
        handleUpdateEnvelope(i, envelopeCondition[i]);
    }
    //***********************
    //**************HANDLE UPDATE REFRESH PARAMS
    if (refreshRegister)
    {
        // Jako pierwsza obsluga parametrow sterowanych z envelope
        handleUpdateRefreshCutoff();
        handleUpdateRefreshPanning();
        handleUpdateRefreshGranPos();
        handleUpdateRefreshWtPos();
        handleUpdateRefreshFinetune();
        //**********
        handleUpdateRefreshLP1();
        handleUpdateRefreshLP2();
        handleUpdateRefreshTune();
        handleUpdateRefreshVolume();
        handleUpdateRefreshResonance();
        handleUpdateRefreshDelay();
        handleUpdateRefreshReverb();
        handleUpdateRefreshOverdrive();
        handleUpdateRefreshBitDepth();
        handleUpdateRefreshGranLen();
        handleUpdateRefreshGranWave();
        handleUpdateRefreshGranLoop();
        handleUpdateRefreshAmpLFO();
        handleUpdateRefreshCutoffLFO();
        handleUpdateRefreshWtPosLFO();
        handleUpdateRefreshGranPosLFO();
        handleUpdateRefreshPanningLFO();
        handleUpdateRefreshFinetuneLFO();
    }
    //***********************
}
inline void playerEngine::handleUpdateEndPlayDetect()
{
    currentPlayState = playMemPtr[0]->isPlaying();
    if (currentPlayState == 0 && lastPlayState == 1)
    {
        //		playMemPtr->envelopeNoteOff();
        interfacePlayingEndFlag = 1;
        if (isTrackDisplayed)
            onEndDisplay = true;
    }
    lastPlayState = currentPlayState;
}

inline void playerEngine::handleUpdateEnvelope(uint8_t type, bool enableCondition)
{
    if (enableCondition)
    {
        if (type == envAmp)
        {
            uint8_t curAmpLoop = mtProject.instrument[currentInstrument_idx].envelope[envAmp].loop;
            if ((curAmpLoop == 1 || curAmpLoop == 3) ||
                (trackControlParameter[(int)controlType::sequencerMode]
                                      [(int)parameterList::lfoAmp]) ||
                (trackControlParameter[(int)controlType::sequencerMode2]
                                      [(int)parameterList::lfoAmp]) ||
                (trackControlParameter[(int)controlType::performanceMode]
                                      [(int)parameterList::lfoAmp]))
            {
                if (playMemPtr[0]->envelopeGetState() != 0)
                {
                    if (sequencer.getSeqState())
                    {
                        currentSeqTempo = sequencer.getCurrentTempo();

                        if (currentSeqTempo != lastSeqTempo)
                        {
                            uint8_t mostSignificantLFORate =
                              mtProject.instrument[currentInstrument_idx].lfo[envAmp].speed;
                            if (trackControlParameter[(int)controlType::performanceMode]
                                                     [(int)parameterList::lfoAmp])
                            {
                                mostSignificantLFORate = currentPerformanceValues.lfoAmpRate;
                            }
                            else if ((trackControlParameter[(int)controlType::sequencerMode2]
                                                           [(int)parameterList::lfoAmp]) ||
                                     (trackControlParameter[(int)controlType::performanceMode]
                                                           [(int)parameterList::lfoAmp]))
                            {
                                mostSignificantLFORate = currentSeqModValues.lfoAmpRate;
                            }
                            uint8_t targetLoop = (curAmpLoop == 3) ? 3 : 1;
                            calcLfoBasedEnvelope(
                              &lfoBasedEnvelope[envAmp],
                              &mtProject.instrument[currentInstrument_idx].lfo[envAmp],
                              mostSignificantLFORate,
                              1,
                              targetLoop);
                            initEnvelopesParameters(envAmp, &lfoBasedEnvelope[envAmp]);
                        }

                        lastSeqTempo = currentSeqTempo;
                    }
                }
            }
        }
        else
        {
            if ((mtProject.instrument[currentInstrument_idx].isEnvEnabled(type)) ||
                (trackControlParameter[(int)controlType::sequencerMode]
                                      [envelopesControlValue[type]]) ||
                (trackControlParameter[(int)controlType::sequencerMode2]
                                      [envelopesControlValue[type]]) ||
                (trackControlParameter[(int)controlType::performanceMode]
                                      [envelopesControlValue[type]]))
            {
                bool isActiveEnvelope = ((envelopePtr[type]->isKeyPressed() == 1) ||
                                         (envelopePtr[type]->getPhase() != 0));

                uint8_t curLoop = mtProject.instrument[currentInstrument_idx].envelope[type].loop;
                if ((curLoop == 1 || curLoop == 3) ||
                    (trackControlParameter[(int)controlType::sequencerMode]
                                          [envelopesControlValue[type]]) ||
                    (trackControlParameter[(int)controlType::sequencerMode2]
                                          [envelopesControlValue[type]]) ||
                    (trackControlParameter[(int)controlType::performanceMode]
                                          [envelopesControlValue[type]]))
                {
                    if (isActiveEnvelope)
                    {
                        if (sequencer.getSeqState())
                        {
                            envelopePtr[type]->syncTrackerSeq(sequencer.getSeqTimer(),
                                                              sequencer.getCurrentTempo());
                            envelopePtr[type]->setSync(1);
                        }
                        else
                        {
                            envelopePtr[type]->setSync(0);
                        }
                    }
                }
                else
                {
                    envelopePtr[type]->setSync(0);
                }
                if (isActiveEnvelope)
                {
                    currentEnvelopeModification[type] = envelopePtr[type]->getOut();
                    if (type == envGranLen &&
                        mtProject.instrument[currentInstrument_idx].playMode == playModeGranular)
                    {
                        refreshRegister |= GRANULAR_LEN_SEND_MASK;
                    }
                    else
                    {
                        refreshRegister |= envTargetRefreshMask[type];
                    }
                }
            }
        }
    }
}

inline void playerEngine::handleUpdateEndReleaseAction()
{
    if (playMemPtr[0]->envelopeEndRelease())
    {
        for (int channel = 0; channel < (int)playMemPtr.size(); channel++)
        {
            playMemPtr[channel]->envelopeClearEndReleaseFlag();
        }
        interfaceEndReleaseFlag = 1;
        if (isTrackDisplayed)
            onEndDisplay = true;
        //		playMemPtr->stop();
        for (uint8_t i = envPan; i < ACTIVE_ENVELOPES; i++)
        {
            uint8_t curLoop = mtProject.instrument[currentInstrument_idx].envelope[i].loop;
            if ((mtProject.instrument[currentInstrument_idx].isEnvEnabled(i) &&
                 (curLoop == 1 || curLoop == 3)) ||
                (trackControlParameter[(int)controlType::sequencerMode]
                                      [envelopesControlValue[i]]) ||
                (trackControlParameter[(int)controlType::sequencerMode2]
                                      [envelopesControlValue[i]]) ||
                (trackControlParameter[(int)controlType::performanceMode]
                                      [envelopesControlValue[i]]))
            {
                currentEnvelopeModification[i] = 0;
                envelopePtr[i]->stop();
            }
        }
    }
}

inline void playerEngine::handleUpdateRefreshLP1()
{
    if (refreshRegister & LP1_MASK)
    {
        refreshRegister &= (~LP1_MASK);
        modLP1(mtProject.instrument[currentInstrument_idx].loopPoint1);
    }
}
inline void playerEngine::handleUpdateRefreshLP2()
{
    if (refreshRegister & LP2_MASK)
    {
        refreshRegister &= (~LP2_MASK);
        modLP2(mtProject.instrument[currentInstrument_idx].loopPoint2);
    }
}
inline void playerEngine::handleUpdateRefreshFinetune()
{
    if (refreshRegister & FINETUNE_MASK)
    {
        refreshRegister &= (~FINETUNE_MASK);

        if (!isActiveEnvelope(envFinetune))
        {
            currentEnvelopeModification[envFinetune] = 0;
        }

        int32_t localFinetuneMod =
          currentEnvelopeModification[envFinetune] * MAX_INSTRUMENT_FINETUNE;

        int16_t localFinetune = 0;

        if (trackControlParameter[(int)controlType::sequencerMode][(int)parameterList::fineTune] ||
            trackControlParameter[(int)controlType::sequencerMode2][(int)parameterList::fineTune])
        {
            localFinetune = currentSeqModValues.fineTune;
        }
        else
        {
            localFinetune = mtProject.instrument[currentInstrument_idx].fineTune;
        }

        if ((int)(localFinetune + localFinetuneMod) < MIN_INSTRUMENT_FINETUNE)
            localFinetune = MIN_INSTRUMENT_FINETUNE;
        else if ((int)(localFinetune + localFinetuneMod) >= MAX_INSTRUMENT_FINETUNE)
            localFinetune = MAX_INSTRUMENT_FINETUNE;
        else
            localFinetune += localFinetuneMod;

        for (int channel = 0; channel < (int)playMemPtr.size(); channel++)
        {
            playMemPtr[channel]->setForcedFineTune(localFinetune);
        }

        modFineTune(localFinetune);
    }
}
inline void playerEngine::handleUpdateRefreshTune()
{
    if (refreshRegister & TUNE_MASK)
    {
        refreshRegister &= (~TUNE_MASK);

        int8_t localTune;
        if (trackControlParameter[(int)controlType::performanceMode]
                                 [(int)parameterList::tune])
        {
            localTune = currentPerformanceValues.tune;
        }
        else if (trackControlParameter[(int)controlType::sequencerMode]
                                      [(int)parameterList::tune] ||
                 trackControlParameter[(int)controlType::sequencerMode2]
                                      [(int)parameterList::tune])
        {
            localTune = currentSeqModValues.tune;
        }
        else
        {
            localTune = mtProject.instrument[currentInstrument_idx].tune;
        }

        modTune(localTune);

        for (int channel = 0; channel < (int)playMemPtr.size(); channel++)
        {
            playMemPtr[channel]->setForcedTune(localTune);
        }
    }
}
inline void playerEngine::handleUpdateRefreshVolume()
{
    if (refreshRegister & VOLUME_MASK)
    {
        refreshRegister &= (~VOLUME_MASK);

        uint8_t volume = getMostSignificantVolume();
        float localAmount = getMostSignificantAmount();

        modVolume(ampLogValues[volume] * localAmount);
    }
}
inline void playerEngine::handleUpdateRefreshPanning()
{
    if (refreshRegister & PANNING_MASK)
    {
        refreshRegister &= (~PANNING_MASK);

        if (!isActiveEnvelope(envPan))
        {
            currentEnvelopeModification[envPan] = 0;
        }

        int32_t localPanningMod = currentEnvelopeModification[envPan] * PANNING_MAX;

        int16_t localPanning = 0;

        if (trackControlParameter[(int)controlType::sequencerMode][(int)parameterList::panning] ||
            trackControlParameter[(int)controlType::sequencerMode2][(int)parameterList::panning])
        {
            localPanning = currentSeqModValues.panning;
        }
        else
        {
            localPanning = mtProject.instrument[currentInstrument_idx].panning;
        }

        if (trackControlParameter[(int)controlType::performanceMode][(int)parameterList::panning])
        {
            localPanning = (int16_t)clamp<int>(
              (int)localPanning + (int)performanceMod.panning, (int)PANNING_MIN, (int)PANNING_MAX);
        }

        localPanning = (int16_t)clamp<int>(
          (int)localPanning + (int)localPanningMod, (int)PANNING_MIN, (int)PANNING_MAX);

        modPanning(localPanning);
    }
}
inline void playerEngine::handleUpdateRefreshCutoff()
{
    if (refreshRegister & CUTOFF_MASK)
    {
        refreshRegister &= (~CUTOFF_MASK);

        if (!isActiveEnvelope(envCutoff))
        {
            currentEnvelopeModification[envCutoff] = 0;
        }

        float localCutoff = 0.0f;

        if (trackControlParameter[(int)controlType::performanceMode]
                                 [(int)parameterList::filterCutoff])
        {
            localCutoff = currentPerformanceValues.filterCutoff;
        }
        else if (trackControlParameter[(int)controlType::sequencerMode]
                                      [(int)parameterList::filterCutoff] ||
                 trackControlParameter[(int)controlType::sequencerMode2]
                                      [(int)parameterList::filterCutoff])
        {
            localCutoff = currentSeqModValues.filterCutoff;
        }
        else
        {
            localCutoff = mtProject.instrument[currentInstrument_idx].cutOff;
        }

        if (localCutoff + currentEnvelopeModification[envCutoff] < 0.0f)
            localCutoff = 0.0f;
        else if (localCutoff + currentEnvelopeModification[envCutoff] > 1.0f)
            localCutoff = 1.0f;
        else
            localCutoff += currentEnvelopeModification[envCutoff];

        modCutoff(localCutoff);
    }
}
inline void playerEngine::handleUpdateRefreshResonance()
{
    if (refreshRegister & RESONANCE_MASK)
    {
        refreshRegister &= (~RESONANCE_MASK);
        modResonance(mtProject.instrument[currentInstrument_idx].resonance);
    }
}
void playerEngine::handleUpdateRefreshDelay()
{
    if (refreshRegister & DELAY_SEND_MASK)
    {
        refreshRegister &= (~DELAY_SEND_MASK);

        uint8_t localDelaySend = 0;

        if (trackControlParameter[(int)controlType::sequencerMode][(int)parameterList::delaySend] ||
            trackControlParameter[(int)controlType::sequencerMode2][(int)parameterList::delaySend])
        {
            localDelaySend = currentSeqModValues.delaySend;
        }
        else
        {
            localDelaySend = mtProject.instrument[currentInstrument_idx].delaySend;
        }

        if (trackControlParameter[(int)controlType::performanceMode][(int)parameterList::delaySend])
        {
            localDelaySend = clamp<int>(
              (int)localDelaySend + (int)performanceMod.delaySend, (int)SEND_MIN, (int)SEND_MAX);
        }

        if (((muteState == MUTE_DISABLE) && (onlyDelayMuteState == MUTE_DISABLE)) ||
            ((engine.forceDelaySend == 1) && !isSampleTrackMutedOrHidden(nChannel)))
        {
            modDelaySend(localDelaySend);
        }
        else
        {
            modDelaySend(AMP_MUTED);
        }
    }
}

void playerEngine::handleUpdateRefreshReverb()
{
    if (refreshRegister & REVERB_SEND_MASK)
    {
        refreshRegister &= (~REVERB_SEND_MASK);

        uint8_t localReverbSend = 0;

        if (trackControlParameter[(int)controlType::sequencerMode]
                                 [(int)parameterList::reverbSend] ||
            trackControlParameter[(int)controlType::sequencerMode2][(int)parameterList::reverbSend])
        {
            localReverbSend = currentSeqModValues.reverbSend;
        }
        else
        {
            localReverbSend = mtProject.instrument[currentInstrument_idx].reverbSend;
        }

        if (trackControlParameter[(int)controlType::performanceMode]
                                 [(int)parameterList::reverbSend])
        {
            localReverbSend = clamp<int>(
              (int)localReverbSend + (int)performanceMod.reverbSend, (int)SEND_MIN, (int)SEND_MAX);
        }

        if (((muteState == 0) && (onlyReverbMuteState == 0)) ||
            ((engine.forceReverbSend == 1) && !isSampleTrackMutedOrHidden(nChannel)))
        {
            modReverbSend(localReverbSend);
        }
        else
        {
            modReverbSend(AMP_MUTED);
        }
    }
}

inline void playerEngine::handleUpdateRefreshOverdrive()
{
    if (refreshRegister & OVERDRIVE_MASK)
    {
        uint8_t overdrive;
        refreshRegister &= (~OVERDRIVE_MASK);

        if (trackControlParameter[(int)controlType::performanceMode][(int)parameterList::overdrive])
        {
            overdrive = currentPerformanceValues.overdrive;
        }
        else if (trackControlParameter[(int)controlType::sequencerMode]
                                      [(int)parameterList::overdrive] ||
                 trackControlParameter[(int)controlType::sequencerMode2]
                                      [(int)parameterList::overdrive])
        {
            overdrive = currentSeqModValues.overdrive;
        }
        else
        {
            overdrive = mtProject.instrument[currentInstrument_idx].overdrive;
        }

        modOverdrive(overdrive);
    }
}

inline void playerEngine::handleUpdateRefreshBitDepth()
{
    if (refreshRegister & BIT_DEPTH_MASK)
    {
        uint8_t bitDepth;
        refreshRegister &= (~BIT_DEPTH_MASK);

        if (trackControlParameter[(int)controlType::performanceMode][(int)parameterList::bitDepth])
        {
            bitDepth = currentPerformanceValues.bitDepth;
        }
        else if (trackControlParameter[(int)controlType::sequencerMode]
                                      [(int)parameterList::bitDepth] ||
                 trackControlParameter[(int)controlType::sequencerMode2]
                                      [(int)parameterList::bitDepth])
        {
            bitDepth = currentSeqModValues.bitDepth;
        }
        else
        {
            bitDepth = mtProject.instrument[currentInstrument_idx].bitDepth;
        }

        modBitDepth(bitDepth);
    }
}

inline void playerEngine::handleUpdateRefreshWtPos()
{
    if (refreshRegister & WT_POS_SEND_MASK)
    {
        refreshRegister &= (~WT_POS_SEND_MASK);
        if ((!mtProject.instrument[currentInstrument_idx].isWtPosEnvEnabled()) &&
            (!trackControlParameter[(int)controlType::sequencerMode]
                                   [(int)parameterList::lfoWavetablePosition]) &&
            (!trackControlParameter[(int)controlType::sequencerMode2]
                                   [(int)parameterList::lfoWavetablePosition]) &&
            (!trackControlParameter[(int)controlType::performanceMode]
                                   [(int)parameterList::lfoWavetablePosition]))
        {
            currentEnvelopeModification[envWtPos] = 0;
        }

        int32_t localWtPosisionMod =
          currentEnvelopeModification[envWtPos] *
          mtProject.instrument[currentInstrument_idx].sample.wavetableWindowNumber;
        uint32_t localWtPos = 0;

        if (trackControlParameter[(int)controlType::performanceMode]
                                 [(int)parameterList::wavetablePosition])
        {
            localWtPos = currentPerformanceValues.wavetablePosition;
        }
        else if (trackControlParameter[(int)controlType::sequencerMode]
                                      [(int)parameterList::wavetablePosition] ||
                 trackControlParameter[(int)controlType::sequencerMode2]
                                      [(int)parameterList::wavetablePosition])
        {
            localWtPos = currentSeqModValues.wavetablePosition;
        }
        else
        {
            localWtPos = mtProject.instrument[currentInstrument_idx].wavetableCurrentWindow;
        }

        if ((int)(localWtPos + localWtPosisionMod) < 0)
        {
            localWtPos = 0;
        }
        else if ((int)(localWtPos + localWtPosisionMod) >
                 (int)(mtProject.instrument[currentInstrument_idx].sample.wavetableWindowNumber -
                       1))
        {
            localWtPos =
              mtProject.instrument[currentInstrument_idx].sample.wavetableWindowNumber - 1;
        }
        else
            localWtPos += localWtPosisionMod;

        for (int channel = 0; channel < (int)playMemPtr.size(); channel++)
        {
            playMemPtr[channel]->setWavetableWindow(localWtPos);
            playMemPtr[channel]->setForcedWavetableWindow(localWtPos);
        }
    }
}
inline void playerEngine::handleUpdateRefreshGranPos()
{
    if (refreshRegister & GRANULAR_POS_SEND_MASK)
    {
        refreshRegister &= (~GRANULAR_POS_SEND_MASK);
        if ((!mtProject.instrument[currentInstrument_idx].envelope[envGranPos].enable) &&
            (!trackControlParameter[(int)controlType::sequencerMode]
                                   [(int)parameterList::lfoGranularPosition]) &&
            (!trackControlParameter[(int)controlType::sequencerMode2]
                                   [(int)parameterList::lfoGranularPosition]) &&
            (!trackControlParameter[(int)controlType::sequencerMode2]
                                   [(int)parameterList::lfoGranularPosition]))
        {
            currentEnvelopeModification[envGranPos] = 0;
        }

        int32_t localGranPosisionMod = currentEnvelopeModification[envGranPos] * MAX_16BIT;

        uint16_t localGranPosition = 0;
        if (trackControlParameter[(int)controlType::performanceMode]
                                 [(int)parameterList::granularPosition])
        {
            localGranPosition = currentPerformanceValues.granularPosition;
        }
        else if (trackControlParameter[(int)controlType::sequencerMode]
                                      [(int)parameterList::granularPosition] ||
                 trackControlParameter[(int)controlType::sequencerMode2]
                                      [(int)parameterList::granularPosition])
        {
            localGranPosition = currentSeqModValues.granularPosition;
        }
        else
        {
            localGranPosition =
              mtProject.instrument[currentInstrument_idx].granular.currentPosition;
        }

        if ((int)(localGranPosition + localGranPosisionMod) < 0)
            localGranPosition = 0;
        else if ((int)(localGranPosition + localGranPosisionMod) >= MAX_16BIT)
            localGranPosition = MAX_16BIT;
        else
            localGranPosition += localGranPosisionMod;

        modGranularPosition(localGranPosition);
    }
}
inline void playerEngine::handleUpdateRefreshGranLen()
{
    if (refreshRegister & GRANULAR_LEN_SEND_MASK)
    {
        refreshRegister &= (~GRANULAR_LEN_SEND_MASK);
        if (!mtProject.instrument[currentInstrument_idx].isGranLenEnvEnabled())
        {
            currentEnvelopeModification[envGranLen] = 0;
        }

        uint32_t sampleLen = mtProject.instrument[currentInstrument_idx].sample.length;
        int32_t maxGrainLen = (sampleLen > 0 && sampleLen < MAX_GRANULAR_LENGTH)
                                ? static_cast<int32_t>(sampleLen)
                                : static_cast<int32_t>(MAX_GRANULAR_LENGTH);
        if (maxGrainLen < MIN_GRANULAR_LENGTH)
            maxGrainLen = MIN_GRANULAR_LENGTH;

        int32_t localGranLenMod =
          static_cast<int32_t>(currentEnvelopeModification[envGranLen] *
                               (maxGrainLen - MIN_GRANULAR_LENGTH));

        int32_t localGranLength = 0;
        if (trackControlParameter[(int)controlType::sequencerMode]
                                 [(int)parameterList::granularLength] ||
            trackControlParameter[(int)controlType::sequencerMode2]
                                 [(int)parameterList::granularLength])
        {
            localGranLength = currentSeqModValues.granularLength;
        }
        else
        {
            localGranLength =
              mtProject.instrument[currentInstrument_idx].granular.grainLength;
        }

        localGranLength = clamp<int32_t>(
          localGranLength + localGranLenMod, MIN_GRANULAR_LENGTH, maxGrainLen);

        modGranularGrainLength(static_cast<uint16_t>(localGranLength));
    }
}
inline void playerEngine::handleUpdateRefreshGranWave()
{
    if (refreshRegister & GRANULAR_WAVE_SEND_MASK)
    {
        refreshRegister &= (~GRANULAR_WAVE_SEND_MASK);
        for (int channel = 0; channel < (int)playMemPtr.size(); channel++)
        {
            playMemPtr[channel]->setGranularWave(
              mtProject.instrument[currentInstrument_idx].granular.shape);
        }
    }
}
inline void playerEngine::handleUpdateRefreshGranLoop()
{
    if (refreshRegister & GRANULAR_LOOP_SEND_MASK)
    {
        refreshRegister &= (~GRANULAR_LOOP_SEND_MASK);
        for (int channel = 0; channel < (int)playMemPtr.size(); channel++)
        {
            playMemPtr[channel]->setGranularLoopMode(
              mtProject.instrument[currentInstrument_idx].granular.type);
        }
    }
}
inline void playerEngine::handleUpdateRefreshAmpLFO()
{
    if (refreshRegister & LFO_AMP_SEND_MASK)
    {
        refreshRegister &= (~LFO_AMP_SEND_MASK);

        if ((mtProject.instrument[currentInstrument_idx].envelope[envAmp].enable) &&
            (!trackControlParameter[(int)controlType::sequencerMode][(int)parameterList::lfoAmp]) &&
            (!trackControlParameter[(int)controlType::sequencerMode2]
                                   [(int)parameterList::lfoAmp]) &&
            (!trackControlParameter[(int)controlType::performanceMode][(int)parameterList::lfoAmp]))
        {
            uint8_t curAmpLoop = mtProject.instrument[currentInstrument_idx].envelope[envAmp].loop;
            if (curAmpLoop == 1 || curAmpLoop == 3)
            {
                calcLfoBasedEnvelope(&lfoBasedEnvelope[envAmp],
                                     &mtProject.instrument[currentInstrument_idx].lfo[envAmp],
                                     mtProject.instrument[currentInstrument_idx].lfo[envAmp].speed,
                                     1,
                                     curAmpLoop);
                initEnvelopesParameters(envAmp, &lfoBasedEnvelope[envAmp]);
                if (currentInstrument_idx == mtProject.values.lastUsedInstrument)
                {
                    setSyncParamsLFO(envAmp);
                }
                bool isRandom = (curAmpLoop == 1) &&
                  (mtProject.instrument[currentInstrument_idx].lfo[envAmp].shape == lfoShapeRandom);

                for (int channel = 0; channel < (int)playMemPtr.size(); channel++)
                {
                    playMemPtr[channel]->envelopeSetIsRandom(isRandom);
                }
            }
            else
            {
                initEnvelopesParameters(envAmp, &mtProject.instrument[currentInstrument_idx].envelope[envAmp]);
                for (int channel = 0; channel < (int)playMemPtr.size(); channel++)
                {
                    playMemPtr[channel]->envelopeSetIsRandom(false);
                }
            }
        }
    }
}
inline void playerEngine::handleUpdateRefreshCutoffLFO()
{
    if (refreshRegister & LFO_FILTER_SEND_MASK)
    {
        refreshRegister &= (~LFO_FILTER_SEND_MASK);

        if (mtProject.instrument[currentInstrument_idx].filterEnable)
        {
            if ((mtProject.instrument[currentInstrument_idx].envelope[envCutoff].enable) &&
                (!trackControlParameter[(int)controlType::sequencerMode]
                                       [(int)parameterList::lfoCutoff]) &&
                (!trackControlParameter[(int)controlType::sequencerMode2]
                                       [(int)parameterList::lfoCutoff]) &&
                (!trackControlParameter[(int)controlType::performanceMode]
                                       [(int)parameterList::lfoCutoff]))
            {
                uint8_t curCutoffLoop = mtProject.instrument[currentInstrument_idx].envelope[envCutoff].loop;
                if (curCutoffLoop == 1 || curCutoffLoop == 3)
                {
                    calcLfoBasedEnvelope(
                      &lfoBasedEnvelope[envCutoff],
                      &mtProject.instrument[currentInstrument_idx].lfo[envCutoff],
                      mtProject.instrument[currentInstrument_idx].lfo[envCutoff].speed,
                      0,
                      curCutoffLoop);
                    initEnvelopesParameters(envCutoff, &lfoBasedEnvelope[envCutoff]);
                    if (currentInstrument_idx == mtProject.values.lastUsedInstrument)
                    {
                        setSyncParamsLFO(envCutoff);
                    }

                    bool isRandom = (curCutoffLoop == 1) &&
                      (mtProject.instrument[currentInstrument_idx].lfo[envCutoff].shape ==
                       lfoShapeRandom);
                    envelopePtr[envCutoff]->setIsRandom(isRandom);
                }
                else
                {
                    initEnvelopesParameters(
                      envCutoff, &mtProject.instrument[currentInstrument_idx].envelope[envCutoff]);
                    envelopePtr[envCutoff]->setIsRandom(false);
                }
            }
        }
    }
}
inline void playerEngine::handleUpdateRefreshWtPosLFO()
{
    if (refreshRegister & LFO_WT_POS_SEND_MASK)
    {
        refreshRegister &= (~LFO_WT_POS_SEND_MASK);

        const bool isGranular =
          (mtProject.instrument[currentInstrument_idx].sample.type == mtSampleTypeWaveFile) &&
          (mtProject.instrument[currentInstrument_idx].playMode == playModeGranular);
        const bool isWavetable =
          (mtProject.instrument[currentInstrument_idx].sample.type == mtSampleTypeWavetable) ||
          (mtProject.instrument[currentInstrument_idx].playMode == playModeWavetable);

        if (isWavetable || isGranular)
        {
            if ((mtProject.instrument[currentInstrument_idx].isEnvEnabled(envWtPos)) &&
                (!trackControlParameter[(int)controlType::sequencerMode]
                                       [(int)parameterList::lfoWavetablePosition]) &&
                (!trackControlParameter[(int)controlType::sequencerMode2]
                                       [(int)parameterList::lfoWavetablePosition]) &&
                (!trackControlParameter[(int)controlType::performanceMode]
                                       [(int)parameterList::lfoWavetablePosition]))
            {
                uint8_t curWtLoop = mtProject.instrument[currentInstrument_idx].envelope[envWtPos].loop;
                if (curWtLoop == 1 || curWtLoop == 3)
                {
                    calcLfoBasedEnvelope(
                      &lfoBasedEnvelope[envWtPos],
                      &mtProject.instrument[currentInstrument_idx].lfo[envWtPos],
                      mtProject.instrument[currentInstrument_idx].lfo[envWtPos].speed,
                      0,
                      curWtLoop);
                    initEnvelopesParameters(envWtPos, &lfoBasedEnvelope[envWtPos]);
                    if (currentInstrument_idx == mtProject.values.lastUsedInstrument)
                    {
                        setSyncParamsLFO(envWtPos);
                    }

                    bool isRandom = (curWtLoop == 1) &&
                      (mtProject.instrument[currentInstrument_idx].lfo[envWtPos].shape ==
                       lfoShapeRandom);
                    envelopePtr[envWtPos]->setIsRandom(isRandom);
                }
                else
                {
                    initEnvelopesParameters(
                      envWtPos, &mtProject.instrument[currentInstrument_idx].envelope[envWtPos]);
                    envelopePtr[envWtPos]->setIsRandom(false);
                }
            }
            else if (isGranular)
            {
                currentEnvelopeModification[envGranLen] = 0;
                refreshRegister |= GRANULAR_LEN_SEND_MASK;
            }
            else if (isWavetable)
            {
                currentEnvelopeModification[envWtPos] = 0;
                refreshRegister |= WT_POS_SEND_MASK;
            }
        }
    }
}
inline void playerEngine::handleUpdateRefreshGranPosLFO()
{
    if (refreshRegister & LFO_GRAN_POS_SEND_MASK)
    {
        refreshRegister &= (~LFO_GRAN_POS_SEND_MASK);
        if ((mtProject.instrument[currentInstrument_idx].sample.type == mtSampleTypeWaveFile) &&
            (mtProject.instrument[currentInstrument_idx].playMode == playModeGranular))
        {
            if ((mtProject.instrument[currentInstrument_idx].envelope[envGranPos].enable) &&
                (!trackControlParameter[(int)controlType::sequencerMode]
                                       [(int)parameterList::lfoGranularPosition]) &&
                (!trackControlParameter[(int)controlType::sequencerMode2]
                                       [(int)parameterList::lfoGranularPosition]) &&
                (!trackControlParameter[(int)controlType::performanceMode]
                                       [(int)parameterList::lfoGranularPosition]))
            {
                uint8_t curGranLoop = mtProject.instrument[currentInstrument_idx].envelope[envGranPos].loop;
                if (curGranLoop == 1 || curGranLoop == 3)
                {
                    calcLfoBasedEnvelope(
                      &lfoBasedEnvelope[envGranPos],
                      &mtProject.instrument[currentInstrument_idx].lfo[envGranPos],
                      mtProject.instrument[currentInstrument_idx].lfo[envGranPos].speed,
                      0,
                      curGranLoop);
                    initEnvelopesParameters(envGranPos, &lfoBasedEnvelope[envGranPos]);
                    if (currentInstrument_idx == mtProject.values.lastUsedInstrument)
                    {
                        setSyncParamsLFO(envGranPos);
                    }

                    bool isRandom = (curGranLoop == 1) &&
                      (mtProject.instrument[currentInstrument_idx].lfo[envGranPos].shape ==
                       lfoShapeRandom);
                    envelopePtr[envGranPos]->setIsRandom(isRandom);
                }
                else
                {
                    initEnvelopesParameters(
                      envGranPos,
                      &mtProject.instrument[currentInstrument_idx].envelope[envGranPos]);
                    envelopePtr[envGranPos]->setIsRandom(false);
                }
            }
        }
    }
}
inline void playerEngine::handleUpdateRefreshPanningLFO()
{
    if (refreshRegister & LFO_PANNING_SEND_MASK)
    {
        refreshRegister &= (~LFO_PANNING_SEND_MASK);

        if ((mtProject.instrument[currentInstrument_idx].envelope[envPan].enable) &&
            (!trackControlParameter[(int)controlType::sequencerMode]
                                   [(int)parameterList::lfoPanning]) &&
            (!trackControlParameter[(int)controlType::sequencerMode2]
                                   [(int)parameterList::lfoPanning]) &&
            (!trackControlParameter[(int)controlType::performanceMode]
                                   [(int)parameterList::lfoPanning]))
        {
            uint8_t curPanLoop = mtProject.instrument[currentInstrument_idx].envelope[envPan].loop;
            if (curPanLoop == 1 || curPanLoop == 3)
            {
                calcLfoBasedEnvelope(&lfoBasedEnvelope[envPan],
                                     &mtProject.instrument[currentInstrument_idx].lfo[envPan],
                                     mtProject.instrument[currentInstrument_idx].lfo[envPan].speed,
                                     0,
                                     curPanLoop);
                initEnvelopesParameters(envPan, &lfoBasedEnvelope[envPan]);
                if (currentInstrument_idx == mtProject.values.lastUsedInstrument)
                {
                    setSyncParamsLFO(envPan);
                }

                bool isRandom = (curPanLoop == 1) &&
                  (mtProject.instrument[currentInstrument_idx].lfo[envPan].shape == lfoShapeRandom);
                envelopePtr[envPan]->setIsRandom(isRandom);
            }
            else
            {
                initEnvelopesParameters(
                  envPan, &mtProject.instrument[currentInstrument_idx].envelope[envPan]);
                envelopePtr[envPan]->setIsRandom(false);
            }
        }
    }
}

inline void playerEngine::handleUpdateRefreshFinetuneLFO()
{
    if (refreshRegister & LFO_FINETUNE_SEND_MASK)
    {
        refreshRegister &= (~LFO_FINETUNE_SEND_MASK);

        if ((mtProject.instrument[currentInstrument_idx].envelope[envFinetune].enable) &&
            (!trackControlParameter[(int)controlType::sequencerMode]
                                   [(int)parameterList::lfoFinetune]) &&
            (!trackControlParameter[(int)controlType::sequencerMode2]
                                   [(int)parameterList::lfoFinetune]) &&
            (!trackControlParameter[(int)controlType::performanceMode]
                                   [(int)parameterList::lfoFinetune]))
        {
            uint8_t curFineLoop = mtProject.instrument[currentInstrument_idx].envelope[envFinetune].loop;
            if (curFineLoop == 1 || curFineLoop == 3)
            {
                calcLfoBasedEnvelope(
                  &lfoBasedEnvelope[envFinetune],
                  &mtProject.instrument[currentInstrument_idx].lfo[envFinetune],
                  mtProject.instrument[currentInstrument_idx].lfo[envFinetune].speed,
                  0,
                  curFineLoop);
                initEnvelopesParameters(envFinetune, &lfoBasedEnvelope[envFinetune]);
                if (currentInstrument_idx == mtProject.values.lastUsedInstrument)
                {
                    setSyncParamsLFO(envFinetune);
                }
                bool isRandom = (curFineLoop == 1) &&
                  (mtProject.instrument[currentInstrument_idx].lfo[envFinetune].shape ==
                   lfoShapeRandom);
                envelopePtr[envFinetune]->setIsRandom(isRandom);
            }
            else
            {
                initEnvelopesParameters(
                  envFinetune, &mtProject.instrument[currentInstrument_idx].envelope[envFinetune]);
                envelopePtr[envFinetune]->setIsRandom(false);
            }
        }
    }
}
