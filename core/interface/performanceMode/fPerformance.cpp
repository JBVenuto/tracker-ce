#include "core/interface.h"
#include "core/interfaceMenuPopup.h"
#include "core/interfacePopups.h"
#include "fileManager.h"
#include "mtConfig.h"
#include "mtPadBoard.h"
#include "mtPadsBacklight.h"
#include "mtSequencer.h"

#include "performanceMode.h"

#include "Platform.h"
#include "performanceEngine.h"
#include "util_math.h"

cPerformanceMode performanceMode;
static cPerformanceMode* PM = &performanceMode;

static void functPlayAction();
static void functRecAction();

static void functLeft();
static void functRight();
static void functUp();
static void functDown();

static void functCopyPaste();

static void functShift(uint8_t value);

static void functEncoder(int16_t value, uint8_t state);

static void functSwitchModule(uint8_t button);
static void functSwitchTracksPage();

static void functActionButton(uint8_t button, uint8_t state);

static void functPads(uint8_t pad, uint8_t state, int16_t velo);

static void functMenu(uint8_t state);
static void functMiniMiddle(uint8_t state);

static bool changeInstantlyTracksOnSelected(int8_t delta);
static bool changeSequentiallyTracksOnSelected(int8_t delta);

void peformanceModeOnMidiCC(uint8_t cc_idx, uint8_t cc_value);

static elapsedMillis refreshTime;
static elapsedMillis midiRefreshTime;

void cPerformanceMode::update()
{
    if (midiRefreshTime < 50)
        return;
    midiRefreshTime = 0;

    refreshAllCC();

    if (refreshTime < 100)
        return;
    refreshTime = 0;

    if (refreshBlinkingTrack == 1)
    {
        blinkTrackUntilSwitch();
    }
    if (refreshTrackState == 1)
    {
        refreshTrackState = 0;
        showTracksState(buttons->isButtonPressed(interfaceButtonShift));
    }
    if (refreshTrackPattern == 1)
    {
        refreshTrackPattern = 0;
        showTracksPatterns();
    }
}

void cPerformanceMode::start(uint32_t options)
{
    // ustawienie trackow
    if (!dontClearPerformanceMode)
    {
        performance.resetSelectedRows();
        tracksPage = 0;

        for (uint8_t q = 0; q < kPerformanceTracks; q++)
        {
            mtProject.perfTracksPatterns[q] = mtProject.values.actualPattern;
        }
    }

    if (options == 1)
    {
        tracksPage = 0;
    }
    else if (options == 2)
    {
        tracksPage = 1;
    }

    if (getProjectMaxTracks() <= kSampleTracks)
    {
        tracksPage = 0;
    }

    // zaladowanie danych do performencu
    if (mtConfig.general.performanceSource < performanceDataSourceCount)
    {
        performance.readPerfData(mtConfig.general.performanceSource);
    }

    // czyszczenie afektowania traków
    for (uint8_t track = 0; track < kPerformanceTracks; track++)
    {
        if (!dontClearPerformanceMode)
            performance.setTrackState(track, 0); // ustaw na 0
    }

    // prezepisanie aktualnie zaznaczonych wartosci do tablicy wartosci chwilowych
    performance.loadSelectedValuesToTempValues();

    if (dontClearPerformanceMode)
    {
        refreshTrackPattern = 1;
        refreshTrackState = 1;
    }

    moduleRefresh = 1;
    performanceEditState = 0;
    performance.setEditState(performanceEditState);
    refreshBlinkingTrack = 0;
    dontClearPerformanceMode = 0;

    // ustawienie funkcji
    ip->setButtonObj(interfaceButtonPerformance, buttonPress, functSwitchTracksPage);
    ip->setButtonObj(interfaceButtonParams, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonFile, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonConfig, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonMaster, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSamplePlay, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSampleEdit, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSampleRec, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSampleLoad, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSong, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonPattern, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonGame, buttonPress, functSwitchModule);

    ip->setButtonObj(interfaceButtonMiniMenu, functMenu);
    ip->setButtonObj(interfaceButtonMiniMiddle, functMiniMiddle);

    setDefaultScreenFunct();

    PM->showPerformanceFxes();
    PM->setPerformanceFxes();

    sequencer.enterPerformanceMode();
}

void cPerformanceMode::stop()
{
    if (!dontClearPerformanceMode)
    {
        if (performance.turnOff())
        {
            newFileManager.loadWorkspacePatternNow(mtProject.values.actualPattern);
            sequencer.switchRamPatternsNow();
        }
        mtPadBoard.releaseAllInstruments();
    }

    performance.forceSaveConfig();

    moduleRefresh = 0;
    padsBacklight.clearGlobalPads(1, 1, 1);
}

void cPerformanceMode::setDefaultScreenFunct()
{
    // funkcje
    ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    ip->clearAllPots();

    ip->setButtonObj(interfaceButtonPlay, buttonPress, functPlayAction);
    ip->setButtonObj(interfaceButtonRec, buttonPress, functRecAction);

    ip->setButtonObj(interfaceButtonShift, functShift);

    ip->setButtonObj(interfaceButtonCopy, buttonPress, functCopyPaste);

    ip->setButtonObj(interfaceButton0, functActionButton);
    ip->setButtonObj(interfaceButton1, functActionButton);
    ip->setButtonObj(interfaceButton2, functActionButton);
    ip->setButtonObj(interfaceButton3, functActionButton);
    ip->setButtonObj(interfaceButton4, functActionButton);
    ip->setButtonObj(interfaceButton5, functActionButton);
    ip->setButtonObj(interfaceButton6, functActionButton);
    ip->setButtonObj(interfaceButton7, functActionButton);

    ip->setPadsGlobal(functPads);
}

//==============================================================================================================
void cPerformanceMode::setPerformanceFxes()
{
    ip->setPotObj(interfacePot0, functEncoder);

    ip->setButtonObj(interfaceButtonLeft, buttonPress, functLeft);
    ip->setButtonObj(interfaceButtonRight, buttonPress, functRight);
    ip->setButtonObj(interfaceButtonUp, buttonPress, functUp);
    ip->setButtonObj(interfaceButtonDown, buttonPress, functDown);

    lightUpPadBoard();
}

void cPerformanceMode::toggleEditState()
{
    performanceEditState = !performanceEditState;

    performance.setEditState(performanceEditState);

    if (performanceEditState)
    {
        showEditFrame(performanceEditPlace);
    }
    else
    {
        hideEditFrame(performanceEditPlace);
    }

    showAllPerformanceValues();
}

uint8_t cPerformanceMode::wasPatternChangedOnTrack(uint8_t track)
{
    return trackPatternChange[track] > 1;
}

void cPerformanceMode::setProjectSaveFlags()
{
    newFileManager.setProjectStructChanged();
    //	mtProject.values.projectNotSavedFlag = 1;
    //	fileManager.projectChangeFlag = 1;
}

bool cPerformanceMode::turnOff()
{
    return performance.turnOff();
}

void cPerformanceMode::onMaxTracksChanged(uint8_t maxTracks)
{
    if (maxTracks <= kSampleTracks)
    {
        tracksPage = 0;
    }

    for (uint8_t tr = maxTracks; tr < kPerformanceTracks; tr++)
    {
        trackPatternChange[tr] = 0;
        isCCToReceiveForTrack[tr] = false;
    }
}

void cPerformanceMode::turnOffAtEndOfPattern()
{
    if (!sequencer.isPerformanceMode())
        return;

    if (!sequencer.isPlay())
    {
        // No pattern end is coming that could do it.
        turnOff();
        dontClearPerformanceMode = 0;
        return;
    }

    dontClearPerformanceMode = 1;
    if (!sequencer.isSongMode())
    {
        // force pattern reload to clear changed tracks
        sequencer.sequencialSwitch_changeNextPattern(0);
        sequencer.sequencialSwitch_SetReady();
    }
    sequencer.setOnPatternOrSelectionEnd(cbkTurnOffPerformanceMode);
}

bool cPerformanceMode::canPerformanceModeContinueAfterSwitchingTo(uint8_t button)
{
    return (button == interfaceButtonPerformance || button == interfaceButtonMaster ||
            button == interfaceButtonParams);
}

void cPerformanceMode::setFxValue(int value)
{
    for (uint8_t place = 0; place < 12; place++)
    {
        performance.changeSlotFxValue(place, value);
        if (performance.slotChanged(place))
            PM->showPerformaceValue(place);
    }
}

//=================================================================================
//
//=================================================================================

//=================================================================================
//
//=================================================================================
uint8_t cPerformanceMode::fxAlredyTaken(uint8_t fx)
{
    for (uint8_t i = 0; i < 12; i++)
    {
        if (mtProject.values.perfFxPlaces[i] == fx)
            return 1;
    }

    return 0;
}

void cPerformanceMode::refreshAllCC()
{
    for (uint8_t ccForSlot = 0; ccForSlot < 12; ccForSlot++)
    {
        if (isCCToReceiveForSlot[ccForSlot])
        {
            performance.setSelectedRow(ccForSlot, 4);
            performance.setCCValue(currentCCValueForSlot[ccForSlot], ccForSlot);
            performance.loadCCValuesOnAffectedTracks(ccForSlot);
            showPerformaceValue(ccForSlot);
            lightUpPadBoard();

            isCCToReceiveForSlot[ccForSlot] = false;
        }
    }

    const uint8_t maxTracks = getProjectMaxTracks();
    for (uint8_t ccForTrack = 0; ccForTrack < kPerformanceTracks; ccForTrack++)
    {
        if (isCCToReceiveForTrack[ccForTrack])
        {
            if (ccForTrack < maxTracks)
            {
                trackPatternChange[ccForTrack] = 0;
                mtProject.perfTracksPatterns[ccForTrack] = currentCCValueForTrack[ccForTrack] + 1;
                setProjectSaveFlags();

                sequencer.setTrackToLoadOnSwitch(ccForTrack,
                                                 mtProject.perfTracksPatterns[ccForTrack]);

                PM->refreshBlinkingTrack = 1;
                refreshTrackPattern = 1;
            }

            isCCToReceiveForTrack[ccForTrack] = false;
        }
    }
    display.synchronizeRefresh();
}

//=================================================================================
//
//=================================================================================
static void functEncoder(int16_t value, uint8_t state)
{
    if (changeInstantlyTracksOnSelected(value))
    {
        return;
    }

    (void)state;

#ifdef TRACKER_MINI
    int row = 0;
    switch (value)
    {
        case 10: row = 0; break;
        case 1: row = 1; break;
        case -1: row = 2; break;
        case -10: row = 3; break;
        default: row = 0; break;
    }
    functPads(PM->performanceEditPlace + row * 12, state, 0);
    return;
#endif

    PM->setFxValue(value);
}

static bool changeInstantlyTracksOnSelected(int8_t delta)
{
    bool atLeastOneChanged = false;
    for (uint8_t button = 0; button < 8; button++)
    {
        if (!PM->isButtonActiveOnCurrentPage(button))
        {
            continue;
        }

        if (PM->buttons->isButtonPressed(button))
        {
            uint8_t track = PM->getTrackForButton(button);
            PM->trackPatternChange[track] = 0;

            mtProject.perfTracksPatterns[track] =
              clamp<int>((int16_t)(mtProject.perfTracksPatterns[track] + delta),
                         PATTERN_INDEX_MIN,
                         PATTERN_INDEX_MAX);

            sequencer.setTrackToLoadNow(track, mtProject.perfTracksPatterns[track]);
            atLeastOneChanged = true;
        }
    }
    PM->refreshTrackPattern = 1;

    return atLeastOneChanged;
}

static bool changeSequentiallyTracksOnSelected(int8_t delta)
{
    bool trackChanging = false;

    for (uint8_t button = 0; button < 8; button++)
    {
        if (!PM->isButtonActiveOnCurrentPage(button))
        {
            continue;
        }

        uint8_t track = PM->getTrackForButton(button);
        if ((PM->trackPatternChange[track] == 1 || PM->trackPatternChange[track] == 2) &&
            PM->buttons->isButtonPressed(button))
        {
            trackChanging = true;
            PM->trackPatternChange[track] = 2;

            mtProject.perfTracksPatterns[track] =
              clamp<int>((int16_t)(mtProject.perfTracksPatterns[track] + delta),
                         PATTERN_INDEX_MIN,
                         PATTERN_INDEX_MAX);
            PM->setProjectSaveFlags();

            PM->refreshTrackPattern = 1;
        }
    }

    return trackChanging;
}

//=========================================================================================================
static void functLeft()
{
#ifdef TRACKER_MINI
    if (PM->buttons->isButtonPressed(interfaceButtonMiniPlus10) ||
        PM->buttons->isButtonPressed(interfaceButtonMiniPlus1) ||
        PM->buttons->isButtonPressed(interfaceButtonMiniMinus1) ||
        PM->buttons->isButtonPressed(interfaceButtonMiniMinus10))
    {
        return;
    }
#endif

    if (PM->performanceEditState && PM->buttons->isButtonPressed(interfaceButtonShift) &&
        PM->performanceEditPlace > 0)
    {
        performance.swapSlotFx(PM->performanceEditPlace, PM->performanceEditPlace - 1);
        PM->performanceEditPlace--;

        PM->showEditFrame(PM->performanceEditPlace);
        PM->showFxNames(PM->performanceEditPlace);
        PM->showFxNames(PM->performanceEditPlace + 1);
        PM->showPerformaceValue(PM->performanceEditPlace);
        PM->showPerformaceValue(PM->performanceEditPlace + 1);
        PM->lightUpPadBoard();
    }
    else if (PM->performanceEditState)
    {
        if (PM->performanceEditPlace > 0)
        {
            PM->performanceEditPlace--;
        }
        PM->showEditFrame(PM->performanceEditPlace);
    }
    else
    {
        if (!changeSequentiallyTracksOnSelected(-1))
        {
            if (PM->performanceEditPlace > 0)
            {
                PM->performanceEditPlace--;
            }
            PM->hideEditFrame(PM->performanceEditPlace);
        }
    }
}

static void functRight()
{
#ifdef TRACKER_MINI
    if (PM->buttons->isButtonPressed(interfaceButtonMiniPlus10) ||
        PM->buttons->isButtonPressed(interfaceButtonMiniPlus1) ||
        PM->buttons->isButtonPressed(interfaceButtonMiniMinus1) ||
        PM->buttons->isButtonPressed(interfaceButtonMiniMinus10))
    {
        return;
    }
#endif

    if (PM->performanceEditState && PM->buttons->isButtonPressed(interfaceButtonShift) &&
        PM->performanceEditPlace < 11)
    {
        performance.swapSlotFx(PM->performanceEditPlace, PM->performanceEditPlace + 1);
        PM->performanceEditPlace++;

        PM->showEditFrame(PM->performanceEditPlace);
        PM->showFxNames(PM->performanceEditPlace);
        PM->showFxNames(PM->performanceEditPlace - 1);
        PM->showPerformaceValue(PM->performanceEditPlace);
        PM->showPerformaceValue(PM->performanceEditPlace - 1);
        PM->lightUpPadBoard();
    }
    else if (PM->performanceEditState)
    {
        if (PM->performanceEditPlace < 11)
        {
            PM->performanceEditPlace++;
        }
        PM->showEditFrame(PM->performanceEditPlace);
    }
    else
    {
        if (!changeSequentiallyTracksOnSelected(1))
        {
            if (PM->performanceEditPlace < 11)
            {
                PM->performanceEditPlace++;
            }
            PM->hideEditFrame(PM->performanceEditPlace);
        }
    }
}

static void functUp()
{
    if (changeInstantlyTracksOnSelected(1))
    {
        return;
    }

#ifdef TRACKER_MINI
    if (PM->buttons->isButtonPressed(interfaceButtonMiniPlus10) ||
        PM->buttons->isButtonPressed(interfaceButtonMiniPlus1) ||
        PM->buttons->isButtonPressed(interfaceButtonMiniMinus1) ||
        PM->buttons->isButtonPressed(interfaceButtonMiniMinus10))
    {
        PM->setFxValue(1);
        return;
    }
#endif

    if (PM->performanceEditState)
    {
        performance.changeSlotFx(PM->performanceEditPlace, 1);

        PM->showFxNames(PM->performanceEditPlace);
        PM->showPerformaceValue(PM->performanceEditPlace);
        PM->lightUpPadBoard();
    }
}

static void functDown()
{
    if (changeInstantlyTracksOnSelected(-1))
    {
        return;
    }

#ifdef TRACKER_MINI
    if (PM->buttons->isButtonPressed(interfaceButtonMiniPlus10) ||
        PM->buttons->isButtonPressed(interfaceButtonMiniPlus1) ||
        PM->buttons->isButtonPressed(interfaceButtonMiniMinus1) ||
        PM->buttons->isButtonPressed(interfaceButtonMiniMinus10))
    {
        PM->setFxValue(-1);
        return;
    }
#endif

    if (PM->performanceEditState)
    {
        performance.changeSlotFx(PM->performanceEditPlace, -1);

        PM->showFxNames(PM->performanceEditPlace);
        PM->showPerformaceValue(PM->performanceEditPlace);
        PM->lightUpPadBoard();
    }
}

//=========================================================================================================
static void functPlayAction()
{
    if (sequencer.getSeqState() == Sequencer::SEQ_STATE_STOP)
    {
        if (!PM->buttons->isButtonPressed(interfaceButtonShift))
        {
            sequencer.playPattern();
        }
        else
        {
            sequencer.playSong();
            for (uint8_t a = 0; a < kPerformanceTracks; a++)
            {
                mtProject.perfTracksPatterns[a] = mtProject.values.actualPattern;
            }
            PM->refreshTrackPattern = 1;
        }
    }
    else
    {
        if (PM->buttons->isButtonPressed(interfaceButtonShift))
        {
            sequencer.setOnGlobalStep(cbkPerformancePlaybackModeToggle);
        }
        else
        {
            sequencer.stop();
        }
    }
}

static void functRecAction()
{
    PM->toggleEditState();
}

static void functShift(uint8_t value)
{
    if (value == buttonPress)
    {

        PM->showMuteButtons(1);
    }
    else if (value == buttonRelease)
    {
        PM->showMuteButtons(0);
    }
}

static void functCopyPaste()
{

    if (PM->performanceEditState)
    {
        if (PM->buttons->isButtonPressed(interfaceButtonShift))
        {
            performance.pasteSlotFx(PM->performanceEditPlace);
        }
        else
        {
            performance.copySlotFx(PM->performanceEditPlace);
        }
    }

    PM->showFxNames(PM->performanceEditPlace);
    PM->showPerformaceValue(PM->performanceEditPlace);
    PM->lightUpPadBoard();
}

static void functSwitchModule(uint8_t button)
{
    if (sequencer.isPerformanceMode())
    {
        if (PM->canPerformanceModeContinueAfterSwitchingTo(button))
        {
            PM->dontClearPerformanceMode = 1;
        }
        else if (sequencer.isPlay())
        {
            PM->turnOffAtEndOfPattern();
        }
        else
        {
            PM->turnOff();
        }
    }

    PM->eventFunct(eventSwitchModule, PM, &button, 0);
}

static void functSwitchTracksPage()
{
    if (getProjectMaxTracks() <= kSampleTracks)
    {
        return;
    }

    for (uint8_t i = 0; i < kPerformanceTracks; i++)
    {
        PM->trackPatternChange[i] = 0;
    }

    PM->tracksPage = !PM->tracksPage;

    if (PM->buttons->isButtonPressed(interfaceButtonShift))
    {
        PM->showMuteButtons(1);
    }
    else
    {
        PM->showTracksPatterns();
        PM->showTracksState();
        if (PM->refreshBlinkingTrack == 1)
        {
            PM->blinkTrackUntilSwitch();
        }
    }
}

//##############################################################################################
//###############################        ACTION BUTTONS        #################################
//##############################################################################################

static void functActionButton(uint8_t button, uint8_t state)
{
    if (!PM->isButtonActiveOnCurrentPage(button))
    {
        return;
    }

    uint8_t track = PM->getTrackForButton(button);

    if (state == buttonPress)
    {
        if (PM->buttons->isButtonPressed(interfaceButtonShift))
        {
            if (isSampleTrack(track))
            {
                mtProject.values.trackMute[track] = !mtProject.values.trackMute[track];
                engine.muteTrack(track, mtProject.values.trackMute[track]);
            }
            else
            {
                uint8_t midiIdx = track - kSampleTracks;
                mtProject.values.midiTrackMute[midiIdx] = !mtProject.values.midiTrackMute[midiIdx];
            }

            PM->refreshTrackState = 1;
            PM->setProjectSaveFlags();
        }
        else
        {
            PM->trackPatternChange[track] = 1;
        }
    }
    else if (state == buttonRelease)
    {
        if (!PM->buttons->isButtonPressed(interfaceButtonShift))
        {
            if (PM->wasPatternChangedOnTrack(track))
            {
                sequencer.setTrackToLoadOnSwitch(track, mtProject.perfTracksPatterns[track]);
                PM->refreshBlinkingTrack = 1;
            }
            else if (PM->trackPatternChange[track] == 0)
            {
                // do nothing
                // tu sie trafia jak zmienia sie track w trybie natychmiastowym
            }
            else
            {
                performance.toggleTrackState(track);

                PM->refreshTrackState = 1;
            }
        }

        PM->trackPatternChange[track] = 0;
    }
}

void cPerformanceMode::blinkTrackUntilSwitch()
{
    blinkInterval++;
    if (blinkInterval >= 4)
    {
        blinkState = !blinkState;
        blinkInterval = 0;
    }

    uint8_t continueBlink = 0;

    if (PM->buttons->isButtonPressed(interfaceButtonShift))
    {
        continueBlink = 1;
    }

    for (uint8_t track = 0; track < kPerformanceTracks; track++)
    {
        if (sequencer.isPerformanceTrackChange(track))
        {
            continueBlink = 1;
            break;
        }
    }

    for (uint8_t button = 0; button < 8; button++)
    {
        if (!isButtonActiveOnCurrentPage(button))
        {
            continue;
        }

        uint8_t track = getTrackForButton(button);
        if (PM->buttons->isButtonPressed(interfaceButtonShift))
        {
            colorTracksLabel(button, 0);
        }
        else if (sequencer.isPerformanceTrackChange(track))
        {
            colorTracksLabel(button, blinkState);
        }
        else
        {
            colorTracksLabel(button, 0);
        }
    }

    if (!continueBlink)
    {
        refreshTrackState = 1;

        refreshTrackPattern = 1;

        refreshBlinkingTrack = 0;
    }

    display.synchronizeRefresh();
}

//##############################################################################################
//###############################          PAD BOARD           #################################
//##############################################################################################

void cPerformanceMode::lightUpPadBoard()
{
    // padsBacklight.clearGlobalPads(0, 1, 1);
    uint8_t slot_state[4];

    for (uint8_t slot = 0; slot < 12; slot++)
    {
        slot_state[0] = 0;
        slot_state[1] = 0;
        slot_state[2] = 0;
        slot_state[3] = 0;

        int8_t row = performance.getSelectedRow(slot);

        if (row >= 0 && row < 4)
            slot_state[row] = 1; // ponizej 0 i powyzej 3 nie zaswieci zadnego pada

        for (uint8_t r = 0; r < 4; r++)
        {
            if (slot_state[r] == 0)
                padsBacklight.setBackLayer(0, 0, slot + (12 * r));
            else
                padsBacklight.setBackLayer(1, mtConfig.values.padsLightBack, slot + (12 * r));
        }
    }
}

static void functPads(uint8_t pad, uint8_t state, int16_t velo)
{
    if (state > 1)
    {
        return;
    }

    uint8_t slot = pad % 12;

    if (state == buttonPress)
    {
        padsBacklight.setFrontLayer(1, mtConfig.values.padsLightFront, pad);

        performance.clearCC(slot);
        performance.setSelectedRow(slot, pad / 12);
        performance.setSlotState(slot, 1);

        performance.loadSlotFxValues(slot);
    }
    else if (state == buttonRelease)
    {
        // jesli puszczany ostatnio wcisniety pad w danym slocie
        // to przeladuj wartosc
        performance.loadSlotFxValues(slot);

        performance.setSlotState(slot, 0);

        padsBacklight.setFrontLayer(0, 0, pad);
    }

    // odwiez zmienione sloty
    for (uint8_t place = 0; place < 12; place++)
    {
        if (performance.slotChanged(place))
        {
            PM->showPerformaceValue(place);
        }
    }

    PM->lightUpPadBoard();
}
// constexpr uint32_t CC_REFRESH_TIME = 25000;

// elapsedMicros midiCCReceiveTimer[12];

void peformanceModeOnMidiCC(uint8_t cc_idx, uint8_t cc_value)
{
    constexpr uint8_t PERFORMANCE_MODE_CC_FOR_SLOT_RANGE_MIN = 51;
    constexpr uint8_t PERFORMANCE_MODE_CC_FOR_SLOT_RANGE_MAX = 62;

    constexpr uint8_t PERFORMANCE_MODE_CC_FOR_TRACK_RANGE_MIN = 41;
    constexpr uint8_t PERFORMANCE_MODE_CC_FOR_TRACK_RANGE_MAX = 48;

    if ((cc_idx <= PERFORMANCE_MODE_CC_FOR_SLOT_RANGE_MAX) &&
        (cc_idx >= PERFORMANCE_MODE_CC_FOR_SLOT_RANGE_MIN))
    {
        uint8_t ccToSlot = cc_idx - PERFORMANCE_MODE_CC_FOR_SLOT_RANGE_MIN;

        PM->isCCToReceiveForSlot[ccToSlot] = true;
        PM->currentCCValueForSlot[ccToSlot] = cc_value;
    }
    else if ((cc_idx <= PERFORMANCE_MODE_CC_FOR_TRACK_RANGE_MAX) &&
             (cc_idx >= PERFORMANCE_MODE_CC_FOR_TRACK_RANGE_MIN))
    {
        uint8_t ccToTrack = cc_idx - PERFORMANCE_MODE_CC_FOR_TRACK_RANGE_MIN;

        if (ccToTrack < getProjectMaxTracks())
        {
            PM->isCCToReceiveForTrack[ccToTrack] = true;
            PM->currentCCValueForTrack[ccToTrack] = cc_value;
        }
    }
}

static void functMenu(uint8_t state)
{
    if (state == buttonPress)
    {
        PM->dontClearPerformanceMode = 1;
        PM->hideFxNames();
        mtMenuPopup.showModulePopup(PM);
    }
    else if (state == buttonRelease)
    {
        mtMenuPopup.hideModulePopup(PM);
    }
}

static void functMiniMiddle(uint8_t state)
{
}

AUDIO_CORE_RAM void cbkTurnOffPerformanceMode(void)
{
    performanceMode.turnOff();
    performanceMode.dontClearPerformanceMode = 0;
    sequencer.setOnPatternOrSelectionEnd(nullCallback);
}

/* Executed from the sequencer interrupt. */
void cbkPerformancePlaybackModeToggle()
{
    uint8_t selectedSlot = mtProject.song.playlistPos;

    if (newFileManager.isPatternLoading() || sequencer.isLastPatternStep())
    {
        return; // retry in one step (because we're not unsetting)
    }

    if (sequencer.isSongMode())
    {
        // już ktoś wcisnął "Continue Song" i teraz zmienił zdanie
        sequencer.loopCurrentPattern(selectedSlot);
        sequencer.setOnPatternOrSelectionEnd(nullCallback);
    }
    else
    {
        sequencer.continueSong(selectedSlot);
        // performanceMode.turnOffAtEndOfPattern();  //commented to fix: #1385
        // Performance Mode - switch from loop
        // mode to song mode mutes effects
        sequencer.setOnPatternOrSelectionEnd(nullCallback);
    }

    sequencer.setOnGlobalStep(nullCallback);
}
