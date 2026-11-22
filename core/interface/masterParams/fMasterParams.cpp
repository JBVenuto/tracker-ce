#include "mtAudioEngine.h"
#include "mtConfig.h"
#include "mtPadBoard.h"
#include "mtPadsBacklight.h"
#include "mtSequencer.h"

#include "patternEditor/patternEditor.h"
#include "performanceMode/performanceMode.h"

#include "masterParams/masterParams.h"
#include "mtGainLevelLogarithmicTab.h"

#include "Platform.h"
#include "ampLogarythmicValues.h"
#include "core/interface.h"
#include "core/interfaceMenuPopup.h"
#include "fileManager.h"
#include "math.h"
#include "mtMasterEnhancer.h"
#include "util_math.h"
#ifdef FEATURE_SYNTH
#include "Synth.h"
#endif

cMasterParams masterParams;
static cMasterParams* MP = &masterParams;

extern strMtProject mtProject;

static void functPlayAction();
static void functRecAction();

static void functLeft();
static void functRight();
static void functUp();
static void functDown();
static void functDelete();

// master
static void functSelectVolume(uint8_t state);
static void functSwitchToLimiterScreen();
static void functSwitchToEqualizerScreen();
static void functSelectSaturation();
static void functSelectBassBoost();
static void functSelectStereoBoost();

// delay
static void functSwitchToDelayWindow();
static void functSelectDelaySyncEnable();
static void functSelectDelayPingpongEnable();
static void functSelectDelaySyncRate();
static void functSelectDelayTime();
static void functSelectDelayFeedback();
static void functSelectDelayFilter();
static void functDelayLineIn();
static void functDelayCancel();
// reverb
static void functSwitchToReverbWindow();
static void functSelectReverbSize();
static void functSelectReverbDamp();
static void functSelectReverbPredelay();
static void functSelectReverbDiffusion();
static void functReverbLineIn();
static void functReverbCancel();
// limiter
static void functSelectLimiterAttack();
static void functSelectLimiterRelease();
static void functSelectLimiterThreshold();
static void functSelectLimiterSidechain();
static void functLimiterCancel();
// equalizer
static void functSelectEqualizerBand(uint8_t n, uint8_t state);
static void functEqualizerCancel();

static void functEncoder(int16_t value);
static void functSwitchModule(uint8_t button);
static void functSwitchModeMaster(uint8_t state);

// mixer
static void functSoloMute(uint8_t state);
static void functSoloMuteTrack(uint8_t n, uint8_t state);
// mixer delay reverb
static void functSoloMuteDelay(uint8_t state);
static void functSoloMuteReverb(uint8_t state);
static void functSoloMuteDryMix(uint8_t state);
static void functSoloMuteLineIn(uint8_t state);
static void functSoloMuteDelayReverb(uint8_t state);
#ifdef FEATURE_SYNTH
static void functSelectSynth1(uint8_t state);
static void functSelectSynth2(uint8_t state);
static void functSelectSynth3(uint8_t state);
#endif

// MASTER EDIT FUNCTIONS
void changeVolume(int16_t value);
void changeSaturationType(int16_t value);
void changeBassBoost(int16_t value);
void changeStereoBoost(int16_t value);

void setDefaultMasterVolume();
void setDefaultSaturationType();
void setDefaultBassBoost();
void setDefaultStereoBoost();

// limiter screen
void changeLimiterAttack(int16_t value);
void changeLimiterRelease(int16_t value);
void changeLimiterTreshold(int16_t value);
void changeLimiterSidechain(int16_t value);

void setDefaultLimiterAttack();
void setDefaultLimiterRelease();
void setDefaultLimiterTreshold();
void setDefaultLimiterSidechain();

static void functMiniMiddle(uint8_t state);

static void functPads(uint8_t pad, uint8_t state, int16_t velo);

static void functMenu(uint8_t state);

// zmiana nazwy trackow
static void functEditModeToggle();

static void functCancelEditTrackName();
static void functSaveTrackName();

static void functEnterKeyExport();
static void functLeftKeyboard();
static void functRightKeyboard();
static void functUpKeyboard();
static void functDownKeyboard();
static void functDeleteKeyboard();
static void functInsertKeyboard();

void changeMixerOnMidiCC(uint8_t cc_idx, uint8_t cc_value);

//---------------------------------------

static elapsedMillis refreshTimer;

void cMasterParams::onMaxTracksChanged(uint8_t maxTracks)
{
    (void)maxTracks;
    const uint8_t activeSampleTracks = getProjectActiveSampleTracks();

    if (soloTrack >= static_cast<int8_t>(activeSampleTracks) && soloTrack < kSampleTracks)
    {
        soloTrack = -1;
    }

    for (uint8_t i = activeSampleTracks; i < kSampleTracks; i++)
    {
        trackIsEdited[i] = false;
        ignoreMuteRelease[i] = false;
    }

    rebuildLimiterSidechainList();
    engine.setLimiterSidechain(getEffectiveLimiterSidechain());
}

void cMasterParams::update()
{

    if (displayType == displayMode::mixer && !keyboardManager.getState())
    {
        const uint8_t activeSampleTracks = getProjectActiveSampleTracks();
        for (uint8_t i = 0; i < activeSampleTracks; i++)
        {
            calcTrackLevel(i);
        }
    }
    else if (displayType == displayMode::mixerDelayReverb && !keyboardManager.getState())
    {
        calcDelayLevel();
        calcReverbLevel();
        calcLineInLevel();
        calcDryMixLevel();
#ifdef FEATURE_SYNTH
        calcSynthLevels();
#endif
    }

    if ((displayType == displayMode::masterValues) && (isLimiterScreen))
    {
        gainReduceHandler();
    }

    // gdyby byla taka potrzeba mozna sprobowac tu odswiezac rowniez levele, a obliczac w kazdej
    // petli
    if (refreshTimer < 50)
        return;
    refreshTimer = 0;

    if (displayType == displayMode::mixer && !keyboardManager.getState())
    {
        const uint8_t activeSampleTracks = getProjectActiveSampleTracks();
        for (uint8_t i = 0; i < activeSampleTracks; i++)
        {
            if (trackLevel[i].value != trackLevel[i].lastValue)
            {
                showLevelBar(i, &trackLevel[i]);
            }
            trackLevel[i].lastValue = trackLevel[i].value;
        }
        display.synchronizeRefresh();
    }
    else if (displayType == displayMode::mixerDelayReverb && !keyboardManager.getState() &&
             !mtMenuPopup.isPopupVisible())
    {
        for (uint8_t i = 0; i < 2; i++)
        {
            if (delayLevel[i].value != delayLevel[i].lastValue)
            {
                showLevelBar(delayVolumeScreenPosition + i * 4, &delayLevel[i]);
            }
            delayLevel[i].lastValue = delayLevel[i].value;
        }

        for (uint8_t i = 0; i < 2; i++)
        {
            if (reverbLevel[i].value != reverbLevel[i].lastValue)
            {
                showLevelBar(reverbVolumeScreenPosition + i * 4, &reverbLevel[i]);
            }
            reverbLevel[i].lastValue = reverbLevel[i].value;
        }

        for (uint8_t i = 0; i < 2; i++)
        {
            if (lineInLevel[i].value != lineInLevel[i].lastValue)
            {
                showLevelBar(lineInVolumeScreenPosition + i * 4, &lineInLevel[i]);
            }
            lineInLevel[i].lastValue = lineInLevel[i].value;
        }

        for (uint8_t i = 0; i < 2; i++)
        {
            if (dryMixLevel[i].value != dryMixLevel[i].lastValue)
            {
                showLevelBar(dryMixVolumeScreenPosition + i * 4, &dryMixLevel[i]);
            }
            dryMixLevel[i].lastValue = dryMixLevel[i].value;
        }

#ifdef FEATURE_SYNTH
        static constexpr int kSynthOffset = 8;
        for (int synthIdx = 0; synthIdx < kSynthParts; synthIdx++)
        {

            if (synthLevel[synthIdx].value != synthLevel[synthIdx].lastValue)
            {
                showLevelBar(kSynthOffset + synthIdx, &synthLevel[synthIdx]);
                synthLevel[synthIdx].lastValue = synthLevel[synthIdx].value;
            }
        }
#endif

        display.synchronizeRefresh();
    }

    if (displayType == displayMode::mixer && !keyboardManager.getState())
    {
        refreshAllTracksMixerVolume();
    }
    else if (displayType == displayMode::mixerDelayReverb && !keyboardManager.getState())
    {
        refreshOtherMixerVolume();
    }

    shiftHandler();
}

// parametr options - przycisk modulu z ktorego nastapilo wejscie do mastera
void cMasterParams::start(uint32_t options)
{
    moduleRefresh = 1;

    //	displayType = display_t::masterValues;

    sourceModuleButton = options & 0xff;

    // drugi bajt to numer podekranu
    uint8_t moduleMode = (options >> 8) & 0xff;
    if (moduleMode == 1)
        displayType = displayMode::masterValues;
    else if (moduleMode == 2)
        displayType = displayMode::mixer;
    else if (moduleMode == 3)
        displayType = displayMode::mixerDelayReverb;

    //--------------------------------------------------------------------
    // ustawienie funkcji
    ip->setButtonObj(interfaceButtonParams, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonPerformance, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonFile, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonConfig, buttonPress, functSwitchModule);
    // ip->setButtonObj(interfaceButtonMaster, buttonPress, functSwitchModule);

    ip->setButtonObj(interfaceButtonMaster, functSwitchModeMaster);

    ip->setButtonObj(interfaceButtonSamplePlay, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSampleEdit, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSampleRec, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSampleLoad, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSong, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonPattern, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonGame, buttonPress, functSwitchModule);

    ip->setButtonObj(interfaceButtonMiniMenu, functMenu);

    ip->setPadsGlobal(functPads);

    isSolo = false;

    showMasterScreen();
    setMasterScreenFunct();
    activateLabelsBorder();

    if (displayType == displayMode::mixer)
    {
        switchToMixer();
    }
    else if (displayType == displayMode::masterValues)
    {
        if (isDelayScreen)
            switchToDelayScreen();
        else if (isReverbScreen)
            switchToReverbScreen();
        else if (isLimiterScreen)
            switchToLimiterScreen();
        else if (isEqualizerScreen)
            switchToEqualizerScreen();
        else
            switchToMaster();
    }
    else if (displayType == cMasterParams::displayMode::mixerDelayReverb)
    {
        switchToMixerDelayReverb();
    }

    displayTypeBeforeMasterPress = displayType;
    isDelayScreenBeforeMasterPress = isDelayScreen;
    isReverbScreenBeforeMasterPress = isReverbScreen;
    isLimiterScreenBeforeMasterPress = isLimiterScreen;
    isEqualizerScreenBeforeMasterPress = isEqualizerScreen;
    selectedPlaceBeforeMasterPress = selectedPlace;
    showMasterVolumeOnHold = true;
    resetMixerLevels();
    resetMixerDelayReverbLevels();

    lightUpPadBoard();
}

void cMasterParams::stop()
{
    sequencer.stopManualNotes();
    moduleRefresh = 0;
}

void cMasterParams::hideControlsUnderMiniMenu()
{
    hideControlUnderMenu();
}

uint8_t cMasterParams::getCurrentModeIdx()
{
    return displayType == displayMode::masterValues ? 0
                                                    : (displayType == displayMode::mixer ? 1 : 2);
}

void cMasterParams::setMasterScreenFunct()
{
    // funkcje
    ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    ip->clearAllPots();

    ip->setButtonObj(interfaceButtonPlay, buttonPress, functPlayAction);
    ip->setButtonObj(interfaceButtonRec, buttonPress, functRecAction);

    ip->setButtonObj(interfaceButtonLeft, buttonPress, functLeft);
    ip->setButtonObj(interfaceButtonRight, buttonPress, functRight);
    ip->setButtonObj(interfaceButtonUp, buttonPress, functUp);
    ip->setButtonObj(interfaceButtonDown, buttonPress, functDown);
    ip->setButtonObj(interfaceButtonDelete, buttonPress, functDelete);

    ip->setButtonObj(interfaceButton0, functSelectVolume);
    ip->setButtonObj(interfaceButton1, buttonPress, functSwitchToReverbWindow);
    ip->setButtonObj(interfaceButton2, buttonPress, functSwitchToDelayWindow);
    ip->setButtonObj(interfaceButton3, buttonPress, functSwitchToLimiterScreen);
    ip->setButtonObj(interfaceButton4, buttonPress, functSwitchToEqualizerScreen);
    ip->setButtonObj(interfaceButton5, buttonPress, functSelectSaturation);
    ip->setButtonObj(interfaceButton6, buttonPress, functSelectBassBoost);
    ip->setButtonObj(interfaceButton7, buttonPress, functSelectStereoBoost);
    ip->setButtonObj(interfaceButtonMiniMiddle, functMiniMiddle);

    ip->setPotObj(interfacePot0, functEncoder);
}

void cMasterParams::setDelayScreenFunct()
{
}

void cMasterParams::setKeyboardExportFunctions()
{
    // funkcje
    ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    ip->clearButtonsRange(interfaceButtonPlay, interfaceButtonRight);
    // ip->clearButton(interfaceButtonMaster);

    ip->setButtonObj(interfaceButtonLeft, buttonPress, functLeftKeyboard);
    ip->setButtonObj(interfaceButtonRight, buttonPress, functRightKeyboard);
    ip->setButtonObj(interfaceButtonUp, buttonPress, functUpKeyboard);
    ip->setButtonObj(interfaceButtonDown, buttonPress, functDownKeyboard);

    ip->setButtonObj(interfaceButton0, buttonPress, functEnterKeyExport);
    // ip->setButtonObj(interfaceButton4,buttonPress, functAutoNameTrackName);

    ip->setButtonObj(interfaceButton6, buttonPress, functCancelEditTrackName);
    ip->setButtonObj(interfaceButton7, buttonPress, functSaveTrackName);

    ip->setButtonObj(interfaceButtonDelete, buttonPress, functDeleteKeyboard);
    ip->setButtonObj(interfaceButtonInsert, buttonPress, functInsertKeyboard);
}

void cMasterParams::shiftHandler()
{
    if (displayType != cMasterParams::displayMode::masterValues)
    {
        return;
    }

    static bool previousState = buttons->isButtonPressed(interfaceButtonShift);

    if (previousState != buttons->isButtonPressed(interfaceButtonShift))
    {
        if (isDelayScreen)
        {
            showLineInDelaySend();
        }
        else if (isReverbScreen)
        {
            showLineInReverbSend();
        }

        previousState = buttons->isButtonPressed(interfaceButtonShift);
    }
}

//==============================================================================================================
// tracker mini middle button
static void functMiniMiddle(uint8_t state)
{
    if (state != buttonPress)
    {
        return;
    }

    if (MP->keyboardManager.getState())
    {
        MP->keyboardManager.confirmKey();
    }
}

// ##############################################################################################
// ###############################        ACTION BUTTONS        #################################
// ##############################################################################################
//==============================================================================================================

static void functEncoder(int16_t value)
{
#ifdef TRACKER_MINI
    if (MP->keyboardManager.getState())
    {
        MP->keyboardManager.confirmKey();
        return;
    }
#endif

    if (MP->displayType == cMasterParams::displayMode::mixer)
    {
        const uint8_t activeSampleTracks = getProjectActiveSampleTracks();
        for (uint8_t track = 0; track < activeSampleTracks; track++)
        {
            if (MP->trackIsEdited[track])
            {
                MP->changeTrackVolume(value, track);
                MP->ignoreMuteRelease[track] = true;
            }
        }
        return;
    }

    if (MP->displayType == cMasterParams::displayMode::mixerDelayReverb)
    {
        if (MP->delayIsEdited)
        {
            MP->changeDelayVolume(value);
            MP->ignoreMuteReleaseDelay = true;
        }
        if (MP->reverbIsEdited)
        {
            MP->changeReverbVolume(value);
            MP->ignoreMuteReleaseReverb = true;
        }
        if (MP->dryMixIsEdited)
        {
            MP->changeDryMixVolume(value);
            MP->ignoreMuteReleaseDryMix = true;
        }
        if (MP->lineInIsEdited)
        {
            MP->changeLineInVolume(value);
            MP->ignoreMuteReleaseLineIn = true;
        }

#ifdef FEATURE_SYNTH
        for (int synthIdx = 0; synthIdx < kSynthParts; synthIdx++)
        {
            if (MP->synthGainIsEdited[synthIdx])
            {
                MP->changeSynthGain(synthIdx, value);
            }
        }
#endif
        return;
    }

    if (MP->isDelayScreen)
    {
        switch (MP->selectedPlaceDelay)
        {
            case 0:
                MP->changeDelayPingPongEnable(interfaceGlobals.invertValForList ? -value : value);
                break;
            case 1:
                MP->changeDelaySyncEnable(interfaceGlobals.invertValForList ? -value : value);
                break;
            case 2: MP->changeDelayRate(interfaceGlobals.invertValForList ? -value : value); break;
            case 3: MP->changeDelayTime(value); break;
            case 4: MP->changeDelayFeedback(value); break;
            case 5: MP->changeDelayFilterFrequency(value); break;
            case 6: MP->changeLineInDelaySend(value); break;
            default: break;
        }
    }
    else if (MP->isReverbScreen)
    {
        switch (MP->selectedPlaceReverb)
        {
            case 0: MP->changeReverbSize(value); break;
            case 1: MP->changeReverbDamp(value); break;
            case 2: MP->changeReverbPredelay(value); break;
            case 3: MP->changeReverbDiffusion(value); break;
            case 4: MP->changeLineInReverbSend(value); break;
            default: break;
        }
    }
    else if (MP->isLimiterScreen)
    {
        switch (MP->selectedPlaceLimiter)
        {
            case 0: changeLimiterAttack(value); break;
            case 1: changeLimiterRelease(value); break;
            case 2: changeLimiterTreshold(value); break;
            case 4:
                changeLimiterSidechain(interfaceGlobals.invertValForList ? -value : value);
                break;
            default: break;
        }
    }
    else if (MP->isEqualizerScreen)
    {
        MP->changeEqualizerBandAdjust(MP->selectedPlaceEqualizer, value);
    }
    else
    {
        switch (MP->selectedPlace)
        {
            case 0: changeVolume(value); break;
            case 5: changeSaturationType(interfaceGlobals.invertValForList ? -value : value); break;
            case 6: changeBassBoost(value); break;
            case 7: changeStereoBoost(value); break;
            default: break;
        }
    }
}

static void functLeft()
{
    if (MP->displayType == cMasterParams::displayMode::mixer)
    {
        return;
    }
    if (MP->displayType == cMasterParams::displayMode::mixerDelayReverb)
    {
        return;
    }

    if (MP->isDelayScreen)
    {
        if (MP->selectedPlaceDelay > 0)
            MP->selectedPlaceDelay--;

        if ((MP->selectedPlaceDelay == 2) && !(mtProject.values.delayParams & 0b01000000))
            MP->selectedPlaceDelay = 1;
        else if ((MP->selectedPlaceDelay == 3) && (mtProject.values.delayParams & 0b01000000))
            MP->selectedPlaceDelay = 2;

        MP->refreshDelayFrame();
    }
    else if (MP->isReverbScreen)
    {
        if (MP->selectedPlaceReverb > 0)
            MP->selectedPlaceReverb--;
        MP->refreshReverbFrame();
    }
    else if (MP->isLimiterScreen)
    {
        if (MP->selectedPlaceLimiter == 4)
            MP->selectedPlaceLimiter = 2;
        else if (MP->selectedPlaceLimiter > 0)
            MP->selectedPlaceLimiter--;
        MP->refreshLimiterFrame();
    }
    else if (MP->isEqualizerScreen)
    {
        if (MP->selectedPlaceEqualizer > 0)
            MP->selectedPlaceEqualizer--;
        MP->refreshEqualizerFrame();
    }
    else
    {
        if (MP->selectedPlace > 0)
        {
            if (MP->selectedPlace == 5)
            {
                MP->selectedPlace = 0;
            }
            else if (MP->selectedPlace == 6)
            {
                MP->selectedPlace = 5;
            }
            else if (MP->selectedPlace == 7)
            {
                MP->selectedPlace = 6;
            }
        }
        MP->activateLabelsBorder();
    }
}

static void functRight()
{
    if (MP->displayType == cMasterParams::displayMode::mixer)
    {
        return;
    }
    if (MP->displayType == cMasterParams::displayMode::mixerDelayReverb)
    {
        return;
    }

    if (MP->isDelayScreen)
    {
        if (MP->selectedPlaceDelay < MP->frameData.placesCount - 1)
            MP->selectedPlaceDelay++;

        if ((MP->selectedPlaceDelay == 2) && !(mtProject.values.delayParams & 0b01000000))
            MP->selectedPlaceDelay = 3;
        else if ((MP->selectedPlaceDelay == 3) && (mtProject.values.delayParams & 0b01000000))
            MP->selectedPlaceDelay = 4;

        MP->refreshDelayFrame();
    }
    else if (MP->isReverbScreen)
    {
        if (MP->selectedPlaceReverb < MP->frameData.placesCount - 1)
            MP->selectedPlaceReverb++;
        MP->refreshReverbFrame();
    }
    else if (MP->isLimiterScreen)
    {
        if (MP->selectedPlaceLimiter == 2)
            MP->selectedPlaceLimiter = 4;
        else if (MP->selectedPlaceLimiter < MP->frameData.placesCount - 1)
            MP->selectedPlaceLimiter++;
        MP->refreshLimiterFrame();
    }
    else if (MP->isEqualizerScreen)
    {
        if (MP->selectedPlaceEqualizer < MP->frameData.placesCount - 1)
            MP->selectedPlaceEqualizer++;
        MP->refreshEqualizerFrame();
    }
    else
    {
        if (MP->selectedPlace < MP->frameData.placesCount - 1)
        {
            if (MP->selectedPlace == 0)
            {
                MP->selectedPlace = 5;
            }
            else if (MP->selectedPlace == 5)
            {
                MP->selectedPlace = 6;
            }
            else if (MP->selectedPlace == 6)
            {
                MP->selectedPlace = 7;
            }
        }
        MP->activateLabelsBorder();
    }
}

static void functUp()
{
    if (MP->displayType == cMasterParams::displayMode::mixer)
    {
        const uint8_t activeSampleTracks = getProjectActiveSampleTracks();
        for (uint8_t track = 0; track < activeSampleTracks; track++)
        {
            if (MP->trackIsEdited[track])
            {
                MP->changeTrackVolume(1, track);
                MP->ignoreMuteRelease[track] = true;
            }
        }

        return;
    }

    if (MP->displayType == cMasterParams::displayMode::mixerDelayReverb)
    {
        if (MP->delayIsEdited)
        {
            MP->changeDelayVolume(1);
            MP->ignoreMuteReleaseDelay = true;
        }
        if (MP->reverbIsEdited)
        {
            MP->changeReverbVolume(1);
            MP->ignoreMuteReleaseReverb = true;
        }
        if (MP->dryMixIsEdited)
        {
            MP->changeDryMixVolume(1);
            MP->ignoreMuteReleaseDryMix = true;
        }
        if (MP->lineInIsEdited)
        {
            MP->changeLineInVolume(1);
            MP->ignoreMuteReleaseLineIn = true;
        }
#ifdef FEATURE_SYNTH
        for (int synthIdx = 0; synthIdx < kSynthParts; synthIdx++)
        {
            if (MP->synthGainIsEdited[synthIdx])
            {
                MP->changeSynthGain(synthIdx, 1);
            }
        }
#endif

        return;
    }

    if (MP->isDelayScreen)
    {
        switch (MP->selectedPlaceDelay)
        {
            case 0: MP->changeDelayPingPongEnable(-1); break;
            case 1: MP->changeDelaySyncEnable(-1); break;
            case 2: MP->changeDelayRate(-1); break;
            case 3: MP->changeDelayTime(1); break;
            case 4: MP->changeDelayFeedback(1); break;
            case 5: MP->changeDelayFilterFrequency(1); break;
            case 6: MP->changeLineInDelaySend(1); break;
            default: break;
        }
    }
    else if (MP->isReverbScreen)
    {
        switch (MP->selectedPlaceReverb)
        {
            case 0: MP->changeReverbSize(1); break;
            case 1: MP->changeReverbDamp(1); break;
            case 2: MP->changeReverbPredelay(1); break;
            case 3: MP->changeReverbDiffusion(1); break;
            case 4: MP->changeLineInReverbSend(1); break;
            default: break;
        }
    }
    else if (MP->isLimiterScreen)
    {
        switch (MP->selectedPlaceLimiter)
        {
            case 0: changeLimiterAttack(1); break;
            case 1: changeLimiterRelease(1); break;
            case 2: changeLimiterTreshold(1); break;
            case 4: changeLimiterSidechain(-1); break;
            default: break;
        }
    }
    else if (MP->isEqualizerScreen)
    {
        MP->changeEqualizerBandAdjust(MP->selectedPlaceEqualizer, 1);
    }
    else
    {
        switch (MP->selectedPlace)
        {
            case 0: changeVolume(1); break;
            case 5: changeSaturationType(-1); break;
            case 6: changeBassBoost(1); break;
            case 7: changeStereoBoost(1); break;
            default: break;
        }
    }
}

static void functDown()
{
    if (MP->displayType == cMasterParams::displayMode::mixer)
    {
        const uint8_t activeSampleTracks = getProjectActiveSampleTracks();
        for (uint8_t track = 0; track < activeSampleTracks; track++)
        {
            if (MP->trackIsEdited[track])
            {
                MP->changeTrackVolume(-1, track);
                MP->ignoreMuteRelease[track] = true;
            }
        }

        return;
    }

    if (MP->displayType == cMasterParams::displayMode::mixerDelayReverb)
    {
        if (MP->delayIsEdited)
        {
            MP->changeDelayVolume(-1);
            MP->ignoreMuteReleaseDelay = true;
        }
        if (MP->reverbIsEdited)
        {
            MP->changeReverbVolume(-1);
            MP->ignoreMuteReleaseReverb = true;
        }
        if (MP->dryMixIsEdited)
        {
            MP->changeDryMixVolume(-1);
            MP->ignoreMuteReleaseDryMix = true;
        }
        if (MP->lineInIsEdited)
        {
            MP->changeLineInVolume(-1);
            MP->ignoreMuteReleaseLineIn = true;
        }
#ifdef FEATURE_SYNTH
        for (int synthIdx = 0; synthIdx < kSynthParts; synthIdx++)
        {
            if (MP->synthGainIsEdited[synthIdx])
            {
                MP->changeSynthGain(synthIdx, -1);
            }
        }
#endif
        return;
    }

    if (MP->isDelayScreen)
    {
        switch (MP->selectedPlaceDelay)
        {
            case 0: MP->changeDelayPingPongEnable(1); break;
            case 1: MP->changeDelaySyncEnable(1); break;
            case 2: MP->changeDelayRate(1); break;
            case 3: MP->changeDelayTime(-1); break;
            case 4: MP->changeDelayFeedback(-1); break;
            case 5: MP->changeDelayFilterFrequency(-1); break;
            case 6: MP->changeLineInDelaySend(-1); break;
            default: break;
        }
    }
    else if (MP->isReverbScreen)
    {
        switch (MP->selectedPlaceReverb)
        {
            case 0: MP->changeReverbSize(-1); break;
            case 1: MP->changeReverbDamp(-1); break;
            case 2: MP->changeReverbPredelay(-1); break;
            case 3: MP->changeReverbDiffusion(-1); break;
            case 4: MP->changeLineInReverbSend(-1); break;
            default: break;
        }
    }
    else if (MP->isLimiterScreen)
    {
        switch (MP->selectedPlaceLimiter)
        {
            case 0: changeLimiterAttack(-1); break;
            case 1: changeLimiterRelease(-1); break;
            case 2: changeLimiterTreshold(-1); break;
            case 4: changeLimiterSidechain(1); break;
            default: break;
        }
    }
    else if (MP->isEqualizerScreen)
    {
        MP->changeEqualizerBandAdjust(MP->selectedPlaceEqualizer, -1);
    }
    else
    {
        switch (MP->selectedPlace)
        {
            case 0: changeVolume(-1); break;
            case 5: changeSaturationType(1); break;
            case 6: changeBassBoost(-1); break;
            case 7: changeStereoBoost(-1); break;
            default: break;
        }
    }
}

static void functDelete()
{
    if (MP->displayType == cMasterParams::displayMode::mixer)
    {
        const uint8_t activeSampleTracks = getProjectActiveSampleTracks();
        for (uint8_t track = 0; track < activeSampleTracks; track++)
        {
            if (MP->trackIsEdited[track])
            {
                MP->setDefaultTrackVolume(track);
                MP->ignoreMuteRelease[track] = true;
            }
        }
        return;
    }

    if (MP->displayType == cMasterParams::displayMode::mixerDelayReverb)
    {
        if (MP->delayIsEdited)
        {
            MP->setDefaultDelayVolume();
            MP->ignoreMuteReleaseDelay = true;
        }
        if (MP->reverbIsEdited)
        {
            MP->setDefaultReverbVolume();
            MP->ignoreMuteReleaseReverb = true;
        }
        if (MP->dryMixIsEdited)
        {
            MP->setDefaultDryMixVolume();
            ;
            MP->ignoreMuteReleaseDryMix = true;
        }
        if (MP->lineInIsEdited)
        {
            MP->setDefaultLineInVolume();
            MP->ignoreMuteReleaseLineIn = true;
        }

#ifdef FEATURE_SYNTH
        for (int synthIdx = 0; synthIdx < kSynthParts; synthIdx++)
        {
            if (MP->synthGainIsEdited[synthIdx])
            {
                MP->setDefaultSynthGain(synthIdx);
            }
        }
#endif
        return;
    }

    if (MP->isDelayScreen)
    {
        switch (MP->selectedPlaceDelay)
        {
            case 0: MP->setDefaultDelayPingPongEnable(); break;
            case 1: MP->setDefaultDelaySyncEnable(); break;
            case 2: MP->setDefaultDelayRate(); break;
            case 3: MP->setDefaultDelayTime(); break;
            case 4: MP->setDefaultDelayFeedback(); break;
            case 5: MP->setDefaultDelayFilterFrequency(); break;
            case 6: MP->setDefaultLineInDelaySend(); break;
            default: break;
        }
    }
    else if (MP->isReverbScreen)
    {
        switch (MP->selectedPlaceReverb)
        {
            case 0: MP->setDefaultReverbSize(); break;
            case 1: MP->setDefaultReverbDamp(); break;
            case 2: MP->setDefaultReverbPredelay(); break;
            case 3: MP->setDefaultReverbDiffusion(); break;
            case 4: MP->setDefaultLineInReverbSend(); break;
            default: break;
        }
    }
    else if (MP->isLimiterScreen)
    {
        switch (MP->selectedPlaceLimiter)
        {
            case 0: setDefaultLimiterAttack(); break;
            case 1: setDefaultLimiterRelease(); break;
            case 2: setDefaultLimiterTreshold(); break;
            case 4: setDefaultLimiterSidechain(); break;
            default: break;
        }
    }
    else if (MP->isEqualizerScreen)
    {
        MP->setDefaultEqualizerBandAdjust(MP->selectedPlaceEqualizer);
    }
    else
    {
        switch (MP->selectedPlace)
        {
            case 0: setDefaultMasterVolume(); break;
            case 5: setDefaultSaturationType(); break;
            case 6: setDefaultBassBoost(); break;
            case 7: setDefaultStereoBoost(); break;
            default: break;
        }
    }
}

static void functPlayAction()
{
    if (MP->sourceModuleButton == interfaceButtonSampleRec)
    {
        return;
    }

    if (sequencer.getSeqState() == Sequencer::SEQ_STATE_STOP)
    {
        if (MP->buttons->isButtonPressed(interfaceButtonShift))
        {
            sequencer.playSong();
        }
        else
        {
            sequencer.playPattern();
        }
    }
    else
    {
        sequencer.stop();
    }
}

static void functRecAction()
{
}

static void functSelectVolume(uint8_t state)
{
    if (state > buttonPress)
    {
        return;
    }

    uint8_t node = 0;

    if (state == buttonPress)
    {
        MP->selectedPlace = node;
    }
    MP->activateLabelsBorder();
}

static void functSwitchToLimiterScreen()
{
    MP->switchToLimiterScreen();
}

static void functSwitchToEqualizerScreen()
{
    MP->switchToEqualizerScreen();
}

static void functSelectSaturation()
{
    if (MP->selectedPlace == 5)
    {
        if ((int)mtProject.values.saturationType < (int)SaturationType::kSaturationTypeEnd - 1)
        {
            mtProject.values.saturationType++;
        }
        else
        {
            mtProject.values.saturationType = (uint8_t)SaturationType::kSaturationMin;
        }

        engine.setSaturationType((SaturationType)mtProject.values.saturationType);

        newFileManager.setProjectStructChanged();

        MP->showSaturationType();
    }
    else
    {
        MP->selectedPlace = 5;
        MP->activateLabelsBorder();
    }
}

static void functSelectBassBoost()
{
    MP->selectedPlace = 6;
    MP->activateLabelsBorder();
}

static void functSelectStereoBoost()
{
    MP->selectedPlace = 7;
    MP->activateLabelsBorder();
}

static void functSwitchModule(uint8_t button)
{
    if (!performanceMode.canPerformanceModeContinueAfterSwitchingTo(button))
    {
        performanceMode.turnOffAtEndOfPattern();
    }

    if (MP->sourceModuleButton == interfaceButtonSampleRec)
    {
        // The handler reads the options through a uint32_t pointer.
        uint32_t option = button;
        MP->eventFunct(eventSwitchModule, MP, &MP->sourceModuleButton, &option);
    }
    else
    {
        MP->eventFunct(eventSwitchModule, MP, &button, 0);
    }
}

void cMasterParams::switchScreen()
{
    selectedPlace = 0;

    if (displayType == displayMode::masterValues)
    {
        switchToMixer();
    }
    else if (displayType == displayMode::mixerDelayReverb)
    {
        if (isDelayScreen)
            switchToDelayScreen();
        else if (isReverbScreen)
            switchToReverbScreen();
        else if (isLimiterScreen)
            switchToLimiterScreen();
        else if (isEqualizerScreen)
            switchToEqualizerScreen();
        else
            switchToMaster();
    }
    else if (displayType == cMasterParams::displayMode::mixer)
    {
        switchToMixerDelayReverb();
    }
}

void cMasterParams::returnToScreen()
{
    selectedPlace = selectedPlaceBeforeMasterPress;

    if (displayTypeBeforeMasterPress == displayMode::mixer)
    {
        switchToMixer();
    }
    else if (displayTypeBeforeMasterPress == displayMode::masterValues)
    {
        if (isDelayScreenBeforeMasterPress)
            MP->switchToDelayScreen();
        else if (isReverbScreenBeforeMasterPress)
            MP->switchToReverbScreen();
        else if (isLimiterScreenBeforeMasterPress)
            MP->switchToLimiterScreen();
        else if (isEqualizerScreenBeforeMasterPress)
            MP->switchToEqualizerScreen();
        else
            switchToMaster();
    }
    else if (displayTypeBeforeMasterPress == cMasterParams::displayMode::mixerDelayReverb)
    {
        switchToMixerDelayReverb();
    }
}
void cMasterParams::switchToMaster()
{
    ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    ip->clearButton(interfaceButtonShift);

    ip->setButtonObj(interfaceButton0, functSelectVolume);
    ip->setButtonObj(interfaceButton1, buttonPress, functSwitchToReverbWindow);
    ip->setButtonObj(interfaceButton2, buttonPress, functSwitchToDelayWindow);
    ip->setButtonObj(interfaceButton3, buttonPress, functSwitchToLimiterScreen);
    ip->setButtonObj(interfaceButton4, buttonPress, functSwitchToEqualizerScreen);
    ip->setButtonObj(interfaceButton5, buttonPress, functSelectSaturation);
    ip->setButtonObj(interfaceButton6, buttonPress, functSelectBassBoost);
    ip->setButtonObj(interfaceButton7, buttonPress, functSelectStereoBoost);

    keyboardManager.deactivateKeyboard();

    displayType = displayMode::masterValues;
    isDelayScreen = false;
    isReverbScreen = false;
    isLimiterScreen = false;
    isEqualizerScreen = false;

    showMasterScreen();
}
void cMasterParams::switchToMixer()
{
    ip->clearButtonsRange(interfaceButton0, interfaceButton7);

    ip->setButtonObj(interfaceButton0, functSoloMuteTrack);

    ip->setButtonObj(interfaceButton1, functSoloMuteTrack);
    ip->setButtonObj(interfaceButton2, functSoloMuteTrack);

    ip->setButtonObj(interfaceButton3, functSoloMuteTrack);
    ip->setButtonObj(interfaceButton4, functSoloMuteTrack);
    ip->setButtonObj(interfaceButton5, functSoloMuteTrack);
    ip->setButtonObj(interfaceButton6, functSoloMuteTrack);
    ip->setButtonObj(interfaceButton6, functSoloMuteTrack);
    ip->setButtonObj(interfaceButton7, functSoloMuteTrack);

    ip->setButtonObj(interfaceButtonShift, functSoloMute);

    ip->setButtonObj(interfaceButtonPlay, buttonPress, functPlayAction);
    ip->setButtonObj(interfaceButtonRec, buttonPress, functEditModeToggle);

    editTrackNameMode = 0;
    displayType = displayMode::mixer;

    keyboardManager.deactivateKeyboard();

    resetMixerLevels();
    showMixerScreen();
}

void cMasterParams::switchToMixerDelayReverb()
{
    ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    ip->clearButton(interfaceButtonRec);

    ip->setButtonObj(interfaceButton0, functSoloMuteDelay);
    ip->setButtonObj(interfaceButton1, functSoloMuteReverb);
    ip->setButtonObj(interfaceButton2, functSoloMuteDryMix);
    ip->setButtonObj(interfaceButton3, functSoloMuteLineIn);
#ifdef FEATURE_SYNTH
    ip->setButtonObj(interfaceButton4, functSelectSynth1);
    ip->setButtonObj(interfaceButton5, functSelectSynth2);
    ip->setButtonObj(interfaceButton6, functSelectSynth3);
#endif
    ip->setButtonObj(interfaceButtonShift, functSoloMuteDelayReverb);

    ip->setButtonObj(interfaceButtonPlay, buttonPress, functPlayAction);

    keyboardManager.deactivateKeyboard();

    editTrackNameMode = 0;
    displayType = displayMode::mixerDelayReverb;

    resetMixerDelayReverbLevels();
    showMixerDelayReverbScreen();
}

void cMasterParams::switchToDelayScreen()
{
    ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    ip->clearButton(interfaceButtonShift);

    ip->setButtonObj(interfaceButton0, buttonPress, functSelectDelayPingpongEnable);
    ip->setButtonObj(interfaceButton1, buttonPress, functSelectDelaySyncEnable);
    ip->setButtonObj(interfaceButton2, buttonPress, functSelectDelaySyncRate);
    ip->setButtonObj(interfaceButton3, buttonPress, functSelectDelayTime);
    ip->setButtonObj(interfaceButton4, buttonPress, functSelectDelayFeedback);
    ip->setButtonObj(interfaceButton5, buttonPress, functSelectDelayFilter);
    ip->setButtonObj(interfaceButton6, buttonPress, functDelayLineIn);
    ip->setButtonObj(interfaceButton7, buttonPress, functDelayCancel);

    keyboardManager.deactivateKeyboard();

    showDelayScreen();

    displayType = displayMode::masterValues;
    isDelayScreen = true;
}

void cMasterParams::switchToReverbScreen()
{
    ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    ip->clearButton(interfaceButtonShift);

    ip->setButtonObj(interfaceButton0, buttonPress, functSelectReverbSize);
    ip->setButtonObj(interfaceButton1, buttonPress, functSelectReverbDamp);
    ip->setButtonObj(interfaceButton2, buttonPress, functSelectReverbPredelay);
    ip->setButtonObj(interfaceButton3, buttonPress, functSelectReverbDiffusion);
    ip->setButtonObj(interfaceButton4, buttonPress, functReverbLineIn);
    ip->setButtonObj(interfaceButton7, buttonPress, functReverbCancel);

    keyboardManager.deactivateKeyboard();

    showReverbScreen();

    displayType = displayMode::masterValues;
    isReverbScreen = true;
}

void cMasterParams::switchToLimiterScreen()
{
    ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    ip->clearButton(interfaceButtonShift);

    ip->setButtonObj(interfaceButton0, buttonPress, functSelectLimiterAttack);
    ip->setButtonObj(interfaceButton1, buttonPress, functSelectLimiterRelease);
    ip->setButtonObj(interfaceButton2, buttonPress, functSelectLimiterThreshold);
    ip->setButtonObj(interfaceButton4, buttonPress, functSelectLimiterSidechain);
    ip->setButtonObj(interfaceButton7, buttonPress, functLimiterCancel);

    keyboardManager.deactivateKeyboard();

    showLimiterScreen();

    displayType = displayMode::masterValues;
    isLimiterScreen = true;
}

void cMasterParams::switchToEqualizerScreen()
{
    ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    ip->clearButton(interfaceButtonShift);

    ip->setButtonObj(interfaceButton0, functSelectEqualizerBand);
    ip->setButtonObj(interfaceButton1, functSelectEqualizerBand);
    ip->setButtonObj(interfaceButton2, functSelectEqualizerBand);
    ip->setButtonObj(interfaceButton3, functSelectEqualizerBand);
    ip->setButtonObj(interfaceButton4, functSelectEqualizerBand);

    ip->setButtonObj(interfaceButton7, buttonPress, functEqualizerCancel);

    keyboardManager.deactivateKeyboard();

    showEqualizerScreen();

    displayType = displayMode::masterValues;
    isEqualizerScreen = true;
}

// delay
static void functSwitchToDelayWindow()
{
    MP->switchToDelayScreen();
}

static void functSelectDelayPingpongEnable()
{
    if (MP->selectedPlaceDelay == 0) // zmienianie pozycji listy przyciskiem pod ekranem
    {
        MP->changeDelayPingPongEnable(1, true);
    }

    MP->selectedPlaceDelay = 0;
    MP->refreshDelayFrame();
}

static void functSelectDelaySyncEnable()
{
    if (MP->selectedPlaceDelay == 1) // zmienianie pozycji listy przyciskiem pod ekranem
    {
        MP->changeDelaySyncEnable(1, true);
    }

    MP->selectedPlaceDelay = 1;
    MP->refreshDelayFrame();
}

static void functSelectDelaySyncRate()
{
    if (!(mtProject.values.delayParams & 0b01000000))
    {
        return;
    }

    if (MP->selectedPlaceDelay == 2) // zmienianie pozycji listy przyciskiem pod ekranem
    {
        MP->changeDelayRate(1, true);
    }

    MP->selectedPlaceDelay = 2;
    MP->refreshDelayFrame();
}

static void functSelectDelayTime()
{
    if (mtProject.values.delayParams & 0b01000000)
    {
        return;
    }
    MP->selectedPlaceDelay = 3;
    MP->refreshDelayFrame();
}

static void functSelectDelayFeedback()
{
    MP->selectedPlaceDelay = 4;
    MP->refreshDelayFrame();
}

static void functSelectDelayFilter()
{
    MP->selectedPlaceDelay = 5;
    MP->refreshDelayFrame();
}

static void functDelayLineIn()
{
    if (MP->buttons->isButtonPressed(interfaceButtonShift))
    {
        mtProject.values.lineInDelaySendMute = !mtProject.values.lineInDelaySendMute;
        engine.refreshLineInStreamDelaySend();
        MP->showLineInDelaySend();
    }
    else
    {
        MP->selectedPlaceDelay = 6;
        MP->refreshDelayFrame();
    }
}

static void functDelayCancel()
{
    MP->switchToMaster();
}

// reverb
static void functSwitchToReverbWindow()
{
    MP->switchToReverbScreen();
}

static void functSelectReverbSize()
{
    MP->selectedPlaceReverb = 0;
    MP->refreshReverbFrame();
}

static void functSelectReverbDamp()
{
    MP->selectedPlaceReverb = 1;
    MP->refreshReverbFrame();
}

static void functSelectReverbPredelay()
{
    MP->selectedPlaceReverb = 2;
    MP->refreshReverbFrame();
}

static void functSelectReverbDiffusion()
{
    MP->selectedPlaceReverb = 3;
    MP->refreshReverbFrame();
}

static void functReverbLineIn()
{
    if (MP->buttons->isButtonPressed(interfaceButtonShift))
    {
        mtProject.values.lineInReverbSendMute = !mtProject.values.lineInReverbSendMute;
        engine.refreshLineInStreamReverbSend();
        MP->showLineInReverbSend();
    }
    else
    {
        MP->selectedPlaceReverb = 4;
        MP->refreshReverbFrame();
    }
}

static void functReverbCancel()
{
    MP->switchToMaster();
}

// limiter screen
static void functSelectLimiterAttack()
{
    MP->selectedPlaceLimiter = 0;
    MP->refreshLimiterFrame();
}

static void functSelectLimiterRelease()
{
    MP->selectedPlaceLimiter = 1;
    MP->refreshLimiterFrame();
}

static void functSelectLimiterThreshold()
{
    MP->selectedPlaceLimiter = 2;
    MP->refreshLimiterFrame();
}

static void functSelectLimiterSidechain()
{
    if (MP->selectedPlaceLimiter == 4) // zmienianie pozycji listy przyciskiem pod ekranem
    {
        if (MP->limiterSidechainVisibleCount > 0)
        {
            uint8_t visibleIdx =
              MP->getLimiterSidechainVisibleIdx(MP->getEffectiveLimiterSidechain());
            visibleIdx = (visibleIdx + 1) % MP->limiterSidechainVisibleCount;
            mtProject.values.limiterSidechain = MP->limiterSidechainVisibleValues[visibleIdx];

            engine.setLimiterSidechain(MP->getEffectiveLimiterSidechain());
            newFileManager.setProjectStructChanged();
            MP->showLimiterSidechain();
        }
        return;
    }

    MP->selectedPlaceLimiter = 4;
    MP->refreshLimiterFrame();
}

static void functLimiterCancel()
{
    MP->switchToMaster();
}

// equalizer
static void functSelectEqualizerBand(uint8_t n, uint8_t state)
{
    if (state == buttonPress)
    {
        MP->selectedPlaceEqualizer = n;
        MP->refreshEqualizerFrame();
    }
}

static void functEqualizerCancel()
{
    MP->switchToMaster();
}

void cMasterParams::calcTrackLevel(uint8_t n)
{
    float localTrackLevel = instrumentPlayer[n].getRMSValue();
    if (localTrackLevel == -1.0f)
    {
        return;
    }

    calcLevel(&trackLevel[n], localTrackLevel);
}

void cMasterParams::calcDelayLevel()
{
    for (uint8_t i = 0; i < 2; i++)
    {
        localDelayLevel[i] = engine.getDelayRMS(i);
        if (localDelayLevel[i] != -1.0f)
        {
            calcLevel(&delayLevel[i], localDelayLevel[i]);
        }
    }
}

void cMasterParams::calcReverbLevel()
{
    for (uint8_t i = 0; i < 2; i++)
    {
        localReverbLevel[i] = engine.getReverbRMS(i);
        if (localReverbLevel[i] != -1.0f)
        {
            calcLevel(&reverbLevel[i], localReverbLevel[i]);
        }
    }
}

void cMasterParams::calcLineInLevel()
{
    for (uint8_t i = 0; i < 2; i++)
    {
        localLineInLevel[i] = engine.getLineInRMS(i);
        if (localLineInLevel[i] != -1.0f)
        {
            calcLevel(&lineInLevel[i], localLineInLevel[i]);
        }
    }
}

#ifdef FEATURE_SYNTH
void cMasterParams::calcSynthLevels()
{
    for (int synthIdx = 0; synthIdx < kSynthParts; synthIdx++)
    {
        localSynthVoiceLevel[synthIdx] =
          (engine.getSynthVoiceRMS(0, synthIdx) + engine.getSynthVoiceRMS(1, synthIdx)) / 2.0f;
        if (localSynthVoiceLevel[synthIdx] != -1.0f)
        {
            calcLevel(&synthLevel[synthIdx], localSynthVoiceLevel[synthIdx]);
        }
    }
}
#endif

// musi byc wywolana po  calcDelayLevel i calcReverbLevel i calc LineInLevel
void cMasterParams::calcDryMixLevel()
{
    for (uint8_t i = 0; i < 2; i++)
    {
        localDryMixLevel[i] = engine.getDryMixRMS(i);
        if (localDryMixLevel[i] != -1.0f)
        {
            calcLevel(&dryMixLevel[i], localDryMixLevel[i]);
        }
    }
}

void cMasterParams::gainReduceHandler()
{
    constexpr uint8_t MEASURE_NUMBER = 30;

    AudioEffectLimiter* limiter[AudioOutput::OutputChannels::kChannelsCount];

    limiter[AudioOutput::OutputChannels::kLeft] =
      engine.getAudioOutput()->getLimiter(AudioOutput::OutputChannels::kLeft);
    limiter[AudioOutput::OutputChannels::kRight] =
      engine.getAudioOutput()->getLimiter(AudioOutput::OutputChannels::kRight);

    if (limiter[AudioOutput::OutputChannels::kLeft]->isAvailableGainToGet() &&
        limiter[AudioOutput::OutputChannels::kRight]->isAvailableGainToGet())
    {
        float leftGainReduce = limiter[AudioOutput::OutputChannels::kLeft]->getAverageGainChange();
        float rightGainReduce =
          limiter[AudioOutput::OutputChannels::kRight]->getAverageGainChange();
        float localAverageLevel = (leftGainReduce + rightGainReduce) / 2.0f;

        gainReduce.measureSum += localAverageLevel;
        gainReduce.measureCounter++;

        if (gainReduce.measureCounter == MEASURE_NUMBER)
        {
            gainReduce.measureCounter = 0;
            gainReduce.value = 1.0f - (gainReduce.measureSum / (float)MEASURE_NUMBER);
            gainReduce.measureSum = 0.0f;
            if (gainReduce.value != gainReduce.lastValue)
            {
                showLimiterGainReduce();
            }
            gainReduce.lastValue = gainReduce.value;
        }
    }
}

float cMasterParams::calcLevelDecybelValue(uint8_t val)
{
    return 20 * log10f((float)mixAmpLogValues[val]);
}

void cMasterParams::calcLevel(strTrackLevel* level, float value)
{
    constexpr float RMS_LEVEL_0DB = 0.71f;
    constexpr int16_t LOW_MIXER_LEVEL_IN_DECIBELS = -36;
    constexpr int16_t UP_MIXER_LEVEL_IN_DECIBELS = 6;
    constexpr uint8_t MIN_LEVEL_BAR = 0;
    constexpr uint8_t CLIP_TRESHOLD_LEVEL_BAR = 95;
    constexpr uint8_t MAX_LEVEL_BAR = 100;

    level->measureSum += value;
    level->measureCounter++;

    if (level->measureCounter == 10)
    {
        level->measureCounter = 0;

        float averageMeasure = level->measureSum / 10.0f;
        int16_t averageMeasureInDecibels = (averageMeasure > 0.0f)
                                             ? 10 * log10f(averageMeasure / RMS_LEVEL_0DB)
                                             : LOW_MIXER_LEVEL_IN_DECIBELS;

        if (averageMeasureInDecibels < LOW_MIXER_LEVEL_IN_DECIBELS)
            averageMeasureInDecibels = LOW_MIXER_LEVEL_IN_DECIBELS;
        else if (averageMeasureInDecibels > UP_MIXER_LEVEL_IN_DECIBELS)
            averageMeasureInDecibels = UP_MIXER_LEVEL_IN_DECIBELS;

        uint8_t localLevel = (averageMeasureInDecibels <= 0)
                               ? mapClamped<int>(averageMeasureInDecibels,
                                                 LOW_MIXER_LEVEL_IN_DECIBELS,
                                                 0,
                                                 MIN_LEVEL_BAR,
                                                 CLIP_TRESHOLD_LEVEL_BAR)
                               : mapClamped<int>(averageMeasureInDecibels,
                                                 0,
                                                 UP_MIXER_LEVEL_IN_DECIBELS,
                                                 CLIP_TRESHOLD_LEVEL_BAR,
                                                 MAX_LEVEL_BAR);

        if (((level->timer > 500)) && (level->value != 0))
        {
            level->timer = 0;
            level->value--;
        }
        if (localLevel > (level->value + 1))
            level->value = localLevel;

        level->measureSum = 0;
    }
}

void cMasterParams::resetMixerLevels()
{
    for (uint8_t track = 0; track < 8; track++)
    {
        resetLevel(&trackLevel[track]);

        while (instrumentPlayer[track].getRMSValue() != -1.0f)
            ;
    }
}

void cMasterParams::resetMixerDelayReverbLevels()
{
    for (uint8_t channel = 0; channel < 2; channel++)
    {
        resetLevel(&delayLevel[channel]);
        resetLevel(&reverbLevel[channel]);
        resetLevel(&dryMixLevel[channel]);
        resetLevel(&lineInLevel[channel]);

#ifdef FEATURE_SYNTH
        for (int synthIdx = 0; synthIdx < kSynthParts; synthIdx++)
        {
            resetLevel(&synthLevel[synthIdx]);
        }
#endif

        while (engine.getDelayRMS(channel) != -1.0f)
            ;
        while (engine.getReverbRMS(channel) != -1.0f)
            ;
        while (engine.getDryMixRMS(channel) != -1.0f)
            ;
        while (engine.getLineInRMS(channel) != -1.0f)
            ;

#ifdef FEATURE_SYNTH
        for (int synthIdx = 0; synthIdx < kSynthParts; synthIdx++)
        {
            while (engine.getSynthVoiceRMS(channel, synthIdx) != -1.0f)
                ;
        }
#endif
    }
}

void cMasterParams::resetLevel(strTrackLevel* level)
{
    level->lastValue = 0;
    level->measureCounter = 0;
    level->measureSum = 0.0f;
    level->redColorTimer = 350;
    level->timer = 0;
    level->value = 0;
}

// mixer
// funkcja podpieta do shifta - zamienia mute na solo
static void functSoloMute(uint8_t state)
{
    if (MP->displayType == cMasterParams::displayMode::masterValues)
    {
        return;
    }
    if (MP->editTrackNameMode)
    {
        MP->isSolo = false;
        return;
    }

    if (state == buttonPress || state == buttonRelease)
    {
        MP->isSolo = state;
        MP->showMixerScreen();
    }
}

static void functSoloMuteTrack(uint8_t n, uint8_t state)
{
    if (MP->displayType == cMasterParams::displayMode::masterValues)
    {
        return;
    }

    const uint8_t activeSampleTracks = getProjectActiveSampleTracks();
    if (n >= activeSampleTracks)
    {
        return;
    }

    if (state == buttonRelease)
    {
        if (MP->editTrackNameMode)
        {
            return;
        }

        MP->trackIsEdited[n] = 0;
        if (MP->ignoreMuteRelease[n])
        {
            MP->ignoreMuteRelease[n] = false;
            return;
        }

        if (MP->isSolo)
        {
            if ((uint8_t)MP->soloTrack == n)
            {
                MP->soloTrack = -1;
                for (uint8_t i = 0; i < activeSampleTracks; i++)
                {
                    mtProject.values.trackMute[i] = 0;
                }
            }
            else
            {
                for (uint8_t i = 0; i < activeSampleTracks; i++)
                {
                    mtProject.values.trackMute[i] = 1;
                }
                mtProject.values.trackMute[n] = 0;
                MP->soloTrack = n;
            }
        }
        else
        {
            mtProject.values.trackMute[n] = !mtProject.values.trackMute[n];
            MP->soloTrack = -1;
        }

        for (uint8_t i = 0; i < activeSampleTracks; i++)
        {
            engine.muteTrack(i, mtProject.values.trackMute[i]);
            MP->showLevelBar(i, &MP->trackLevel[i]);
        }
        MP->showMixerScreen();
    }

    if (state == buttonPress)
    {
        if (MP->editTrackNameMode)
        {
            MP->editTrackName(n);
            return;
        }
        MP->trackIsEdited[n] = true;
    }
}

// mixer delay reverb
static void functSoloMuteDelay(uint8_t state)
{
    if (MP->displayType == cMasterParams::displayMode::masterValues)
    {
        return;
    }

    if (state == buttonRelease)
    {
        MP->delayIsEdited = 0;
        if (MP->ignoreMuteReleaseDelay)
        {
            MP->ignoreMuteReleaseDelay = false;
            return;
        }

        if (MP->isSolo)
        {
            if (MP->soloTrack == cMasterParams::delayIsSolo)
            {
                MP->soloTrack = -1;

                mtProject.values.delayMute = 0;
                mtProject.values.dryMixMute = 0;
                mtProject.values.reverbMute = 0;
                mtProject.values.lineInMute = 0;
            }
            else
            {
                mtProject.values.delayMute = 0;
                mtProject.values.dryMixMute = 1;
                mtProject.values.reverbMute = 1;
                mtProject.values.lineInMute = 1;
                MP->soloTrack = cMasterParams::delayIsSolo;
            }
        }
        else
        {
            mtProject.values.delayMute = !mtProject.values.delayMute;
            MP->soloTrack = -1;
        }

        MP->showLevelBar(cMasterParams::delayVolumeScreenPosition, &MP->delayLevel[0]);
        MP->showLevelBar(cMasterParams::delayVolumeScreenPosition + 4, &MP->delayLevel[1]);
        engine.refreshTrackVolume();
        engine.refreshDelayVolume();
        engine.refreshReverbVolume();
        engine.refreshLineInStreamVolume();
        engine.refreshLineInStreamReverbSend();
        engine.refreshLineInStreamDelaySend();
        MP->showMixerDelayReverbScreen();
    }

    if (state == buttonPress)
    {
        MP->delayIsEdited = true;
    }
}

static void functSoloMuteReverb(uint8_t state)
{
    if (MP->displayType == cMasterParams::displayMode::masterValues)
    {
        return;
    }

    if (state == buttonRelease)
    {
        MP->reverbIsEdited = 0;
        if (MP->ignoreMuteReleaseReverb)
        {
            MP->ignoreMuteReleaseReverb = false;
            return;
        }

        if (MP->isSolo)
        {
            if (MP->soloTrack == cMasterParams::reverbIsSolo)
            {
                MP->soloTrack = -1;

                mtProject.values.delayMute = 0;
                mtProject.values.dryMixMute = 0;
                mtProject.values.reverbMute = 0;
                mtProject.values.lineInMute = 0;
            }
            else
            {
                mtProject.values.delayMute = 1;
                mtProject.values.dryMixMute = 1;
                mtProject.values.reverbMute = 0;
                mtProject.values.lineInMute = 1;
                MP->soloTrack = cMasterParams::reverbIsSolo;
            }
        }
        else
        {
            mtProject.values.reverbMute = !mtProject.values.reverbMute;
            MP->soloTrack = -1;
        }

        MP->showLevelBar(cMasterParams::reverbVolumeScreenPosition, &MP->reverbLevel[0]);
        MP->showLevelBar(cMasterParams::reverbVolumeScreenPosition + 4, &MP->reverbLevel[1]);
        engine.refreshTrackVolume();
        engine.refreshDelayVolume();
        engine.refreshReverbVolume();
        engine.refreshLineInStreamVolume();
        engine.refreshLineInStreamReverbSend();
        engine.refreshLineInStreamDelaySend();
        MP->showMixerDelayReverbScreen();
    }

    if (state == buttonPress)
    {
        MP->reverbIsEdited = true;
    }
}

static void functSoloMuteDryMix(uint8_t state)
{
    if (MP->displayType == cMasterParams::displayMode::masterValues)
    {
        return;
    }

    if (state == buttonRelease)
    {
        MP->dryMixIsEdited = false;
        if (MP->ignoreMuteReleaseDryMix)
        {
            MP->ignoreMuteReleaseDryMix = false;
            return;
        }

        if (MP->isSolo)
        {
            if (MP->soloTrack == cMasterParams::dryMixIsSolo)
            {
                MP->soloTrack = -1;

                mtProject.values.delayMute = 0;
                mtProject.values.dryMixMute = 0;
                mtProject.values.reverbMute = 0;
                mtProject.values.lineInMute = 0;
            }
            else
            {
                mtProject.values.delayMute = 1;
                mtProject.values.dryMixMute = 0;
                mtProject.values.reverbMute = 1;
                mtProject.values.lineInMute = 1;
                MP->soloTrack = cMasterParams::dryMixIsSolo;
            }
        }
        else
        {
            mtProject.values.dryMixMute = !mtProject.values.dryMixMute;
            MP->soloTrack = -1;
        }

        MP->showLevelBar(cMasterParams::dryMixVolumeScreenPosition, &MP->dryMixLevel[0]);
        MP->showLevelBar(cMasterParams::dryMixVolumeScreenPosition + 4, &MP->dryMixLevel[1]);
        engine.refreshTrackVolume();
        engine.refreshDelayVolume();
        engine.refreshReverbVolume();
        engine.refreshLineInStreamVolume();
        MP->showMixerDelayReverbScreen();
    }

    if (state == buttonPress)
    {
        MP->dryMixIsEdited = true;
    }
}
#ifdef FEATURE_SYNTH
static void functSelectSynth1(uint8_t state)
{
    if (MP->displayType == cMasterParams::displayMode::masterValues)
    {
        return;
    }

    if (state == buttonPress)
    {
        MP->synthGainIsEdited[0] = true;
    }
    else if (state == buttonRelease)
    {
        MP->synthGainIsEdited[0] = false;
    }
}

static void functSelectSynth2(uint8_t state)
{
    if (MP->displayType == cMasterParams::displayMode::masterValues)
    {
        return;
    }

    if (state == buttonPress)
    {
        MP->synthGainIsEdited[1] = true;
    }
    else if (state == buttonRelease)
    {
        MP->synthGainIsEdited[1] = false;
    }
}

static void functSelectSynth3(uint8_t state)
{
    if (MP->displayType == cMasterParams::displayMode::masterValues)
    {
        return;
    }

    if (state == buttonPress)
    {
        MP->synthGainIsEdited[2] = true;
    }
    else if (state == buttonRelease)
    {
        MP->synthGainIsEdited[2] = false;
    }
}
#endif

static void functSoloMuteLineIn(uint8_t state)
{
    if (MP->displayType == cMasterParams::displayMode::masterValues)
    {
        return;
    }

    if (state == buttonRelease)
    {
        MP->lineInIsEdited = false;
        if (MP->ignoreMuteReleaseLineIn)
        {
            MP->ignoreMuteReleaseLineIn = false;
            return;
        }

        if (MP->isSolo)
        {
            if (MP->soloTrack == cMasterParams::lineInIsSolo)
            {
                MP->soloTrack = -1;

                mtProject.values.delayMute = 0;
                mtProject.values.dryMixMute = 0;
                mtProject.values.reverbMute = 0;
                mtProject.values.lineInMute = 0;
            }
            else
            {
                mtProject.values.delayMute = 1;
                mtProject.values.dryMixMute = 1;
                mtProject.values.reverbMute = 1;
                mtProject.values.lineInMute = 0;
                MP->soloTrack = cMasterParams::lineInIsSolo;
            }
        }
        else
        {
            mtProject.values.lineInMute = !mtProject.values.lineInMute;
            MP->soloTrack = -1;
        }

        MP->showLevelBar(cMasterParams::lineInVolumeScreenPosition, &MP->lineInLevel[0]);
        MP->showLevelBar(cMasterParams::lineInVolumeScreenPosition + 4, &MP->lineInLevel[1]);
        engine.refreshTrackVolume();
        engine.refreshDelayVolume();
        engine.refreshReverbVolume();
        engine.refreshLineInStreamVolume();
        engine.refreshLineInStreamDelaySend();
        engine.refreshLineInStreamReverbSend();
        MP->showMixerDelayReverbScreen();
    }

    if (state == buttonPress)
    {
        MP->lineInIsEdited = true;
    }
}

// funkcja podpieta do shifta - zamienia mute na solo
static void functSoloMuteDelayReverb(uint8_t state)
{
    if (MP->displayType == cMasterParams::displayMode::masterValues)
    {
        return;
    }

    if (state == buttonPress || state == buttonRelease)
    {
        MP->isSolo = state;
        MP->showMixerDelayReverbScreen();
    }
}

static void functSwitchModeMaster(uint8_t state)
{
    if (state == buttonPress)
    {
        MP->displayTypeBeforeMasterPress = MP->displayType;
        MP->isDelayScreenBeforeMasterPress = MP->isDelayScreen;
        MP->isReverbScreenBeforeMasterPress = MP->isReverbScreen;
        MP->isLimiterScreenBeforeMasterPress = MP->isLimiterScreen;
        MP->isEqualizerScreenBeforeMasterPress = MP->isEqualizerScreen;
        MP->selectedPlaceBeforeMasterPress = MP->selectedPlace;

        MP->sourceModuleButton = interfaceButtonMaster;
        MP->showMasterVolumeOnHold = true;
    }
    else if (state == buttonHold)
    {

        if (MP->showMasterVolumeOnHold)
        {
            MP->showMasterVolumeOnHold = false;
            MP->selectedPlace = 0;
            MP->switchToMaster();
        }
    }
    else if (state == buttonRelease)
    {

        if (MP->sourceModuleButton == interfaceButtonMaster)
        {
            if (MP->showMasterVolumeOnHold)
            {
                MP->switchScreen();
            }
            else
            {
                MP->returnToScreen();
            }
        }
        else
        {
            if (!MP->showMasterVolumeOnHold)
            {
                MP->displayType = MP->displayTypeBeforeMasterPress;
                MP->isDelayScreen = MP->isDelayScreenBeforeMasterPress;
                MP->isReverbScreen = MP->isReverbScreenBeforeMasterPress;
                MP->isLimiterScreen = MP->isLimiterScreenBeforeMasterPress;
                MP->isEqualizerScreen = MP->isEqualizerScreenBeforeMasterPress;
                MP->selectedPlace = MP->selectedPlaceBeforeMasterPress;

                if (MP->sourceModuleButton == interfaceButtonSampleRec)
                {
                    MP->eventFunct(eventSwitchToPreviousModule, MP, &MP->sourceModuleButton, 0);
                }
                else
                {
                    MP->eventFunct(eventSwitchToPreviousModule, MP, 0, 0);
                }
            }
        }
    }
}

//======================================================================================================================
// master
void changeVolume(int16_t value)
{
    if (mtConfig.audioCodecConfig.volume + value < MASTER_VOLUME_MIN)
        mtConfig.audioCodecConfig.volume = MASTER_VOLUME_MIN;
    else if (mtConfig.audioCodecConfig.volume + value > MASTER_VOLUME_MAX)
        mtConfig.audioCodecConfig.volume = MASTER_VOLUME_MAX;
    else
        mtConfig.audioCodecConfig.volume += value;

    engine.setHeadphonesVolume(mtConfig.audioCodecConfig.volume);
    saveConfig();

    if (MP->moduleRefresh)
    {
        MP->showVolume();
    }
}

void changeLimiterAttack(int16_t value)
{
    value *= LIMITER_ATTACK_MAX / 1000;

    if (mtProject.values.limiterAttack + value < LIMITER_ATTACK_MIN)
        mtProject.values.limiterAttack = LIMITER_ATTACK_MIN;
    else if (mtProject.values.limiterAttack + value > LIMITER_ATTACK_MAX)
        mtProject.values.limiterAttack = LIMITER_ATTACK_MAX;
    else
        mtProject.values.limiterAttack += value;

    engine.setLimiterAttack(mtProject.values.limiterAttack);

    newFileManager.setProjectStructChanged();

    MP->showLimiterAttack();
}

void changeLimiterRelease(int16_t value)
{
    value *= (LIMITER_RELEASE_MAX / 1000);

    if (mtProject.values.limiterRelease + value < LIMITER_RELEASE_MIN)
        mtProject.values.limiterRelease = LIMITER_RELEASE_MIN;
    else if (mtProject.values.limiterRelease + value > LIMITER_RELEASE_MAX)
        mtProject.values.limiterRelease = LIMITER_RELEASE_MAX;
    else
        mtProject.values.limiterRelease += value;

    engine.setLimiterRelease(mtProject.values.limiterRelease);

    newFileManager.setProjectStructChanged();

    MP->showLimiterRelease();
}

void changeLimiterTreshold(int16_t value)
{
    int32_t val =
      value * LIMITER_THRESHOLD_MAX /
      100; // mnoznik jest na tyle duzy ze przy mocnym pokreceniu iloczyn przekracza 16bit

    if (val > MAX_SIGNED_16BIT)
        value = MAX_SIGNED_16BIT;
    else if (val < MIN_SIGNED_16BIT)
        value = MIN_SIGNED_16BIT;
    else
        value = val;

    if (mtProject.values.limiterThreshold + value < LIMITER_THRESHOLD_MIN)
        mtProject.values.limiterThreshold = LIMITER_THRESHOLD_MIN;
    else if (mtProject.values.limiterThreshold + value > LIMITER_THRESHOLD_MAX)
        mtProject.values.limiterThreshold = LIMITER_THRESHOLD_MAX;
    else
        mtProject.values.limiterThreshold += value;

    engine.setLimiterThreshold(mtProject.values.limiterThreshold);

    newFileManager.setProjectStructChanged();

    MP->showLimiterThreshold();
}

void changeLimiterSidechain(int16_t value)
{
    if (MP->limiterSidechainVisibleCount == 0)
    {
        return;
    }

    const int maxIdx = static_cast<int>(MP->limiterSidechainVisibleCount) - 1;
    int visibleIdx =
      static_cast<int>(MP->getLimiterSidechainVisibleIdx(MP->getEffectiveLimiterSidechain())) +
      static_cast<int>(value);
    visibleIdx = clamp<int>(visibleIdx, 0, maxIdx);
    mtProject.values.limiterSidechain = MP->limiterSidechainVisibleValues[visibleIdx];

    engine.setLimiterSidechain(MP->getEffectiveLimiterSidechain());

    newFileManager.setProjectStructChanged();

    MP->showLimiterSidechain();
}

void changeSaturationType(int16_t value)
{
    mtProject.values.saturationType = clamp<int>((int)mtProject.values.saturationType + (int)value,
                                                 (int)SaturationType::kSaturationMin,
                                                 (int)SaturationType::kSaturationTypeEnd - 1);

    engine.setSaturationType((SaturationType)mtProject.values.saturationType);

    newFileManager.setProjectStructChanged();

    MP->showSaturationType();
}

void changeBassBoost(int16_t value)
{
    if (mtProject.values.bassBoost + value > BASS_BOOST_MAX)
        mtProject.values.bassBoost = BASS_BOOST_MAX;
    else if (mtProject.values.bassBoost + value < BASS_BOOST_MIN)
        mtProject.values.bassBoost = BASS_BOOST_MIN;
    else
        mtProject.values.bassBoost += value;

    masterEnhancer.changeBassBoost(mtProject.values.bassBoost);

    newFileManager.setProjectStructChanged();
    MP->showBassBoost();
}

void changeStereoBoost(int16_t value)
{
    if (mtProject.values.stereoBoost + value > STEREO_BOOST_MAX)
        mtProject.values.stereoBoost = STEREO_BOOST_MAX;
    else if (mtProject.values.stereoBoost + value < STEREO_BOOST_MIN)
        mtProject.values.stereoBoost = STEREO_BOOST_MIN;
    else
        mtProject.values.stereoBoost += value;

    masterEnhancer.changeStereoBoost(mtProject.values.stereoBoost);

    newFileManager.setProjectStructChanged();
    MP->showStereoBoost();
}

void setDefaultMasterVolume()
{
    // zabezpieczenie przed przypadkowym wysadzeniem glosnikow
    int16_t volumeDifference = MASTER_VOLUME_DEFAULT - mtConfig.audioCodecConfig.volume;
    if (volumeDifference >= 10)
    {
        mtConfig.audioCodecConfig.volume += 10;
    }
    else
    {
        mtConfig.audioCodecConfig.volume = MASTER_VOLUME_DEFAULT;
    }

    engine.setHeadphonesVolume(mtConfig.audioCodecConfig.volume);

    saveConfig();

    MP->showVolume();
}

void setDefaultSaturationType()
{
    mtProject.values.saturationType = SaturationType::kSoftClip;

    engine.setSaturationType((SaturationType)mtProject.values.saturationType);

    newFileManager.setProjectStructChanged();

    MP->showSaturationType();
}

void setDefaultLimiterAttack()
{
    mtProject.values.limiterAttack = 100;
    engine.setLimiterAttack(mtProject.values.limiterAttack);

    newFileManager.setProjectStructChanged();

    MP->showLimiterAttack();
}
void setDefaultLimiterRelease()
{
    mtProject.values.limiterRelease = 0.512;

    engine.setLimiterRelease(mtProject.values.limiterRelease);

    newFileManager.setProjectStructChanged();

    MP->showLimiterRelease();
}
void setDefaultLimiterTreshold()
{
    mtProject.values.limiterThreshold = LIMITER_THRESHOLD_DEFAULT;
    engine.setLimiterThreshold(mtProject.values.limiterThreshold);

    newFileManager.setProjectStructChanged();

    MP->showLimiterThreshold();
}

void setDefaultLimiterSidechain()
{
    mtProject.values.limiterSidechain = LIMITER_SIDECHAIN_DEFAULT;
    engine.setLimiterSidechain(mtProject.values.limiterSidechain);

    newFileManager.setProjectStructChanged();

    MP->showLimiterSidechain();
}

void setDefaultBassBoost()
{
    mtProject.values.bassBoost = BASS_BOOST_DEFAULT;

    masterEnhancer.changeBassBoost(mtProject.values.bassBoost);

    newFileManager.setProjectStructChanged();
    MP->showBassBoost();
}
void setDefaultStereoBoost()
{
    mtProject.values.stereoBoost = STEREO_BOOST_DEFAULT;

    masterEnhancer.changeStereoBoost(mtProject.values.stereoBoost);

    newFileManager.setProjectStructChanged();
    MP->showStereoBoost();
}

void cMasterParams::changeDelayPingPongEnable(int16_t val, bool rollListOver)
{
    if (val > 0)
    {
        if (rollListOver && !(mtProject.values.delayParams >> 7 & 1))
            mtProject.values.delayParams |= 0b10000000;
        else
            mtProject.values.delayParams &= 0b01111111;
    }
    else if (val < 0)
        mtProject.values.delayParams |= 0b10000000;

    engine.setDelayParams(mtProject.values.delayParams);

    newFileManager.setProjectStructChanged();

    showDelayPingPongEnable();
}
void cMasterParams::changeDelaySyncEnable(int16_t val, bool rollListOver)
{
    if (val > 0)
    {
        if (rollListOver && !(mtProject.values.delayParams >> 6 & 1))
            mtProject.values.delayParams |= 0b01000000;
        else
            mtProject.values.delayParams &= 0b10111111;
    }
    else if (val < 0)
        mtProject.values.delayParams |= 0b01000000;

    engine.setDelayParams(mtProject.values.delayParams);
    if (val > 0 && (mtProject.values.delayParams >> 6 & 1))
        engine.setDelayTime(mtProject.values.delayTime);

    newFileManager.setProjectStructChanged();

    showDelaySyncEnable();
}
void cMasterParams::changeDelayRate(int16_t val, bool rollListOver)
{
    uint8_t temp = mtProject.values.delayParams & 0b00111111;

    if (temp + val < 0)
        temp = 0;
    else if (temp + val > 18)
        temp = (rollListOver ? 0 : 18);
    else
        temp += val;

    mtProject.values.delayParams &= 0b11000000;
    mtProject.values.delayParams |= temp;

    engine.setDelayParams(mtProject.values.delayParams);

    newFileManager.setProjectStructChanged();

    showDelayRate();
}
void cMasterParams::changeDelayTime(int16_t val)
{
    int mult = mtProject.values.delayTime < 100 ? 1 : (mtProject.values.delayTime < 500 ? 10 : 30);

    val *= mult;

    if (mtProject.values.delayTime + val < DELAY_TIME_MIN)
        mtProject.values.delayTime = DELAY_TIME_MIN;
    else if (mtProject.values.delayTime + val > DELAY_TIME_MAX)
        mtProject.values.delayTime = DELAY_TIME_MAX;
    else
        mtProject.values.delayTime += val;

    engine.setDelayTime(mtProject.values.delayTime);

    newFileManager.setProjectStructChanged();

    showDelayTime();
}

void cMasterParams::changeDelayFilterFrequency(int16_t val)
{
    val *= 100;

    if (mtProject.values.delayFilterFrequency + val < DELAY_FILTER_FREQ_MIN)
        mtProject.values.delayFilterFrequency = DELAY_FILTER_FREQ_MIN;
    else if (mtProject.values.delayFilterFrequency + val > DELAY_FILTER_FREQ_MAX)
        mtProject.values.delayFilterFrequency = DELAY_FILTER_FREQ_MAX;
    else
        mtProject.values.delayFilterFrequency += val;

    engine.setDelayFilterFrequency(mtProject.values.delayFilterFrequency);

    newFileManager.setProjectStructChanged();

    showDelayFilterFrequency();
}

void cMasterParams::changeDelayFeedback(int16_t val)
{
    if (mtProject.values.delayFeedback + val < DELAY_FEEDBACK_MIN)
        mtProject.values.delayFeedback = DELAY_FEEDBACK_MIN;
    else if (mtProject.values.delayFeedback + val > DELAY_FEEDBACK_MAX)
        mtProject.values.delayFeedback = DELAY_FEEDBACK_MAX;
    else
        mtProject.values.delayFeedback += val;

    engine.setDelayFeedback(mtProject.values.delayFeedback);

    newFileManager.setProjectStructChanged();

    showDelayFeedback();
}

void cMasterParams::changeReverbSize(int16_t val)
{
    float v = 0.01 * val;

    if (mtProject.values.reverb.size + v > REVERB_SIZE_MAX)
        mtProject.values.reverb.size = REVERB_SIZE_MAX;
    else if (mtProject.values.reverb.size + v < REVERB_SIZE_MIN)
        mtProject.values.reverb.size = REVERB_SIZE_MIN;
    else
        mtProject.values.reverb.size += v;

    engine.setReverbSize(mtProject.values.reverb.size);

    showReverbSize();
}
void cMasterParams::changeReverbDamp(int16_t val)
{
    float v = 0.01 * val;

    if (mtProject.values.reverb.damp + v > REVERB_DAMP_MAX)
        mtProject.values.reverb.damp = REVERB_DAMP_MAX;
    else if (mtProject.values.reverb.damp + v < REVERB_DAMP_MIN)
        mtProject.values.reverb.damp = REVERB_DAMP_MIN;
    else
        mtProject.values.reverb.damp += v;

    engine.setReverbDamp(mtProject.values.reverb.damp);

    showReverbDamp();

    newFileManager.setProjectStructChanged();
}
void cMasterParams::changeReverbPredelay(int16_t val)
{
    float v = 0.01 * val;

    if (mtProject.values.reverb.predelay + v > REVERB_PREDELAY_MAX)
        mtProject.values.reverb.predelay = REVERB_PREDELAY_MAX;
    else if (mtProject.values.reverb.predelay + v < REVERB_PREDELAY_MIN)
        mtProject.values.reverb.predelay = REVERB_PREDELAY_MIN;
    else
        mtProject.values.reverb.predelay += v;

    engine.setReverbPredelay(mtProject.values.reverb.predelay);

    showReverbPredelay();

    newFileManager.setProjectStructChanged();
}
void cMasterParams::changeReverbDiffusion(int16_t val)
{
    float v = 0.0085 * val;

    if (mtProject.values.reverb.diffusion + v > REVERB_DIFFUSION_MAX)
        mtProject.values.reverb.diffusion = REVERB_DIFFUSION_MAX;
    else if (mtProject.values.reverb.diffusion + v < REVERB_DIFFUSION_MIN)
        mtProject.values.reverb.diffusion = REVERB_DIFFUSION_MIN;
    else
        mtProject.values.reverb.diffusion += v;

    engine.setReverbDiffusion(mtProject.values.reverb.diffusion);

    showReverbDiffusion();

    newFileManager.setProjectStructChanged();
}

// equalizer
void cMasterParams::changeEqualizerBandAdjust(uint8_t n, int16_t val)
{

    float changeStep = (BAND_ADJUSTMENT_MAX - BAND_ADJUSTMENT_MIN) / 100.0f;
    float localValue = val * changeStep;

    if (mtProject.values.eqBandAdjustment[n] + localValue > BAND_ADJUSTMENT_MAX)
    {
        mtProject.values.eqBandAdjustment[n] = BAND_ADJUSTMENT_MAX;
    }
    else if (mtProject.values.eqBandAdjustment[n] + localValue < BAND_ADJUSTMENT_MIN)
    {
        mtProject.values.eqBandAdjustment[n] = BAND_ADJUSTMENT_MIN;
    }
    else
    {
        mtProject.values.eqBandAdjustment[n] += localValue;
    }

    masterEnhancer.fiveBandsEq.change(mtProject.values.eqBandAdjustment);
    newFileManager.setProjectStructChanged();

    showEqualizerBand(n);
}

void cMasterParams::setDefaultEqualizerBandAdjust(uint8_t n)
{
    mtProject.values.eqBandAdjustment[n] = BAND_ADJUSTMENT_DEFAULT;
    masterEnhancer.fiveBandsEq.change(mtProject.values.eqBandAdjustment);
    newFileManager.setProjectStructChanged();

    showEqualizerBand(n);
}

// Mixer
void cMasterParams::changeTrackVolume(int16_t val, uint8_t track)
{
    if (mtProject.values.trackVolume[track] + val > TRACK_VOLUME_MAX)
        mtProject.values.trackVolume[track] = TRACK_VOLUME_MAX;
    else if (mtProject.values.trackVolume[track] + val < TRACK_VOLUME_MIN)
        mtProject.values.trackVolume[track] = TRACK_VOLUME_MIN;
    else
        mtProject.values.trackVolume[track] += val;

    engine.refreshTrackVolume();
    newFileManager.setProjectStructChanged();
    showTrackVolumeBar(track);
}
void cMasterParams::setDefaultTrackVolume(uint8_t track)
{
    mtProject.values.trackVolume[track] = DEFAULT_TRACK_VOLUME;
    engine.refreshTrackVolume();
    newFileManager.setProjectStructChanged();
    showTrackVolumeBar(track);
}

void cMasterParams::changeReverbVolume(int16_t val)
{
    if (mtProject.values.reverbVolume + val > REVERB_VOLUME_MAX)
        mtProject.values.reverbVolume = REVERB_VOLUME_MAX;
    else if (mtProject.values.reverbVolume + val < REVERB_VOLUME_MIN)
        mtProject.values.reverbVolume = REVERB_VOLUME_MIN;
    else
        mtProject.values.reverbVolume += val;

    newFileManager.setProjectStructChanged();
    showReverbVolumeBar();
    engine.refreshReverbVolume();
}
void cMasterParams::changeDelayVolume(int16_t val)
{
    if (mtProject.values.delayVolume + val > DELAY_VOLUME_MAX)
        mtProject.values.delayVolume = DELAY_VOLUME_MAX;
    else if (mtProject.values.delayVolume + val < DELAY_VOLUME_MIN)
        mtProject.values.delayVolume = DELAY_VOLUME_MIN;
    else
        mtProject.values.delayVolume += val;

    newFileManager.setProjectStructChanged();
    showDelayVolumeBar();
    engine.refreshDelayVolume();
}
void cMasterParams::changeDryMixVolume(int16_t val)
{
    if (mtProject.values.dryMixVolume + val > DRY_MIX_VOLUME_MAX)
        mtProject.values.dryMixVolume = DRY_MIX_VOLUME_MAX;
    else if (mtProject.values.dryMixVolume + val < DRY_MIX_VOLUME_MIN)
        mtProject.values.dryMixVolume = DRY_MIX_VOLUME_MIN;
    else
        mtProject.values.dryMixVolume += val;

    newFileManager.setProjectStructChanged();
    showDryMixVolumeBar();
    engine.refreshTrackVolume();
}

void cMasterParams::changeLineInVolume(int16_t val)
{
    if (mtProject.values.lineInVolume + val > LINE_IN_VOLUME_MAX)
        mtProject.values.lineInVolume = LINE_IN_VOLUME_MAX;
    else if (mtProject.values.lineInVolume + val < LINE_IN_VOLUME_MIN)
        mtProject.values.lineInVolume = LINE_IN_VOLUME_MIN;
    else
        mtProject.values.lineInVolume += val;

    newFileManager.setProjectStructChanged();
    showLineInVolumeBar();
    engine.refreshLineInStreamVolume();
    engine.refreshLineInStreamDelaySend();
    engine.refreshLineInStreamReverbSend();
}

void cMasterParams::changeLineInDelaySend(int16_t val)
{
    mtProject.values.lineInDelaySend =
      clamp<int>(mtProject.values.lineInDelaySend + val, 0, LINE_IN_SEND_MAX);

    newFileManager.setProjectStructChanged();
    showLineInDelaySend();
    engine.refreshLineInStreamDelaySend();
}

void cMasterParams::changeLineInReverbSend(int16_t val)
{
    mtProject.values.lineInReverbSend =
      clamp<int>(mtProject.values.lineInReverbSend + val, 0, LINE_IN_SEND_MAX);

    newFileManager.setProjectStructChanged();
    showLineInReverbSend();
    engine.refreshLineInStreamReverbSend();
}

#ifdef FEATURE_SYNTH
void cMasterParams::changeSynthGain(int synthIdx, int16_t val)
{
    mtProject.values.synthPart[synthIdx].gainDb =
      clamp<float>(mtProject.values.synthPart[synthIdx].gainDb + (val * 0.5),
                   Synth::kGainSilence,
                   Synth::kGainMax);

    engine.setSynthParam(audioEngine::SynthParam::kVolume,
                         synthIdx,
                         mtProject.values.synthPart[synthIdx].gainDb,
                         audioEngine::SynthValuesSource::kInstrument);

    showSynthGain(synthIdx);

    newFileManager.setProjectStructChanged();
}

void cMasterParams::setDefaultSynthGain(int synthIdx)
{
    mtProject.values.synthPart[synthIdx].gainDb =
      clamp<float>(Synth::kGainDefault, Synth::kGainSilence, Synth::kGainMax);

    engine.setSynthParam(audioEngine::SynthParam::kVolume,
                         synthIdx,
                         mtProject.values.synthPart[synthIdx].gainDb,
                         audioEngine::SynthValuesSource::kInstrument);

    showSynthGain(synthIdx);

    newFileManager.setProjectStructChanged();
}
#endif

void cMasterParams::setDefaultReverbVolume()
{
    mtProject.values.reverbVolume = DEFAULT_REVERB_VOLUME;
    newFileManager.setProjectStructChanged();
    showReverbVolumeBar();
    engine.refreshReverbVolume();
}
void cMasterParams::setDefaultDelayVolume()
{
    mtProject.values.delayVolume = DEFAULT_DELAY_VOLUME;
    newFileManager.setProjectStructChanged();
    showDelayVolumeBar();
    engine.refreshDelayVolume();
}
void cMasterParams::setDefaultDryMixVolume()
{
    mtProject.values.dryMixVolume = DEFAULT_DRY_MIX_VOLUME;
    newFileManager.setProjectStructChanged();
    showDryMixVolumeBar();
    engine.refreshTrackVolume();
}

void cMasterParams::setDefaultLineInVolume()
{
    mtProject.values.lineInVolume = DEFAULT_LINE_IN_VOLUME;
    newFileManager.setProjectStructChanged();
    showLineInVolumeBar();
    engine.refreshLineInStreamVolume();
}

void cMasterParams::setDefaultLineInDelaySend()
{
    mtProject.values.lineInDelaySend = 0;
    newFileManager.setProjectStructChanged();
    showLineInDelaySend();
    engine.refreshLineInStreamDelaySend();
}

void cMasterParams::setDefaultLineInReverbSend()
{
    mtProject.values.lineInReverbSend = 0;
    newFileManager.setProjectStructChanged();
    showLineInReverbSend();
    engine.refreshLineInStreamReverbSend();
}

void cMasterParams::setDefaultReverbSize()
{
    mtProject.values.reverb.size = DEFAULT_REVERB_SIZE;
    engine.setReverbSize(mtProject.values.reverb.size);

    showReverbSize();

    newFileManager.setProjectStructChanged();
}
void cMasterParams::setDefaultReverbDamp()
{
    mtProject.values.reverb.damp = DEFAULT_REVERB_DAMP;

    engine.setReverbDamp(mtProject.values.reverb.damp);

    showReverbDamp();

    newFileManager.setProjectStructChanged();
}
void cMasterParams::setDefaultReverbPredelay()
{
    mtProject.values.reverb.predelay = DEFAULT_REVERB_PREDELAY;

    engine.setReverbPredelay(mtProject.values.reverb.predelay);

    showReverbPredelay();

    newFileManager.setProjectStructChanged();
}
void cMasterParams::setDefaultReverbDiffusion()
{
    mtProject.values.reverb.diffusion = DEFAULT_REVERB_DIFFUSION;

    engine.setReverbDiffusion(mtProject.values.reverb.diffusion);

    showReverbDiffusion();

    newFileManager.setProjectStructChanged();
}

void cMasterParams::setDefaultDelayPingPongEnable()
{
    mtProject.values.delayParams &= 0b01111111;
    engine.setDelayParams(mtProject.values.delayParams);
    newFileManager.setProjectStructChanged();
    showDelayPingPongEnable();
}
void cMasterParams::setDefaultDelaySyncEnable()
{
    mtProject.values.delayParams &= 0b10111111;
    engine.setDelayParams(mtProject.values.delayParams);
    newFileManager.setProjectStructChanged();
    showDelaySyncEnable();
}
void cMasterParams::setDefaultDelayRate()
{
    mtProject.values.delayParams &= 0b11000000;
    engine.setDelayParams(mtProject.values.delayParams);
    newFileManager.setProjectStructChanged();
    showDelayRate();
}
void cMasterParams::setDefaultDelayTime()
{
    mtProject.values.delayTime = 500;
    engine.setDelayTime(mtProject.values.delayTime);
    newFileManager.setProjectStructChanged();
    showDelayTime();
}

void cMasterParams::setDefaultDelayFilterFrequency()
{
    mtProject.values.delayFilterFrequency = 20000;
    engine.setDelayFilterFrequency(mtProject.values.delayFilterFrequency);
    newFileManager.setProjectStructChanged();
    showDelayFilterFrequency();
}

void cMasterParams::setDefaultDelayFeedback()
{
    mtProject.values.delayFeedback = 50;
    engine.setDelayFeedback(mtProject.values.delayFeedback);

    newFileManager.setProjectStructChanged();

    showDelayFeedback();
}

void cMasterParams::lightUpPadBoard()
{
    padsBacklight.clearGlobalPads(0, 1, 1);

    bool isBeatSlice =
      mtProject.values.lastUsedInstrument < kSampleInstrumentsCount &&
      mtProject.instrument[mtProject.values.lastUsedInstrument].playMode == playModeBeatSlice;

    if (isBeatSlice)
    {
        for (uint8_t pad = 0;
             pad < mtProject.instrument[mtProject.values.lastUsedInstrument].sliceNumber;
             pad++)
        {
            padsBacklight.setBackLayer(1, mtConfig.values.padsLightBack, pad);
        }
    }
}

static void functPads(uint8_t pad, uint8_t state, int16_t velo)
{
    (void)velo;

    if (MP->keyboardManager.getState())
    {
        MP->keyboardManager.onPadChange(pad, state);
        return;
    }

    bool isBeatSlice =
      mtProject.values.lastUsedInstrument < kSampleInstrumentsCount &&
      mtProject.instrument[mtProject.values.lastUsedInstrument].playMode == playModeBeatSlice;

    if (state == buttonPress)
    {
        // uint8_t note = mtPadBoard.convertPadToNote(pad);
        // if(note > 48) note = 48;
        // editorInstrument->tune = note;

        padsBacklight.setFrontLayer(1, mtConfig.values.padsLightFront, pad);
        uint8_t noteFromPad = mtPadBoard.getNoteFromPad(pad, isBeatSlice);

        sequencer.handleNoteOn(Sequencer::GRID_OUTSIDE_PATTERN, noteFromPad, -1, pad);
    }
    else if (state == buttonRelease)
    {
        padsBacklight.setFrontLayer(0, 0, pad);
        uint8_t noteFromPad = mtPadBoard.getNoteFromPad(pad, isBeatSlice);

        sequencer.handleNoteOff(Sequencer::GRID_OUTSIDE_PATTERN, noteFromPad, 0, pad);
    }
}
// ###############################################################################################
//                            NAZYWANIE TRACKOW

// przelaczanie trybu edycji nazw trackow
static void functEditModeToggle()
{
    if (MP->displayType != MP->displayMode::mixer)
    {
        return;
    }

    MP->editTrackNameMode = !MP->editTrackNameMode;

    MP->showEditTracksNamesMode();
}

void cMasterParams::editTrackName(uint8_t button)
{
    actualEditingTrackName = button;

    MP->setKeyboardExportFunctions();

    MP->keyboardManager.fillName(&mtProject.values.TrackNames[button][0]);
    MP->keyboardManager.activateKeyboard(7);

    MP->showKeyboardExport();
}

// anuluj edycje nazwy tracka
static void functCancelEditTrackName()
{
    MP->setMasterScreenFunct();
    MP->switchToMixer();
    MP->editTrackNameMode = 1;
    MP->showEditTracksNamesMode();

    MP->keyboardManager.deactivateKeyboard();
}

// zapisz nazwe tracka
static void functSaveTrackName()
{
    if (MP->actualEditingTrackName < kSampleTracks)
    {
        strncpy(&mtProject.values.TrackNames[MP->actualEditingTrackName][0],
                MP->keyboardManager.getName(),
                7);
        mtProject.values.TrackNames[MP->actualEditingTrackName][7] = 0;
    }

    newFileManager.setProjectStructChanged();

    MP->setMasterScreenFunct();
    MP->switchToMixer();
    MP->editTrackNameMode = 1;
    MP->showEditTracksNamesMode();

    MP->keyboardManager.deactivateKeyboard();
}

// Zatwierdzenie znaku w klawiaturze
static void functEnterKeyExport()
{
    MP->keyboardManager.confirmKey();
}

// Modyfikacja zaznaczenia znaku w klawiaturze
static void functLeftKeyboard()
{
    MP->keyboardManager.makeMove('a');
}

// Modyfikacja zaznaczenia znaku w klawiaturze
static void functRightKeyboard()
{
    MP->keyboardManager.makeMove('d');
}

// Modyfikacja zaznaczenia znaku w klawiaturze
static void functUpKeyboard()
{
    MP->keyboardManager.makeMove('w');
}

// Modyfikacja zaznaczenia znaku w klawiaturze
static void functDownKeyboard()
{
    MP->keyboardManager.makeMove('s');
}

// Backspace znaku z przycisku delete
static void functDeleteKeyboard()
{
    MP->keyboardManager.makeBackspace();
}

// Zatwierdzenie znaku z przycisku insert
static void functInsertKeyboard()
{
    MP->keyboardManager.confirmKey();
}

uint8_t* const indexedMixerValues[12] = {
    &mtProject.values.trackVolume[0], &mtProject.values.trackVolume[1],
    &mtProject.values.trackVolume[2], &mtProject.values.trackVolume[3],
    &mtProject.values.trackVolume[4], &mtProject.values.trackVolume[5],
    &mtProject.values.trackVolume[6], &mtProject.values.trackVolume[7],
    &mtProject.values.delayVolume,    &mtProject.values.reverbVolume,
    &mtProject.values.dryMixVolume,   &mtProject.values.lineInVolume
};

void changeMixerOnMidiCC(uint8_t cc_idx, uint8_t cc_value)
{
    constexpr uint8_t MIXER_CC_RANGE_MIN = 71;
    constexpr uint8_t MIXER_CC_RANGE_MAX = 82;

    if (cc_idx > MIXER_CC_RANGE_MAX)
        return;
    if (cc_idx < MIXER_CC_RANGE_MIN)
        return;

    uint8_t ccToIdx = cc_idx - MIXER_CC_RANGE_MIN;

    if (ccToIdx < kSampleTracks && ccToIdx >= getProjectActiveSampleTracks())
    {
        return;
    }

    *(indexedMixerValues[ccToIdx]) = ((uint16_t)(cc_value * 100)) / 127;

    if ((ccToIdx < 8) || (ccToIdx == 10))
        engine.refreshTrackVolume();
    else if (ccToIdx == 8)
        engine.refreshDelayVolume();
    else if (ccToIdx == 9)
        engine.refreshReverbVolume();
    else if (ccToIdx == 11)
        engine.refreshLineInStreamVolume();

    if (!mtInterface.isMasterScreenActive())
        return;

    MP->refreshVolumeFromCC[ccToIdx] = true;
}

static void functMenu(uint8_t state)
{
    if (state == buttonPress)
    {
        mtMenuPopup.showModulePopup(MP);
    }
    else if (state == buttonRelease)
    {
        mtMenuPopup.hideModulePopup(MP);
    }
}
