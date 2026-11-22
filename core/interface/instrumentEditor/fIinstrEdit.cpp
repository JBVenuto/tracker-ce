#include "instrumentEditor/instrumentEditor.h"
#include "mtWavetableEnvelopes.h"

#include "core/interfaceMenuPopup.h"
#include "core/interfacePopups.h"
#include "mtAudioEngine.h"
#include "mtPadBoard.h"
#include "mtPadsBacklight.h"

#include "Platform.h"
#include "ampLogarythmicValues.h"
#include "config.h"
#include "core/interface.h"
#include "cr_section_macros.h"
#include "fileBrowser/GlobalFileBrowser.h"
#include "fileManager.h"
#include "performanceMode/performanceMode.h"
#include "util_math.h"
#include "util_string.h"

#include "log.h"
#include <iterator>

cInstrumentEditor instrumentEditor;
static cInstrumentEditor* IE = &instrumentEditor;

static void functPlayAction();
static void functDelete(uint8_t state);
static void functShift(uint8_t state);

static void functInstrument(uint8_t state);

static void functEncoder(int16_t value);
static void functUp();
static void functDown();

static void functLeft();
static void functRight();

static void functMiniMiddle(uint8_t state);

static void functPads(uint8_t pad, uint8_t state, int16_t velo);

static void functMenu(uint8_t state);

static void functSwitchModule(uint8_t button);
static void functSwitchMode();

static void functSelectParams(uint8_t button, uint8_t state);

static void functInstrSynthScreenButtons(uint8_t button, uint8_t state);

static void functSwitchToSends(uint8_t button, uint8_t state);

static void functNote(uint8_t value);

static void functEffectSelectReverb();
static void functEffectSelectDelay();
static void functEffectSelectOverdrive();
static void functEffectSelectBitDepth();
static void functOpenInstrumentSynthesizer();
static void functEffectSavePti();
static void functEffectBack();

void changeParamsVelocity(int16_t value);
void changeParamsOffset(int16_t value);
void changeParamsCC(int8_t index, int16_t value);
void changeParamsCCA(int16_t value);
void changeParamsCCB(int16_t value);
void changeParamsCCC(int16_t value);
void changeParamsCCD(int16_t value);
void changeParamsCCE(int16_t value);
void changeParamsCCF(int16_t value);

static void onNoteKeyboardHide();

void cInstrumentEditor::update()
{
    moduleRefresh = 0;
}

void cInstrumentEditor::start(uint32_t options)
{
    moduleRefresh = 1;
#if GUI_HANDLE_SYNTH
    const uint8_t prevMode = mode;
#endif

    if (mtProject.values.lastUsedInstrument >= SYNTH_INSTRUMENTS_START_IDX + kSynthParts)
    {
        mtProject.values.lastUsedInstrument = 0;
    }

    bool openEffectsWindow = false;

    // adjust mode when instrument is other than current mode expects
    switch (options)
    {
        case 1:
        {
#if GUI_HANDLE_SYNTH
            if (mtProject.values.lastUsedInstrument >= SYNTH_INSTRUMENTS_START_IDX)
            {
                mode = mtInstEditModeSynth;
            }
            else
#endif
              if (mtProject.values.lastUsedInstrument >= MIDI_INSTRUMENTS_START_IDX)
            {
                mode = mtInstEditModeMidi;
            }
            else
            {
                mode = mtInstEditModeParams;
            }
            break;
        }
        case 2:
        {
#if GUI_HANDLE_SYNTH
            if (mtProject.values.lastUsedInstrument >= SYNTH_INSTRUMENTS_START_IDX)
            {
                mode = mtInstEditModeSynthMacros;
            }
            else
#endif
            {
                if (mtProject.values.lastUsedInstrument >= kSampleInstrumentsCount)
                {
                    mtProject.values.lastUsedInstrument = 0;
                }
                mode = mtInstEditModeEnv;
            }
            break;
        }
        case 3:
        {
            if (mtProject.values.lastUsedInstrument >= kSampleInstrumentsCount)
            {
                mtProject.values.lastUsedInstrument = 0;
            }
            mode = mtInstEditModeInstrSynth;
            break;
        }
        case 4:
        {
            if (isSynthInstrument(mtProject.values.lastUsedInstrument))
            {
            }
            else if (isMidiInstrument(mtProject.values.lastUsedInstrument))
            {
                mode = mtInstEditModeMidi;
            }
            else if (isSampleInstrument(mtProject.values.lastUsedInstrument))
            {
                mode = mtInstEditModeParams;
            }
            else
            {
                assert(false);
            }
            break;
        }
        case 5:
        {
            if (isSampleInstrument(mtProject.values.lastUsedInstrument))
            {
                mode = mtInstEditModeParams;
                openEffectsWindow = true;
            }
            break;
        }
        default:
        {
#if GUI_HANDLE_SYNTH
            if (mtProject.values.lastUsedInstrument >= SYNTH_INSTRUMENTS_START_IDX)
            {
                if (mode != mtInstEditModeSynth && mode != mtInstEditModeSynthMacros &&
                    mode != mtInstEditModeSynthPatchEdit)
                {
                    mode = mtInstEditModeSynth;
                }
            }
            else
#endif
              if (mtProject.values.lastUsedInstrument >= MIDI_INSTRUMENTS_START_IDX)
            {
                mode = mtInstEditModeMidi;
            }
            else if (mode != mtInstEditModeParams && mode != mtInstEditModeEnv)
            {
                mode = mtInstEditModeParams;
            }

            break;
        }
    }

    if (mtProject.values.lastUsedInstrument < kSampleInstrumentsCount)
    {
        editorInstrument = &mtProject.instrument[mtProject.values.lastUsedInstrument];
    }
    else
    {
        editorInstrument = nullptr;
    }

    isEffectsWindow = false;

    keyboardManager.deactivateKeyboard();

    clearAllNodes();
    cancelMultiFrame();
    lightUpPadBoard();

    setBaseScreenFunct();

#if GUI_HANDLE_SYNTH
    if (prevMode == mtInstEditModeSynthPatchEdit && mode != mtInstEditModeSynthPatchEdit)
    {
        cleanPatchEditorResources();
    }
#endif

    switch (mode)
    {
        case mtInstEditModeEnv:
        {
            if ((!IE->editorInstrument->isAutomationDestEnabled(IE->selectedEnvelope)) &&
                (IE->selectedPlace[IE->mode] > 1))
            {
                IE->selectedPlace[IE->mode] = 1;
            }

            showInstrumentEnv();
            setDefaultScreenFunct();
            break;
        }
        case mtInstEditModeMidi:
        {
            showInstrumentMidiParams();
            setDefaultScreenFunct();
            break;
        }
        case mtInstEditModeInstrSynth:
        {
            if (mtProject.values.lastUsedInstrument >= kSampleInstrumentsCount)
            {
                if (instListPos >= kSampleInstrumentsCount)
                {
                    instListPos = 0;
                }
                mtProject.values.lastUsedInstrument = instListPos;
            }

            editorInstrument = &mtProject.instrument[mtProject.values.lastUsedInstrument];

            showInstrumentSynth();
            setInstrumentSynthScreenFunct();
            break;
        }
#if GUI_HANDLE_SYNTH
        case mtInstEditModeSynth:
        {
            showSynthParamsScreen();
            setSynthParamsScreenFunct();
            break;
        }
        case mtInstEditModeSynthMacros:
        {
            showSynthMacrosScreen();
            setSynthMacrosScreenFunct();
            break;
        }
        case mtInstEditModeSynthPatchEdit:
        {
            showSynthPatchEditorScreen();
            setSynthPatchEditorScreenFunct();
            break;
        }
#endif
        default:
        {
            showInstrumentParams();
            if (openEffectsWindow)
            {
                switchToEffectScreen();
            }
            else
            {
                setDefaultScreenFunct();
            }
            break;
        }
    }

    if (mtPopups.isNoteKeyboardPopupActive())
    {
        mtPopups.toggleNoteKeyboardPopup(InterfacePopups::noteKeyboardOption::kForceShow);
    }
}

void cInstrumentEditor::stop()
{
    mtPopups.hideInfoPopup();
    keyboardManager.deactivateKeyboard();
    sequencer.stopManualNotes();

    if (mtPopups.isNoteKeyboardPopupActive())
    {
        mtPopups.toggleNoteKeyboardPopup();
    }

    moduleRefresh = 0;
#if GUI_HANDLE_SYNTH
    // The "Init patch" popup blocks all inputs but its two buttons, and only those buttons
    // unblock them. The module can be closed under the popup, e.g. when the card is removed.
    ip->unblockAllInputs();
    exitSynthEditor();
    cleanPatchEditorResources();
    cleanMacrosResources();
#endif
    mtPadBoard.releaseInstrumentsTakenByPads();
    padsBacklight.clearGlobalPads(1, 1, 1);
}

void cInstrumentEditor::hideControlsUnderMiniMenu()
{
    hideControlUnderMenu();
}

uint8_t cInstrumentEditor::getCurrentModeIdx()
{
#if GUI_HANDLE_SYNTH
    return (mode == mtInstEditModeEnv || mode == mtInstEditModeSynthMacros) ? 1 : 0;
#else
    return (mode == mtInstEditModeEnv) ? 1 : 0;
#endif
}

void cInstrumentEditor::setBaseScreenFunct()
{
    ip->setButtonObj(interfaceButtonParams, buttonPress, functSwitchMode);

    ip->setButtonObj(interfaceButtonPerformance, buttonPress, functSwitchModule);
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

    setPlayButtonAction();

    ip->setButtonObj(interfaceButtonNote, functNote);
    ip->setButtonObj(interfaceButtonInstr, functInstrument);
}

void cInstrumentEditor::setDefaultScreenFunct()
{
    ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    ip->clearAllPots();

    for (uint8_t i = interfaceButton0; i < interfaceButton7; i++)
    {
        ip->setButtonObj(i, functSelectParams);
    }
    ip->setButtonObj(interfaceButton7, functSwitchToSends);

    ip->setPotObj(interfacePot0, functEncoder);

    ip->setButtonObj(interfaceButtonLeft, buttonPress, functLeft);
    ip->setButtonObj(interfaceButtonRight, buttonPress, functRight);
    ip->setButtonObj(interfaceButtonUp, buttonPress, functUp);
    ip->setButtonObj(interfaceButtonDown, buttonPress, functDown);

    ip->setButtonObj(interfaceButtonDelete, functDelete);
    ip->clearButton(interfaceButtonInsert);

    ip->setButtonObj(interfaceButtonShift, functShift);

    ip->setButtonObj(interfaceButtonParams, buttonPress, functSwitchMode);
    setPlayButtonAction();

    ip->setButtonObj(interfaceButtonMiniMiddle, functMiniMiddle);
    ip->setPadsGlobal(functPads);
}

void cInstrumentEditor::setInstrumentSynthScreenFunct()
{
    ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    ip->clearAllPots();

    for (uint8_t i = interfaceButton0; i <= interfaceButton7; i++)
    {
        ip->setButtonObj(i, functInstrSynthScreenButtons);
    }

    ip->setButtonObj(interfaceButtonLeft, buttonPress, functLeft);
    ip->setButtonObj(interfaceButtonRight, buttonPress, functRight);
    ip->setButtonObj(interfaceButtonUp, buttonPress, functUp);
    ip->setButtonObj(interfaceButtonDown, buttonPress, functDown);

    ip->clearButton(interfaceButtonNote);
    ip->clearButton(interfaceButtonInstr);
    ip->clearButton(interfaceButtonFx1);
    ip->clearButton(interfaceButtonFx2);

    ip->clearButton(interfaceButtonCopy);
    ip->clearButton(interfaceButtonDelete);
    ip->clearButton(interfaceButtonRec);
    ip->clearButton(interfaceButtonInsert);

    ip->setPotObj(interfacePot0, functEncoder);
    ip->setPadsGlobal(functPads);
}

void cInstrumentEditor::clearDefaultScreenFunct()
{
    ip->clearAllPots();
    ip->clearButtonsRange(interfaceButton0, interfaceButton7);

    ip->clearButton(interfaceButtonLeft);
    ip->clearButton(interfaceButtonRight);
    ip->clearButton(interfaceButtonUp);
    ip->clearButton(interfaceButtonDown);

    ip->clearButton(interfaceButtonParams);
}

void cInstrumentEditor::switchToEffectScreen()
{
    isEffectsWindow = true;
    setEffectScreenFunct();
    showEffectScreen();
}
void cInstrumentEditor::setEffectScreenFunct()
{
    ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    ip->setButtonObj(interfaceButton0, buttonPress, functEffectSelectOverdrive);
    ip->setButtonObj(interfaceButton1, buttonPress, functEffectSelectBitDepth);
    ip->setButtonObj(interfaceButton2, buttonPress, functEffectSelectReverb);
    ip->setButtonObj(interfaceButton3, buttonPress, functEffectSelectDelay);
    ip->setButtonObj(interfaceButton4, buttonPress, functOpenInstrumentSynthesizer);
    ip->setButtonObj(interfaceButton6, buttonPress, functEffectSavePti);

    ip->setButtonObj(interfaceButton7, buttonPress, functEffectBack);
}

void cInstrumentEditor::changeParamValue(int value, ValueChangeSource source)
{
    bool invert = source != kEncoder || interfaceGlobals.invertValForList;

    if (isEffectsWindow && (mode == mtInstEditModeParams))
    {
        switch (selectedPlaceSends)
        {
            case 0: cInstrumentEditor::changeParamsOverdrive(value); break;
            case 1: cInstrumentEditor::changeParamsBitDepth(value); break;
            case 2: cInstrumentEditor::changeParamsReverbSend(value); break;
            case 3: cInstrumentEditor::changeParamsDelaySend(value); break;
            default: break;
        }
        return;
    }

    if (frameData.multiSelActiveNum != 0)
    {
        stepThroughNodes(value, invert);
    }
    else
    {
        uint8_t mode_places = selectedPlace[mode] + mode * 10;

        switch (mode_places)
        {
            case 0: cInstrumentEditor::changeParamsVolume(value); break;
            case 1: cInstrumentEditor::changeParamsPanning(value); break;
            case 2: cInstrumentEditor::changeParamsTune(value); break;
            case 3: cInstrumentEditor::changeParamsFineTune(value); break;
            case 4: cInstrumentEditor::changeFilterType(invert ? -value : value); break;
            case 5: cInstrumentEditor::changeFilterCutoff(value); break;
            case 6: cInstrumentEditor::changeFilterResonance(value); break;

            case 10: cInstrumentEditor::changeSelectedEnv(invert ? -value : value); break;
            case 11: cInstrumentEditor::changeSelectedEnvState(invert ? -value : value); break;
            case 12:
            {
                const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
                if (IE->editorInstrument->envelope[envSlot].loop == 1)
                    cInstrumentEditor::changeSelectedLfoShape(invert ? -value : value);
                else if (IE->editorInstrument->envelope[envSlot].loop >= 2)
                    cInstrumentEditor::changeSelectedWtWave(value);
                else
                    cInstrumentEditor::changeSelectedEnvAttack(value);
                break;
            }
            case 13:
            {
                const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
                if (IE->editorInstrument->envelope[envSlot].loop == 1 ||
                    IE->editorInstrument->envelope[envSlot].loop == 3)
                    cInstrumentEditor::changeSelectedLfoSpeed(invert ? -value : value);
                else
                    cInstrumentEditor::changeSelectedEnvDecay(value);
                break;
            }
            case 14:
            {
                const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
                if (IE->editorInstrument->envelope[envSlot].loop == 1)
                    cInstrumentEditor::changeSelectedLfoAmount(value);
                else if (IE->editorInstrument->envelope[envSlot].loop == 2)
                    cInstrumentEditor::changeSelectedWtWarpXY(value);
                else if (IE->editorInstrument->envelope[envSlot].loop == 3)
                    cInstrumentEditor::changeSelectedWtWarpX(value);
                else
                    cInstrumentEditor::changeSelectedEnvSustain(value);
                break;
            }
            case 15:
            {
                const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
                if (IE->editorInstrument->envelope[envSlot].loop == 0)
                    cInstrumentEditor::changeSelectedEnvRelease(value);
                else if (IE->editorInstrument->envelope[envSlot].loop == 2)
                    cInstrumentEditor::changeSelectedWtSustain(value);
                else if (IE->editorInstrument->envelope[envSlot].loop == 3)
                    cInstrumentEditor::changeSelectedWtWarpY(value);
                break;
            }
            case 16:
            {
                const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
                if (IE->editorInstrument->envelope[envSlot].loop == 0 ||
                    IE->editorInstrument->envelope[envSlot].loop == 2)
                    cInstrumentEditor::changeSelectedEnvAmount(value);
                else if (IE->editorInstrument->envelope[envSlot].loop == 3)
                    cInstrumentEditor::changeSelectedLfoAmount(value);
                break;
            }
                //		case 17: changeEnvLoop(value); 		break;

            case 30: changeParamsVelocity(value); break;
            case 31: changeParamsOffset(value); break;
            case 32:
            case 33:
            case 34:
            case 35:
            case 36:
            case 37: changeParamsCC(mode_places - 32, invert ? -value : value); break;

            case 20: changeSynthCCList(invert ? -value : value); break;
            case 21: changeInstList(invert ? -value : value); break;

            default: break;
        }
    }
}

//==============================================================================================================

static void functSelectParams(uint8_t button, uint8_t state)
{
    uint8_t mode_places = button + IE->mode * 10;

    if (IE->mode == mtInstEditModeEnv)
    {
        const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
        if (state == buttonPress) // zmienianie pozycji listy przyciskiem pod ekranem
        {
            IE->rollListOver = true;

            if (button == 2 && IE->selectedPlace[IE->mode] == 1)
                cInstrumentEditor::changeSelectedEnvState(1);
            if (button == 3 && IE->selectedPlace[IE->mode] == 2)
            {
                if (IE->editorInstrument->envelope[envSlot].loop == 1)
                    cInstrumentEditor::changeSelectedLfoShape(1);
                else if (IE->editorInstrument->envelope[envSlot].loop >= 2)
                    cInstrumentEditor::changeSelectedWtWave(1);
            }
            if (button == 4 && IE->selectedPlace[IE->mode] == 3)
            {
                if (IE->editorInstrument->envelope[envSlot].loop == 1 ||
                    IE->editorInstrument->envelope[envSlot].loop == 3)
                    cInstrumentEditor::changeSelectedLfoSpeed(1);
            }

            IE->rollListOver = false;
        }

        if ((button > 2) && (!IE->editorInstrument->isAutomationDestEnabled(IE->selectedEnvelope)))
        {
            return;
        }
    }
    else if (IE->mode == mtInstEditModeParams)
    {
        if (state == buttonPress) // zmienianie pozycji listy przyciskiem pod ekranem
        {
            if (button == 4 && IE->selectedPlace[IE->mode] == 4)
            {
                IE->rollListOver = true;
                cInstrumentEditor::changeFilterType(1);
                IE->rollListOver = false;
            }
        }
        if ((button == 5) || (button == 6))
        {
            if ((!IE->selectNodes[button].isActive) && (!IE->editorInstrument->filterEnable))
            {
                return;
            }
        }
    }

    uint8_t node = button;

    if (state == buttonPress)
    {
        uint8_t lastSelectedPlace = IE->selectedPlace[IE->mode];
        const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);

        IE->selectedPlace[IE->mode] =
          (button > 0) && (IE->mode == mtInstEditModeEnv) ? button - 1 : button;

        if ((IE->mode == mtInstEditModeEnv) && (IE->selectedPlace[IE->mode] > 4) &&
            (IE->editorInstrument->envelope[envSlot].loop == 1))
        {
            IE->selectedPlace[IE->mode] = 4;
        }

        switch (mode_places)
        {
            case 0: IE->addNode(cInstrumentEditor::changeParamsVolume, node); break;
            case 1: IE->addNode(cInstrumentEditor::changeParamsPanning, node); break;
            case 2: IE->addNode(cInstrumentEditor::changeParamsTune, node); break;
            case 3: IE->addNode(cInstrumentEditor::changeParamsFineTune, node); break;
            case 4: IE->addNode(cInstrumentEditor::changeFilterType, node, 1); break;
            case 5: IE->addNode(cInstrumentEditor::changeFilterCutoff, node); break;
            case 6: IE->addNode(cInstrumentEditor::changeFilterResonance, node); break;

            case 10:
                if (lastSelectedPlace == 0)
                    cInstrumentEditor::changeSelectedEnv(-1);
                break;
            case 11:
                if (lastSelectedPlace == 0)
                    cInstrumentEditor::changeSelectedEnv(1);
                break;
            case 12: break;
            case 13:
                if (IE->editorInstrument->envelope[envSlot].loop == 1)
                    IE->addNode(cInstrumentEditor::changeSelectedLfoShape, node, 1);
                else if (IE->editorInstrument->envelope[envSlot].loop >= 2)
                    IE->addNode(cInstrumentEditor::changeSelectedWtWave, node);
                else
                    IE->addNode(cInstrumentEditor::changeSelectedEnvAttack, node);
                break;
            case 14:
                if (IE->editorInstrument->envelope[envSlot].loop == 1 ||
                    IE->editorInstrument->envelope[envSlot].loop == 3)
                    IE->addNode(cInstrumentEditor::changeSelectedLfoSpeed, node, 1);
                else
                    IE->addNode(cInstrumentEditor::changeSelectedEnvDecay, node);
                break;
            case 15:
                if (IE->editorInstrument->envelope[envSlot].loop == 1)
                    IE->addNode(cInstrumentEditor::changeSelectedLfoAmount, node);
                else if (IE->editorInstrument->envelope[envSlot].loop == 2)
                    IE->addNode(cInstrumentEditor::changeSelectedWtWarpXY, node);
                else if (IE->editorInstrument->envelope[envSlot].loop == 3)
                    IE->addNode(cInstrumentEditor::changeSelectedWtWarpX, node);
                else
                    IE->addNode(cInstrumentEditor::changeSelectedEnvSustain, node);
                break;
            case 16:
                if (IE->editorInstrument->envelope[envSlot].loop == 0)
                    IE->addNode(cInstrumentEditor::changeSelectedEnvRelease, node);
                else if (IE->editorInstrument->envelope[envSlot].loop == 2)
                    IE->addNode(cInstrumentEditor::changeSelectedWtSustain, node);
                else if (IE->editorInstrument->envelope[envSlot].loop == 3)
                    IE->addNode(cInstrumentEditor::changeSelectedWtWarpY, node);
                break;
            case 17:
                if (IE->editorInstrument->envelope[envSlot].loop == 0 ||
                    IE->editorInstrument->envelope[envSlot].loop == 2)
                    IE->addNode(cInstrumentEditor::changeSelectedEnvAmount, node);
                else if (IE->editorInstrument->envelope[envSlot].loop == 3)
                    IE->addNode(cInstrumentEditor::changeSelectedLfoAmount, node);
                break;

            // midi params
            case 30: IE->addNode(changeParamsVelocity, node); break;
            case 31: IE->addNode(changeParamsOffset, node); break;
            case 32: IE->addNode(changeParamsCCA, node); break;
            case 33: IE->addNode(changeParamsCCB, node); break;
            case 34: IE->addNode(changeParamsCCC, node); break;
            case 35: IE->addNode(changeParamsCCD, node); break;
            case 36: IE->addNode(changeParamsCCE, node); break;
            case 37: IE->addNode(changeParamsCCF, node); break;
        }

        if ((node > 2) && (IE->mode == mtInstEditModeEnv))
        {
            if (((node <= 5) && (IE->editorInstrument->envelope[envSlot].loop == 1)) ||
                ((node <= 7) && (IE->editorInstrument->envelope[envSlot].loop != 1)))
            {
                IE->frameData.multisel[button].frameNum = node - 1;
                IE->frameData.multisel[button].isActive = 1;
                IE->frameData.multiSelActiveNum += 1;
            }
        }
        else if (IE->mode == mtInstEditModeParams)
        {
            IE->frameData.multisel[button].frameNum = node;
            IE->frameData.multisel[button].isActive = 1;
            IE->frameData.multiSelActiveNum += 1;
        }
    }
    else if (state == buttonRelease)
    {
        if (IE->frameData.multiSelActiveNum)
        {
            if (IE->frameData.multisel[button].isActive)
            {
                IE->removeNode(node);
                IE->frameData.multiSelActiveNum -= 1;

                IE->frameData.multisel[button].isActive = 0;

                if (IE->frameData.multiSelActiveNum == 0)
                {
                    const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
                    IE->selectedPlace[IE->mode] =
                      (button > 0) && (IE->mode == mtInstEditModeEnv) ? button - 1 : button;
                    if ((IE->mode == mtInstEditModeEnv) && (IE->selectedPlace[IE->mode] > 4) &&
                        (IE->editorInstrument->envelope[envSlot].loop == 1))
                        IE->selectedPlace[IE->mode] = 4;

                    if ((!IE->editorInstrument->filterEnable) && ((button == 5) || (button == 6)) &&
                        (IE->mode == mtInstEditModeParams))
                    {
                        IE->selectedPlace[IE->mode] = 4;
                    }
                }
            }
        }
    }

    IE->activateLabelsBorder();
}

static void functInstrSynthScreenButtons(uint8_t button, uint8_t state)
{
    if (state != buttonPress)
    {
        return;
    }

    if (button < 4)
    {
        if (IE->selectedPlace[IE->mode] == 1)
        {
            IE->selectedPlace[IE->mode] = 0;
            IE->activateLabelsBorder();
            return;
        }
    }
    else if (button >= 4 && button < 7)
    {
        if (IE->selectedPlace[IE->mode] == 0)
        {
            IE->selectedPlace[IE->mode] = 1;
            IE->activateLabelsBorder();
            return;
        }
    }

    if (button == 0)
    {
        IE->changeSynthCCList(-1);
    }
    else if (button == 3)
    {
        IE->changeSynthCCList(1);
    }
    else if (button == 4)
    {
        IE->changeInstList(-1);
    }
    else if (button == 6)
    {
        IE->changeInstList(1);
    }

    if (button == 7)
    {
        IE->start(1);
        IE->switchToEffectScreen();
        return;
    }

    IE->activateLabelsBorder();
}

static void functSwitchToSends(uint8_t button, uint8_t state)
{
    if ((IE->mode == mtInstEditModeEnv) || (IE->mode == mtInstEditModeMidi))
    {
        functSelectParams(button, state);
        return;
    }

    if (state == buttonPress)
    {
        for (uint8_t node = 0; node < MAX_SELECT_NODES; node++)
        {
            if (IE->selectNodes[node].isActive == 1)
            {
                return;
            }
        }

        IE->switchToEffectScreen();
    }
}

//==============================================================================================================
static void functEncoder(int16_t value)
{
    IE->changeParamValue(value, IE->kEncoder);
}

static void functUp()
{
    IE->changeParamValue(1);
}

static void functDown()
{
    IE->changeParamValue(-1);
}

//=========================================================================================================
static void functLeft()
{
    if (IE->frameData.multiSelActiveNum != 0)
    {
        return;
    }

    if ((IE->isEffectsWindow) && (IE->mode == mtInstEditModeParams))
    {
        if (IE->selectedPlaceSends > 0)
            IE->selectedPlaceSends--;
        IE->showSendScreenFrame();
        return;
    }

    if (IE->selectedPlace[IE->mode] > 0)
        IE->selectedPlace[IE->mode]--;

    if (IE->mode == mtInstEditModeParams)
    {
        if (IE->selectedPlace[IE->mode] == 6)
        {
            if (!IE->editorInstrument->filterEnable)
            {
                IE->selectedPlace[IE->mode] = 4;
            }
        }
    }

    IE->activateLabelsBorder();
}

static void functRight()
{
    if (IE->frameData.multiSelActiveNum != 0)
    {
        return;
    }

    if ((IE->isEffectsWindow) && (IE->mode == mtInstEditModeParams))
    {
        if (IE->selectedPlaceSends < IE->frameData.placesCount - 1)
            IE->selectedPlaceSends++;

        IE->showSendScreenFrame();
        return;
    }

    if (IE->selectedPlace[IE->mode] < IE->frameData.placesCount - 1)
        IE->selectedPlace[IE->mode]++;

    if (IE->mode == mtInstEditModeParams)
    {
        if (IE->selectedPlace[IE->mode] == 5)
        {
            if (!IE->editorInstrument->filterEnable)
            {
                IE->selectedPlace[IE->mode] = 4;
            }
        }
    }

    if (IE->mode == mtInstEditModeEnv)
    {
        const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
        if ((!IE->editorInstrument->isAutomationDestEnabled(IE->selectedEnvelope)) &&
            (IE->selectedPlace[IE->mode] > 1))
        {
            IE->selectedPlace[IE->mode] = 1;
        }
        else if ((IE->selectedPlace[IE->mode] > 4) &&
                 (IE->editorInstrument->envelope[envSlot].loop == 1))
        {
            IE->selectedPlace[IE->mode] = 4;
        }
    }

    IE->activateLabelsBorder();
}

//=========================================================================================================
static void functPlayAction()
{
    if (sequencer.getSeqState() == Sequencer::SEQ_STATE_STOP)
    {
        if (IE->buttons->isButtonPressed(interfaceButtonShift))
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
static void functShift(uint8_t state)
{
    if (state == buttonPress)
    {
        if (mtPopups.isNoteKeyboardPopupActive())
        {
            mtPopups.toggleNoteKeyboardPopup(InterfacePopups::noteKeyboardOption::kShowEditButtons);
        }
    }
    else if (state == buttonRelease)
    {
        if (mtPopups.isNoteKeyboardPopupActive())
        {
            mtPopups.toggleNoteKeyboardPopup(InterfacePopups::noteKeyboardOption::kForceShow);
        }
    }
}

static void functDelete(uint8_t state)
{
    if (state == buttonPress)
    {
        if ((IE->isEffectsWindow) && (IE->mode == mtInstEditModeParams))
        {
            switch (IE->selectedPlaceSends)
            {
                case 0: IE->setDefaultOverdrive(); break;
                case 1: IE->setDefaultBitDepth(); break;
                case 2: IE->setDefaultReverbSend(); break;
                case 3: IE->setDefaultDelaySend(); break;
            }

            return;
        }

        if (IE->frameData.multiSelActiveNum != 0)
        {
            for (uint8_t i = 0; i < 8; i++)
            {
                if (IE->selectNodes[i].isActive)
                {
                    uint8_t mode_places = i + IE->mode * 10;

                    IE->setDefaultValue(mode_places);
                }
            }
        }
        else
        {
            uint8_t mode_places = IE->selectedPlace[IE->mode] + IE->mode * 10;

            IE->setDefaultValue(mode_places);
        }
    }
}

static void functSwitchModule(uint8_t button)
{
    if (!performanceMode.canPerformanceModeContinueAfterSwitchingTo(button))
    {
        performanceMode.turnOffAtEndOfPattern();
    }

    IE->isEffectsWindow = false;
    IE->eventFunct(eventSwitchModule, IE, &button, 0);
}

static void functSwitchMode()
{
    mtPopups.hideInfoPopup();
    sequencer.stopManualNotes();

    IE->isEffectsWindow = false;

    IE->clearAllNodes();
    IE->cancelMultiFrame();

    if (IE->mode == mtInstEditModeParams)
    {
        IE->mode = mtInstEditModeEnv;
    }
    else if (IE->mode == mtInstEditModeEnv || IE->mode == mtInstEditModeInstrSynth)
    {
        IE->mode = mtInstEditModeParams;
    }
#if GUI_HANDLE_SYNTH
    else if (IE->mode == mtInstEditModeSynth)
    {
        IE->exitSynthEditor();
        IE->mode = mtInstEditModeSynthMacros;
    }
    else if (IE->mode == mtInstEditModeSynthMacros)
    {
        IE->mode = mtInstEditModeSynth;
    }
    else if (IE->mode == mtInstEditModeSynthPatchEdit)
    {
        IE->cleanPatchEditorResources();
        IE->mode = mtInstEditModeSynth;
    }
#endif

    IE->start(0);
}

//======================================================================================================================

void cInstrumentEditor::changeSynthCCList(int16_t value)
{
    if (IE->instSynthCCListPos + value < 0)
        IE->instSynthCCListPos = 0;
    else if (IE->instSynthCCListPos + value > instSynthCCData.length - 1)
        IE->instSynthCCListPos = instSynthCCData.length - 1;
    else
        IE->instSynthCCListPos += value;

    IE->showSynthCCList();
}

void cInstrumentEditor::changeInstList(int16_t value)
{
    IE->instListPos = clamp(IE->instListPos + value, 0, kSampleInstrumentsCount - 1);

    mtProject.values.lastUsedInstrument = instListPos;
    editorInstrument = &mtProject.instrument[mtProject.values.lastUsedInstrument];

    IE->showInstList();
    IE->titleBar->showCurrentInstrumentOnRightLabel();
    IE->lightUpPadBoard();
}

void cInstrumentEditor::changeSelectedEnv(int16_t value)
{
    if (IE->isAnyNodeActive())
    {
        return;
    }

    value = clamp<int>(value, -3, 3);

    if (IE->selectedEnvelope + value < 0)
        IE->selectedEnvelope = 0;
    else if (IE->selectedEnvelope + value > (autoDestCount - 1))
        IE->selectedEnvelope = autoDestCount - 1;
    else
        IE->selectedEnvelope += value;

    IE->showInstrumentEnv();

    IE->showEnvList();
}

void cInstrumentEditor::changeSelectedEnvState(int16_t value)
{
    if (IE->isAnyNodeActive())
    {
        return;
    }

    IE->changeEnvState(IE->selectedEnvelope, value);
}

void cInstrumentEditor::changeSelectedEnvAttack(int16_t value)
{
    IE->changeEnvAttack(IE->selectedEnvelope, value);
}

void cInstrumentEditor::changeSelectedEnvDecay(int16_t value)
{
    IE->changeEnvDecay(IE->selectedEnvelope, value);
}

void cInstrumentEditor::changeSelectedEnvSustain(int16_t value)
{
    IE->changeEnvSustain(IE->selectedEnvelope, value);
}

void cInstrumentEditor::changeSelectedEnvRelease(int16_t value)
{
    IE->changeEnvRelease(IE->selectedEnvelope, value);
}

void cInstrumentEditor::changeSelectedEnvAmount(int16_t value)
{
    IE->changeEnvAmount(IE->selectedEnvelope, value);
}

void cInstrumentEditor::changeSelectedLfoShape(int16_t value)
{
    IE->changeLfoShape(IE->selectedEnvelope, value);
}
void cInstrumentEditor::changeSelectedLfoSpeed(int16_t value)
{
    IE->changeLfoSpeed(IE->selectedEnvelope, value);
}

void cInstrumentEditor::changeSelectedLfoAmount(int16_t value)
{
    IE->changeLfoAmount(IE->selectedEnvelope, value);
}

void cInstrumentEditor::changeSelectedWtWave(int16_t value)
{
    IE->changeWtWave(IE->selectedEnvelope, value);
}

void cInstrumentEditor::changeSelectedWtWarpXY(int16_t value)
{
    IE->changeWtWarpXY(IE->selectedEnvelope, value);
}

void cInstrumentEditor::changeSelectedWtWarpX(int16_t value)
{
    IE->changeWtWarpX(IE->selectedEnvelope, value);
}

void cInstrumentEditor::changeSelectedWtWarpY(int16_t value)
{
    IE->changeWtWarpY(IE->selectedEnvelope, value);
}

void cInstrumentEditor::changeSelectedWtSustain(int16_t value)
{
    IE->changeWtSustain(IE->selectedEnvelope, value);
}

void cInstrumentEditor::changeEnvState(int idx, int16_t value)
{
    const int envIdx = automationDestToEnvSlot(idx);
    int listPosition = 0;
    if (!IE->editorInstrument->isAutomationDestEnabled(idx))
        listPosition = 0;
    else if (IE->editorInstrument->envelope[envIdx].loop == 0)
        listPosition = 1;
    else if (IE->editorInstrument->envelope[envIdx].loop == 1)
        listPosition = 2;
    else if (IE->editorInstrument->envelope[envIdx].loop == 2)
        listPosition = 3;
    else if (IE->editorInstrument->envelope[envIdx].loop == 3)
        listPosition = 4;

    constexpr int kListPositionMax = 4;

    if (listPosition + value > kListPositionMax)
        listPosition = IE->rollListOver ? 0 : kListPositionMax;
    else if (listPosition + value < 0)
        listPosition = 0;
    else
        listPosition += value;

    switch (listPosition)
    {
        case 0:
            IE->editorInstrument->setAutomationDestEnabled(idx, false);
            if (!IE->editorInstrument->isEnvEnabled(envIdx))
            {
                IE->editorInstrument->envelope[envIdx].loop = 0;
            }
            break;
        case 1:
            IE->editorInstrument->setAutomationDestEnabled(idx, true);
            IE->editorInstrument->envelope[envIdx].loop = 0;
            break;
        case 2:
            IE->editorInstrument->setAutomationDestEnabled(idx, true);
            IE->editorInstrument->envelope[envIdx].loop = 1;
            if (IE->editorInstrument->lfo[envIdx].shape >= lfoShapeCount)
                IE->editorInstrument->lfo[envIdx].shape = 0;
            {
                uint8_t maxSpd = (envIdx == envAmp) ? CONSTRAIN_SPEED_AMP_ENVELOPE : CONSTRAIN_SPEED_OTHER_ENVELOPE;
                if (IE->editorInstrument->lfo[envIdx].speed > maxSpd)
                    IE->editorInstrument->lfo[envIdx].speed = 0;
            }
            break;
        case 3:
            IE->editorInstrument->setAutomationDestEnabled(idx, true);
            IE->editorInstrument->envelope[envIdx].loop = 2; // WT Env
            if (IE->editorInstrument->envelope[envIdx].decay == 0)
                IE->editorInstrument->envelope[envIdx].decay = 1000;
            break;
        case 4:
            IE->editorInstrument->setAutomationDestEnabled(idx, true);
            IE->editorInstrument->envelope[envIdx].loop = 3; // WT LFO
            {
                uint8_t maxSpd = (envIdx == envAmp) ? CONSTRAIN_SPEED_AMP_ENVELOPE : CONSTRAIN_SPEED_OTHER_ENVELOPE;
                if (IE->editorInstrument->lfo[envIdx].speed > maxSpd)
                    IE->editorInstrument->lfo[envIdx].speed = 0;
            }
            break;
    }

    uint32_t statusByte = 0;
    switch (envIdx)
    {
        case envAmp: statusByte = LFO_AMP_SEND_MASK; break;
        case envCutoff: statusByte = LFO_FILTER_SEND_MASK; break;
        case envWtPos: statusByte = LFO_WT_POS_SEND_MASK; break;
        case envGranPos: statusByte = LFO_GRAN_POS_SEND_MASK; break;
        case envPan: statusByte = LFO_PANNING_SEND_MASK; break;
        case envFinetune: statusByte = LFO_FINETUNE_SEND_MASK; break;
        default: break;
    }
    for (int i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(statusByte);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (idx == IE->selectedEnvelope) &&
                          (mode == mtInstrumentEditorMode::mtInstEditModeEnv);

    if (isParamVisible)
    {
        IE->showInstrumentEnv();
        IE->showEnvState();
    }
}

void cInstrumentEditor::changeEnvAttack(int idx, int16_t value)
{
    const int envIdx = automationDestToEnvSlot(idx);
    if (!IE->editorInstrument->isAutomationDestEnabled(idx))
        return;

    uint16_t localCurrentValue =
      IE->editorInstrument->envelope[envIdx].attack > ENVELOPE_MICRO_RANGE
        ? mapClamped<int>(IE->editorInstrument->envelope[envIdx].attack,
                          ENVELOPE_MICRO_RANGE,
                          ATTACK_MAX,
                          ENVELOPE_MICRO_VAL,
                          ENVELOPE_MICRO_VAL + ((ATTACK_MAX - ENVELOPE_MICRO_RANGE) / 100))
        : mapClamped<int>(IE->editorInstrument->envelope[envIdx].attack,
                          0,
                          ENVELOPE_MICRO_RANGE,
                          0,
                          ENVELOPE_MICRO_VAL);

    if (localCurrentValue + value >
        (ENVELOPE_MICRO_VAL + ((ATTACK_MAX - ENVELOPE_MICRO_RANGE) / 100)))
        localCurrentValue = ENVELOPE_MICRO_VAL + ((ATTACK_MAX - ENVELOPE_MICRO_RANGE) / 100);
    else if (localCurrentValue + value < 0)
        localCurrentValue = 0;
    else
        localCurrentValue += value;

    IE->editorInstrument->envelope[envIdx].attack =
      localCurrentValue > ENVELOPE_MICRO_VAL
        ? mapClamped<int>(localCurrentValue,
                          ENVELOPE_MICRO_VAL,
                          ENVELOPE_MICRO_VAL + ((ATTACK_MAX - ENVELOPE_MICRO_RANGE) / 100),
                          ENVELOPE_MICRO_RANGE,
                          ATTACK_MAX)
        : mapClamped<int>(localCurrentValue, 0, ENVELOPE_MICRO_VAL, 0, ENVELOPE_MICRO_RANGE);

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (idx == IE->selectedEnvelope) &&
                          (mode == mtInstrumentEditorMode::mtInstEditModeEnv) &&
                          IE->editorInstrument->envelope[envIdx].loop == 0;

    if (isParamVisible)
        IE->showEnvAttack();
}

void cInstrumentEditor::changeEnvDecay(int idx, int16_t value)
{
    const int envIdx = automationDestToEnvSlot(idx);
    if (!IE->editorInstrument->isAutomationDestEnabled(idx))
        return;

    uint16_t localCurrentValue =
      IE->editorInstrument->envelope[envIdx].decay > ENVELOPE_MICRO_RANGE
        ? mapClamped<int>(IE->editorInstrument->envelope[envIdx].decay,
                          ENVELOPE_MICRO_RANGE,
                          DECAY_MAX,
                          ENVELOPE_MICRO_VAL,
                          ENVELOPE_MICRO_VAL + ((DECAY_MAX - ENVELOPE_MICRO_RANGE) / 100))
        : mapClamped<int>(IE->editorInstrument->envelope[envIdx].decay,
                          0,
                          ENVELOPE_MICRO_RANGE,
                          0,
                          ENVELOPE_MICRO_VAL);

    if (localCurrentValue + value >
        (ENVELOPE_MICRO_VAL + ((DECAY_MAX - ENVELOPE_MICRO_RANGE) / 100)))
        localCurrentValue = ENVELOPE_MICRO_VAL + ((DECAY_MAX - ENVELOPE_MICRO_RANGE) / 100);
    else if (localCurrentValue + value < 0)
        localCurrentValue = 0;
    else
        localCurrentValue += value;

    IE->editorInstrument->envelope[envIdx].decay =
      localCurrentValue > ENVELOPE_MICRO_VAL
        ? mapClamped<int>(localCurrentValue,
                          ENVELOPE_MICRO_VAL,
                          ENVELOPE_MICRO_VAL + ((DECAY_MAX - ENVELOPE_MICRO_RANGE) / 100),
                          ENVELOPE_MICRO_RANGE,
                          DECAY_MAX)
        : mapClamped<int>(localCurrentValue, 0, ENVELOPE_MICRO_VAL, 0, ENVELOPE_MICRO_RANGE);

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    uint32_t statusByte = 0;
    switch (envIdx)
    {
        case envAmp: statusByte = LFO_AMP_SEND_MASK; break;
        case envCutoff: statusByte = LFO_FILTER_SEND_MASK; break;
        case envWtPos: statusByte = LFO_WT_POS_SEND_MASK; break;
        case envGranPos: statusByte = LFO_GRAN_POS_SEND_MASK; break;
        case envPan: statusByte = LFO_PANNING_SEND_MASK; break;
        case envFinetune: statusByte = LFO_FINETUNE_SEND_MASK; break;
        default: break;
    }
    for (int i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(statusByte);
    }

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (idx == IE->selectedEnvelope) &&
                          (mode == mtInstrumentEditorMode::mtInstEditModeEnv) &&
                          (IE->editorInstrument->envelope[envIdx].loop == 0 ||
                           IE->editorInstrument->envelope[envIdx].loop == 2);

    if (isParamVisible)
        IE->showEnvDecay();
}

void cInstrumentEditor::changeEnvSustain(int idx, int16_t value)
{
    const int envIdx = automationDestToEnvSlot(idx);
    if (!IE->editorInstrument->isAutomationDestEnabled(idx))
        return;

    float fVal = value * 0.01;

    if (IE->editorInstrument->envelope[envIdx].sustain + fVal < 0)
        IE->editorInstrument->envelope[envIdx].sustain = 0;
    else if (IE->editorInstrument->envelope[envIdx].sustain + fVal > SUSTAIN_MAX)
        IE->editorInstrument->envelope[envIdx].sustain = SUSTAIN_MAX;
    else
        IE->editorInstrument->envelope[envIdx].sustain += fVal;

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (idx == IE->selectedEnvelope) &&
                          (mode == mtInstrumentEditorMode::mtInstEditModeEnv) &&
                          IE->editorInstrument->envelope[envIdx].loop == 0;

    if (isParamVisible)
        IE->showEnvSustain();
}

void cInstrumentEditor::changeEnvRelease(int idx, int16_t value)
{
    const int envIdx = automationDestToEnvSlot(idx);
    if (!IE->editorInstrument->isAutomationDestEnabled(idx))
        return;

    uint16_t localCurrentValue =
      IE->editorInstrument->envelope[envIdx].release > ENVELOPE_MICRO_RANGE
        ? mapClamped<int>(IE->editorInstrument->envelope[envIdx].release,
                          ENVELOPE_MICRO_RANGE,
                          RELEASE_MAX,
                          ENVELOPE_MICRO_VAL,
                          ENVELOPE_MICRO_VAL + ((RELEASE_MAX - ENVELOPE_MICRO_RANGE) / 100))
        : mapClamped<int>(IE->editorInstrument->envelope[envIdx].release,
                          0,
                          ENVELOPE_MICRO_RANGE,
                          0,
                          ENVELOPE_MICRO_VAL);

    if (localCurrentValue + value >
        (ENVELOPE_MICRO_VAL + ((RELEASE_MAX - ENVELOPE_MICRO_RANGE) / 100)))
        localCurrentValue = ENVELOPE_MICRO_VAL + ((RELEASE_MAX - ENVELOPE_MICRO_RANGE) / 100);
    else if (localCurrentValue + value < 0)
        localCurrentValue = 0;
    else
        localCurrentValue += value;

    IE->editorInstrument->envelope[envIdx].release =
      localCurrentValue > ENVELOPE_MICRO_VAL
        ? mapClamped<int>(localCurrentValue,
                          ENVELOPE_MICRO_VAL,
                          ENVELOPE_MICRO_VAL + ((RELEASE_MAX - ENVELOPE_MICRO_RANGE) / 100),
                          ENVELOPE_MICRO_RANGE,
                          RELEASE_MAX)
        : mapClamped<int>(localCurrentValue, 0, ENVELOPE_MICRO_VAL, 0, ENVELOPE_MICRO_RANGE);

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (idx == IE->selectedEnvelope) &&
                          (mode == mtInstrumentEditorMode::mtInstEditModeEnv) &&
                          IE->editorInstrument->envelope[envIdx].loop == 0;

    if (isParamVisible)
        IE->showEnvRelease();
}

void cInstrumentEditor::changeEnvAmount(int idx, int16_t value)
{
    const int envIdx = automationDestToEnvSlot(idx);
    if (!IE->editorInstrument->isAutomationDestEnabled(idx))
        return;

    float fVal = value * 0.01;

    if (IE->editorInstrument->envelope[envIdx].amount + fVal < 0)
        IE->editorInstrument->envelope[envIdx].amount = 0;
    else if (IE->editorInstrument->envelope[envIdx].amount + fVal > AMOUNT_MAX)
        IE->editorInstrument->envelope[envIdx].amount = AMOUNT_MAX;
    else
        IE->editorInstrument->envelope[envIdx].amount += fVal;

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (idx == IE->selectedEnvelope) &&
                          (mode == mtInstrumentEditorMode::mtInstEditModeEnv) &&
                          (IE->editorInstrument->envelope[envIdx].loop == 0 ||
                           IE->editorInstrument->envelope[envIdx].loop == 2);

    if (isParamVisible)
        IE->showEnvAmount();
}

void cInstrumentEditor::changeLfoShape(int idx, int16_t value)
{
    const int envIdx = automationDestToEnvSlot(idx);
    if (IE->editorInstrument->lfo[envIdx].shape + value < 0)
        IE->editorInstrument->lfo[envIdx].shape = 0;
    else if (IE->editorInstrument->lfo[envIdx].shape + value > (lfoShapeCount - 1))
    {
        IE->editorInstrument->lfo[envIdx].shape = (IE->rollListOver ? 0 : lfoShapeCount - 1);
    }
    else
        IE->editorInstrument->lfo[envIdx].shape += value;

    uint32_t statusByte = 0;
    switch (envIdx)
    {
        case envAmp: statusByte = LFO_AMP_SEND_MASK; break;
        case envCutoff: statusByte = LFO_FILTER_SEND_MASK; break;
        case envWtPos: statusByte = LFO_WT_POS_SEND_MASK; break;
        case envGranPos: statusByte = LFO_GRAN_POS_SEND_MASK; break;
        case envPan: statusByte = LFO_PANNING_SEND_MASK; break;
        case envFinetune: statusByte = LFO_FINETUNE_SEND_MASK; break;
        default: break;
    }
    for (int i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(statusByte);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (idx == IE->selectedEnvelope) &&
                          (mode == mtInstrumentEditorMode::mtInstEditModeEnv) &&
                          IE->editorInstrument->envelope[envIdx].loop == 1;

    if (isParamVisible)
        IE->showLfoShape();
}

void cInstrumentEditor::changeLfoSpeed(int idx, int16_t value)
{
    const int envIdx = automationDestToEnvSlot(idx);
    uint8_t speedConstrain =
      envIdx == envAmp ? CONSTRAIN_SPEED_AMP_ENVELOPE : CONSTRAIN_SPEED_OTHER_ENVELOPE;

    value = clamp<int>(value, -5, 5);

    if (IE->editorInstrument->lfo[envIdx].speed + value < 0)
    {
        IE->editorInstrument->lfo[envIdx].speed = 0;
    }
    else if (IE->editorInstrument->lfo[envIdx].speed + value > speedConstrain)
    {
        IE->editorInstrument->lfo[envIdx].speed = (IE->rollListOver ? 0 : speedConstrain);
    }
    else
    {
        IE->editorInstrument->lfo[envIdx].speed += value;
    }

    uint32_t statusByte = 0;
    switch (envIdx)
    {
        case envAmp: statusByte = LFO_AMP_SEND_MASK; break;
        case envCutoff: statusByte = LFO_FILTER_SEND_MASK; break;
        case envWtPos: statusByte = LFO_WT_POS_SEND_MASK; break;
        case envGranPos: statusByte = LFO_GRAN_POS_SEND_MASK; break;
        case envPan: statusByte = LFO_PANNING_SEND_MASK; break;
        case envFinetune: statusByte = LFO_FINETUNE_SEND_MASK; break;
        default: break;
    }
    for (int i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(statusByte);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (idx == IE->selectedEnvelope) &&
                          (mode == mtInstrumentEditorMode::mtInstEditModeEnv) &&
                          (IE->editorInstrument->envelope[envIdx].loop == 1 ||
                           IE->editorInstrument->envelope[envIdx].loop == 3);

    if (isParamVisible)
        IE->showLfoSpeed();
}

void cInstrumentEditor::changeLfoAmount(int idx, int16_t value)
{
    const int envIdx = automationDestToEnvSlot(idx);
    float fVal = value * 0.01;

    if (IE->editorInstrument->lfo[envIdx].amount + fVal < 0)
        IE->editorInstrument->lfo[envIdx].amount = 0;
    else if (IE->editorInstrument->lfo[envIdx].amount + fVal > AMOUNT_MAX)
        IE->editorInstrument->lfo[envIdx].amount = AMOUNT_MAX;
    else
        IE->editorInstrument->lfo[envIdx].amount += fVal;

    if (IE->editorInstrument->envelope[envIdx].loop == 3)
    {
        IE->editorInstrument->envelope[envIdx].amount = IE->editorInstrument->lfo[envIdx].amount;
    }

    uint32_t statusByte = 0;
    switch (envIdx)
    {
        case envAmp: statusByte = LFO_AMP_SEND_MASK; break;
        case envCutoff: statusByte = LFO_FILTER_SEND_MASK; break;
        case envWtPos: statusByte = LFO_WT_POS_SEND_MASK; break;
        case envGranPos: statusByte = LFO_GRAN_POS_SEND_MASK; break;
        case envPan: statusByte = LFO_PANNING_SEND_MASK; break;
        case envFinetune: statusByte = LFO_FINETUNE_SEND_MASK; break;
        default: break;
    }
    for (int i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(statusByte);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (idx == IE->selectedEnvelope) &&
                          (mode == mtInstrumentEditorMode::mtInstEditModeEnv) &&
                          (IE->editorInstrument->envelope[envIdx].loop == 1 ||
                           IE->editorInstrument->envelope[envIdx].loop == 3);

    if (isParamVisible)
        IE->showLfoAmount();
}

static void triggerEnvelopeRefresh(int idx)
{
    const int envIdx = automationDestToEnvSlot(idx);
    uint32_t statusByte = 0;
    switch (envIdx)
    {
        case envAmp: statusByte = LFO_AMP_SEND_MASK; break;
        case envCutoff: statusByte = LFO_FILTER_SEND_MASK; break;
        case envWtPos: statusByte = LFO_WT_POS_SEND_MASK; break;
        case envGranPos: statusByte = LFO_GRAN_POS_SEND_MASK; break;
        case envPan: statusByte = LFO_PANNING_SEND_MASK; break;
        case envFinetune: statusByte = LFO_FINETUNE_SEND_MASK; break;
        default: break;
    }
    for (int i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(statusByte);
    }
}

void cInstrumentEditor::changeWtWave(int idx, int16_t value)
{
    const int envIdx = automationDestToEnvSlot(idx);
    int val = (int)IE->editorInstrument->lfo[envIdx].shape + value;
    if (val < 0) val = IE->rollListOver ? 255 : 0;
    else if (val > 255) val = IE->rollListOver ? 0 : 255;
    IE->editorInstrument->lfo[envIdx].shape = (uint8_t)val;

    triggerEnvelopeRefresh(idx);
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    if (mtInterface.isInstrumentParamsScreenActive() && (idx == IE->selectedEnvelope) &&
        (mode == mtInstrumentEditorMode::mtInstEditModeEnv))
    {
        IE->showWtEnvWave();
    }
}

void cInstrumentEditor::changeWtWarpXY(int idx, int16_t value)
{
    const int envIdx = automationDestToEnvSlot(idx);
    int8_t wx = unpackWarpX(IE->editorInstrument->lfo[envIdx].wtBend);
    int8_t wy = unpackWarpY(IE->editorInstrument->lfo[envIdx].wtBend);

#ifdef TRACKER_MINI
    bool adjustY = IE->buttons->isButtonPressed(interfaceButtonShift) || (value >= 10 || value <= -10);
    int16_t deltaY = (value >= 10) ? 1 : ((value <= -10) ? -1 : value);
#else
    bool adjustY = IE->buttons->isButtonPressed(interfaceButtonShift);
    int16_t deltaY = value;
#endif

    if (adjustY)
    {
        int yVal = wy + deltaY;
        if (yVal < -7) yVal = -7; else if (yVal > 7) yVal = 7;
        wy = (int8_t)yVal;
    }
    else
    {
        int xVal = wx + value;
        if (xVal < -7) xVal = -7; else if (xVal > 7) xVal = 7;
        wx = (int8_t)xVal;
    }

    IE->editorInstrument->lfo[envIdx].wtBend = packWarpXY(wx, wy);
    triggerEnvelopeRefresh(idx);
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    if (mtInterface.isInstrumentParamsScreenActive() && (idx == IE->selectedEnvelope) &&
        (mode == mtInstrumentEditorMode::mtInstEditModeEnv))
    {
        IE->showWtWarpXY();
    }
}

void cInstrumentEditor::changeWtWarpX(int idx, int16_t value)
{
    const int envIdx = automationDestToEnvSlot(idx);
    int8_t wx = unpackWarpX(IE->editorInstrument->lfo[envIdx].wtBend);
    int8_t wy = unpackWarpY(IE->editorInstrument->lfo[envIdx].wtBend);
    int xVal = wx + value;
    if (xVal < -7) xVal = -7; else if (xVal > 7) xVal = 7;
    wx = (int8_t)xVal;

    IE->editorInstrument->lfo[envIdx].wtBend = packWarpXY(wx, wy);
    triggerEnvelopeRefresh(idx);
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    if (mtInterface.isInstrumentParamsScreenActive() && (idx == IE->selectedEnvelope) &&
        (mode == mtInstrumentEditorMode::mtInstEditModeEnv))
    {
        IE->showWtWarpX();
    }
}

void cInstrumentEditor::changeWtWarpY(int idx, int16_t value)
{
    const int envIdx = automationDestToEnvSlot(idx);
    int8_t wx = unpackWarpX(IE->editorInstrument->lfo[envIdx].wtBend);
    int8_t wy = unpackWarpY(IE->editorInstrument->lfo[envIdx].wtBend);
    int yVal = wy + value;
    if (yVal < -7) yVal = -7; else if (yVal > 7) yVal = 7;
    wy = (int8_t)yVal;

    IE->editorInstrument->lfo[envIdx].wtBend = packWarpXY(wx, wy);
    triggerEnvelopeRefresh(idx);
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    if (mtInterface.isInstrumentParamsScreenActive() && (idx == IE->selectedEnvelope) &&
        (mode == mtInstrumentEditorMode::mtInstEditModeEnv))
    {
        IE->showWtWarpY();
    }
}

void cInstrumentEditor::changeWtSustain(int idx, int16_t value)
{
    const int envIdx = automationDestToEnvSlot(idx);
    int val = (int)IE->editorInstrument->lfo[envIdx].wtSust + value;
    if (val < 0) val = 0; else if (val > 99) val = 99;
    IE->editorInstrument->lfo[envIdx].wtSust = (uint8_t)val;

    triggerEnvelopeRefresh(idx);
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    if (mtInterface.isInstrumentParamsScreenActive() && (idx == IE->selectedEnvelope) &&
        (mode == mtInstrumentEditorMode::mtInstEditModeEnv))
    {
        IE->showWtSustain();
    }
}

void cInstrumentEditor::setDefaultWtWave()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    editorInstrument->lfo[envSlot].shape = 0;
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    showWtEnvWave();
}

void cInstrumentEditor::setDefaultWtWarp()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    editorInstrument->lfo[envSlot].wtBend = 0;
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    if (editorInstrument->envelope[envSlot].loop == 2)
        showWtWarpXY();
    else if (editorInstrument->envelope[envSlot].loop == 3)
    {
        showWtWarpX();
        showWtWarpY();
    }
}

void cInstrumentEditor::setDefaultWtSustain()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    editorInstrument->lfo[envSlot].wtSust = 0;
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    showWtSustain();
}

void cInstrumentEditor::changeFilterType(int16_t value)
{
    if (IE->filterModeListPos + value < 0)
        IE->filterModeListPos = 0;
    else if (IE->filterModeListPos + value > filterModeCount - 1)
        IE->filterModeListPos = (IE->rollListOver ? 0 : filterModeCount - 1);
    else
        IE->filterModeListPos += value;

    if (IE->filterModeListPos == 0)
    {
        IE->editorInstrument->filterEnable = 0;
        IE->editorInstrument->filterType = lowPass;
    }
    else if (IE->filterModeListPos == 1)
    {
        IE->editorInstrument->filterEnable = 1;
        IE->editorInstrument->filterType = lowPass;
    }
    else if (IE->filterModeListPos == 2)
    {
        IE->editorInstrument->filterEnable = 1;
        IE->editorInstrument->filterType = highPass;
    }
    else if (IE->filterModeListPos >= filterModeCount - 1)
    {
        IE->editorInstrument->filterEnable = 1;
        IE->editorInstrument->filterType = bandPass;
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (IE->mode == mtInstrumentEditorMode::mtInstEditModeParams) &&
                          !IE->isEffectsWindow;

    if (isParamVisible)
        IE->showFilterType();
}

void cInstrumentEditor::changeFilterCutoff(int16_t value)
{
    //	if(bgB + value < 0) bgB = 0;
    //	else if(bgB + value > 128 ) bgB = 128;
    //	else bgB += value;

    if (!IE->editorInstrument->filterEnable)
        return;
    float fVal = value * 0.01;

    if (IE->editorInstrument->cutOff + fVal < MIN_CUTOFF)
        IE->editorInstrument->cutOff = MIN_CUTOFF;
    else if (IE->editorInstrument->cutOff + fVal > MAX_CUTOFF)
        IE->editorInstrument->cutOff = MAX_CUTOFF;
    else
        IE->editorInstrument->cutOff += fVal;

    //	display.setBacklightBrightness(map(editorInstrument->cutOff,MIN_CUTOFF,MAX_CUTOFF,0,128));
    //	instrumentPlayer[0].instrumentBasedMod.cutoff = IE->editorInstrument->cutOff;
    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(CUTOFF_MASK);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (IE->mode == mtInstrumentEditorMode::mtInstEditModeParams) &&
                          !IE->isEffectsWindow;

    if (isParamVisible)
        IE->showFilterCutOff();
}

void cInstrumentEditor::changeFilterResonance(int16_t value)
{
    if (!IE->editorInstrument->filterEnable)
        return;

    float fVal = value * (RESONANCE_MAX - RESONANCE_MIN) / 100;

    if (IE->editorInstrument->resonance + fVal < RESONANCE_MIN)
        IE->editorInstrument->resonance = RESONANCE_MIN;
    else if (IE->editorInstrument->resonance + fVal > RESONANCE_MAX)
        IE->editorInstrument->resonance = RESONANCE_MAX;
    else
        IE->editorInstrument->resonance += fVal;

    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(RESONANCE_MASK);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (IE->mode == mtInstrumentEditorMode::mtInstEditModeParams) &&
                          !IE->isEffectsWindow;

    if (isParamVisible)
        IE->showFilterResonance();
}

void cInstrumentEditor::changeParamsVolume(int16_t value)
{
    if (IE->editorInstrument->volume + value < 0)
        IE->editorInstrument->volume = 0;
    else if (IE->editorInstrument->volume + value > MAX_INSTRUMENT_VOLUME)
        IE->editorInstrument->volume = MAX_INSTRUMENT_VOLUME;
    else
        IE->editorInstrument->volume += value;

    for (uint8_t i = 0; i < 8; i++)
    {
        //		if(instrumentPlayer[i].currentInstrument_idx ==
        // mtProject.values.lastUsedInstrument)
        instrumentPlayer[i].setRefreshBit(VOLUME_MASK);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (IE->mode == mtInstrumentEditorMode::mtInstEditModeParams) &&
                          !IE->isEffectsWindow;

    if (isParamVisible)
    {
        IE->showParamsVolume();
    }
}

void cInstrumentEditor::changeParamsTune(int16_t value)
{
    value = value == 10 ? 12 : (value == -10 ? -12 : value);

    if (IE->editorInstrument->tune + value < MIN_INSTRUMENT_TUNE)
        IE->editorInstrument->tune = MIN_INSTRUMENT_TUNE;
    else if (IE->editorInstrument->tune + value > MAX_INSTRUMENT_TUNE)
        IE->editorInstrument->tune = MAX_INSTRUMENT_TUNE;
    else
        IE->editorInstrument->tune += value;

    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(TUNE_MASK);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (IE->mode == mtInstrumentEditorMode::mtInstEditModeParams) &&
                          !IE->isEffectsWindow;

    if (isParamVisible)
        IE->showParamsTune();
}

void cInstrumentEditor::changeParamsFineTune(int16_t value)
{
    if (IE->editorInstrument->fineTune + value < MIN_INSTRUMENT_FINETUNE)
        IE->editorInstrument->fineTune = MIN_INSTRUMENT_FINETUNE;
    else if (IE->editorInstrument->fineTune + value > MAX_INSTRUMENT_FINETUNE)
        IE->editorInstrument->fineTune = MAX_INSTRUMENT_FINETUNE;
    else
        IE->editorInstrument->fineTune += value;

    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(FINETUNE_MASK);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (IE->mode == mtInstrumentEditorMode::mtInstEditModeParams) &&
                          !IE->isEffectsWindow;

    if (isParamVisible)
        IE->showParamsFineTune();
}

void cInstrumentEditor::changeParamsPanning(int16_t value)
{
    if (IE->editorInstrument->panning + value < PANNING_MIN)
        IE->editorInstrument->panning = PANNING_MIN;
    else if (IE->editorInstrument->panning + value > PANNING_MAX)
        IE->editorInstrument->panning = PANNING_MAX;
    else
        IE->editorInstrument->panning += value;

    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(PANNING_MASK);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (IE->mode == mtInstrumentEditorMode::mtInstEditModeParams) &&
                          !IE->isEffectsWindow;

    if (isParamVisible)
        IE->showParamsPanning();
}

void cInstrumentEditor::changeParamsDelaySend(int16_t value)
{
    if (IE->editorInstrument->delaySend + value < SEND_MIN)
        IE->editorInstrument->delaySend = SEND_MIN;
    else if (IE->editorInstrument->delaySend + value > SEND_MAX)
        IE->editorInstrument->delaySend = SEND_MAX;
    else
        IE->editorInstrument->delaySend += value;

    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(DELAY_SEND_MASK);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (IE->mode == mtInstrumentEditorMode::mtInstEditModeParams) &&
                          IE->isEffectsWindow;

    if (isParamVisible)
        IE->showParamsDelaySend();
}

void cInstrumentEditor::changeParamsReverbSend(int16_t value)
{
    if (IE->editorInstrument->reverbSend + value < SEND_MIN)
        IE->editorInstrument->reverbSend = SEND_MIN;
    else if (IE->editorInstrument->reverbSend + value > SEND_MAX)
        IE->editorInstrument->reverbSend = SEND_MAX;
    else
        IE->editorInstrument->reverbSend += value;

    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(REVERB_SEND_MASK);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (IE->mode == mtInstrumentEditorMode::mtInstEditModeParams) &&
                          IE->isEffectsWindow;

    if (isParamVisible)
        IE->showParamsReverbSend();
}

void cInstrumentEditor::changeParamsOverdrive(int16_t value)
{
    if (IE->editorInstrument->overdrive + value < SEND_MIN)
        IE->editorInstrument->overdrive = SEND_MIN;
    else if (IE->editorInstrument->overdrive + value > SEND_MAX)
        IE->editorInstrument->overdrive = SEND_MAX;
    else
        IE->editorInstrument->overdrive += value;

    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(OVERDRIVE_MASK);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (IE->mode == mtInstrumentEditorMode::mtInstEditModeParams) &&
                          IE->isEffectsWindow;

    if (isParamVisible)
        IE->showParamsOverdrive();
}

void cInstrumentEditor::changeParamsBitDepth(int16_t value)
{
    if (IE->editorInstrument->bitDepth + value < BIT_DEPTH_MIN)
        IE->editorInstrument->bitDepth = BIT_DEPTH_MIN;
    else if (IE->editorInstrument->bitDepth + value > BIT_DEPTH_MAX)
        IE->editorInstrument->bitDepth = BIT_DEPTH_MAX;
    else
        IE->editorInstrument->bitDepth += value;

    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(BIT_DEPTH_MASK);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    bool isParamVisible = mtInterface.isInstrumentParamsScreenActive() &&
                          (IE->mode == mtInstrumentEditorMode::mtInstEditModeParams) &&
                          IE->isEffectsWindow;

    if (isParamVisible)
        IE->showParamsBitDepth();
}

void changeParamsVelocity(int16_t value)
{
    uint8_t* temp_velocity =
      &mtProject.values
         .midiInstrument[mtProject.values.lastUsedInstrument - kSampleInstrumentsCount]
         .velocity;

    if (*temp_velocity + value < 0)
        *temp_velocity = 0;
    else if (*temp_velocity + value > 127)
        *temp_velocity = 127;
    else
        *temp_velocity += value;

    IE->showParamsVelocity();

    newFileManager.setProjectStructChanged();
}
// midi only
void changeParamsOffset(int16_t value)
{
    uint8_t* temp_offset =
      &mtProject.values
         .midiInstrument[mtProject.values.lastUsedInstrument - kSampleInstrumentsCount]
         .offset;

    if (*temp_offset + value < 0)
        *temp_offset = 0;
    else if (*temp_offset + value > 100)
        *temp_offset = 100;
    else
        *temp_offset += value;

    IE->showParamsOffset();

    newFileManager.setProjectStructChanged();
}

void changeParamsCC(int8_t index, int16_t value)
{
    if (index > 5)
    {
        return;
    }

    value = clamp<int>(value, -10, 10);

    uint8_t* temp_cc =
      &mtProject.values
         .midiInstrument[mtProject.values.lastUsedInstrument - kSampleInstrumentsCount]
         .ccOut[index];

    if (*temp_cc + value < 0)
        *temp_cc = 0;
    else if (*temp_cc + value > 130)
        *temp_cc = 130;
    else
        *temp_cc += value;

    IE->showParamsCC(index);

    newFileManager.setProjectStructChanged();
}

void changeParamsCCA(int16_t value)
{
    changeParamsCC(0, value);
}
void changeParamsCCB(int16_t value)
{
    changeParamsCC(1, value);
}
void changeParamsCCC(int16_t value)
{
    changeParamsCC(2, value);
}
void changeParamsCCD(int16_t value)
{
    changeParamsCC(3, value);
}
void changeParamsCCE(int16_t value)
{
    changeParamsCC(4, value);
}
void changeParamsCCF(int16_t value)
{
    changeParamsCC(5, value);
}

//======================================================================================================================

void cInstrumentEditor::onControlChangeReceive(int param, int value)
{
#ifdef FEATURE_SYNTH
    if (isSynthInstrument(mtProject.values.lastUsedInstrument))
    {
        int synthIdx = instrumentIdxToSynthIdx(mtProject.values.lastUsedInstrument);

        handleSynthParamControlChange(synthIdx, (SynthInstrumentParamsCC)param, value);

        return;
    }

#endif

    if (mtProject.values.lastUsedInstrument >= kSampleInstrumentsCount)
    {
        if (instListPos >= kSampleInstrumentsCount)
        {
            instListPos = 0;
        }
        editorInstrument = &mtProject.instrument[instListPos];
    }
    else
    {
        editorInstrument = &mtProject.instrument[mtProject.values.lastUsedInstrument];
    }

    changeControlChangeParam(SampleInstrumentParamsCC(param),
                             getControlChangeDelta(SampleInstrumentParamsCC(param), value));
}

#ifdef FEATURE_SYNTH
void cInstrumentEditor::handleSynthParamControlChange(int synthPartIdx,
                                                      SynthInstrumentParamsCC paramCcId,
                                                      int value)
{
    float min = getSynthCCParamValueMin(synthPartIdx, paramCcId);
    float max = getSynthCCParamValueMax(synthPartIdx, paramCcId);

    float newValue = mapClamped<float>(value, 0, 127, min, max);

    bool isShown = isParamShown(synthPartIdx, paramCcId);

    switch (paramCcId)
    {
        case SynthInstrumentParamsCC::kMacro1:
        case SynthInstrumentParamsCC::kMacro2:
        case SynthInstrumentParamsCC::kMacro3:
        case SynthInstrumentParamsCC::kMacro4:
        case SynthInstrumentParamsCC::kMacro5:
        case SynthInstrumentParamsCC::kMacro6:
            mtProject.values.synthPart[synthPartIdx].macroValue[macroCcIdToIdx[paramCcId]] =
              newValue;
            setSynthMacro(synthPartIdx, macroCcIdToIdx[paramCcId], newValue, isShown);
            break;
        case SynthInstrumentParamsCC::kSynthReverbSend:
            mtProject.values.synthPart[synthPartIdx].reverbSend = roundf(newValue);
            setSynthReverbSend(synthPartIdx, newValue, isShown);
            break;
        case SynthInstrumentParamsCC::kSynthDelaySend:
            mtProject.values.synthPart[synthPartIdx].delaySend = roundf(newValue);
            setSynthDelaySend(synthPartIdx, newValue, isShown);
            break;
        case SynthInstrumentParamsCC::kSynthGain:
            mtProject.values.synthPart[synthPartIdx].gainDb =
              roundf(newValue * 2.0f) / 2.0f; // round to half
            setSynthGain(synthPartIdx, newValue, isShown);
            break;
        case SynthInstrumentParamsCC::kSynthPanning:
            mtProject.values.synthPart[synthPartIdx].panning = roundf(newValue);
            setSynthPanning(synthPartIdx, newValue, isShown);
            break;

        default: break;
    }
}

bool cInstrumentEditor::isParamShown(int synthPartIdx, SynthInstrumentParamsCC paramCcId)
{
    if (!mtInterface.isInstrumentParamsScreenActive())
    {
        return false;
    }

    std::optional<mtInstrumentEditorMode> modeForParam = paramToMode(paramCcId);

    return modeForParam.has_value() && mode == modeForParam.value() &&
           synthPartIdx == instrumentIdxToSynthIdx(mtProject.values.lastUsedInstrument);
}

std::optional<mtInstrumentEditorMode> cInstrumentEditor::paramToMode(
  SynthInstrumentParamsCC paramCcId)
{
    switch (paramCcId)
    {
        case SynthInstrumentParamsCC::kMacro1:
        case SynthInstrumentParamsCC::kMacro2:
        case SynthInstrumentParamsCC::kMacro3:
        case SynthInstrumentParamsCC::kMacro4:
        case SynthInstrumentParamsCC::kMacro5:
        case SynthInstrumentParamsCC::kMacro6:
        case SynthInstrumentParamsCC::kSynthReverbSend:
        case SynthInstrumentParamsCC::kSynthDelaySend:
            return mtInstrumentEditorMode::mtInstEditModeSynthMacros;
        case SynthInstrumentParamsCC::kSynthGain:
        case SynthInstrumentParamsCC::kSynthPanning:
            return mtInstrumentEditorMode::mtInstEditModeSynth;
        default: return std::nullopt;
    }
}

float cInstrumentEditor::getSynthCCParamValueMax(int synthPartIdx,
                                                 SynthInstrumentParamsCC paramCcId)
{
    switch (paramCcId)
    {
        case SynthInstrumentParamsCC::kMacro1:
        case SynthInstrumentParamsCC::kMacro2:
        case SynthInstrumentParamsCC::kMacro3:
        case SynthInstrumentParamsCC::kMacro4:
        case SynthInstrumentParamsCC::kMacro5:
        case SynthInstrumentParamsCC::kMacro6:
            return getMacroMax(synthPartIdx, macroCcIdToIdx[paramCcId]);
        case SynthInstrumentParamsCC::kSynthDelaySend:
        case SynthInstrumentParamsCC::kSynthReverbSend: return 100.0f;
        case SynthInstrumentParamsCC::kSynthGain: return 12.0f;
        case SynthInstrumentParamsCC::kSynthPanning: return 100.0f;
        default: return 0.0f;
    }
}

float cInstrumentEditor::getSynthCCParamValueMin(int, SynthInstrumentParamsCC paramCcId)
{
    switch (paramCcId)
    {
        case SynthInstrumentParamsCC::kSynthGain: return -96.0f;
        case SynthInstrumentParamsCC::kSynthPanning: return -100.0f;
        default: return 0.0f;
    }
}

bool cInstrumentEditor::isSynthPercSectionBegin(int note)
{
    int kickBeginNote =
      isInScale(60, mtProject.values.padBoardRootNote, mtProject.values.padBoardScale) ? 60 : 61;
    assert(
      isInScale(kickBeginNote, mtProject.values.padBoardRootNote, mtProject.values.padBoardScale));
    int snareBeginNote =
      isInScale(62, mtProject.values.padBoardRootNote, mtProject.values.padBoardScale) ? 62 : 63;
    assert(
      isInScale(snareBeginNote, mtProject.values.padBoardRootNote, mtProject.values.padBoardScale));
    int closedHihatBeginNote =
      isInScale(64, mtProject.values.padBoardRootNote, mtProject.values.padBoardScale) ? 64 : 65;
    assert(isInScale(
      closedHihatBeginNote, mtProject.values.padBoardRootNote, mtProject.values.padBoardScale));
    int openHihatBeginNote =
      isInScale(66, mtProject.values.padBoardRootNote, mtProject.values.padBoardScale) ? 66 : 67;
    assert(isInScale(
      openHihatBeginNote, mtProject.values.padBoardRootNote, mtProject.values.padBoardScale));
    int cymbalBeginNote =
      isInScale(68, mtProject.values.padBoardRootNote, mtProject.values.padBoardScale) ? 68 : 69;
    assert(isInScale(
      cymbalBeginNote, mtProject.values.padBoardRootNote, mtProject.values.padBoardScale));
    int percBeginNote =
      isInScale(70, mtProject.values.padBoardRootNote, mtProject.values.padBoardScale) ? 70 : 71;
    assert(
      isInScale(percBeginNote, mtProject.values.padBoardRootNote, mtProject.values.padBoardScale));
    int bassBeginNoteFirstSection =
      isInScale(0, mtProject.values.padBoardRootNote, mtProject.values.padBoardScale) ? 0 : 1;
    assert(isInScale(bassBeginNoteFirstSection,
                     mtProject.values.padBoardRootNote,
                     mtProject.values.padBoardScale));
    int bassBeginNoteSecondSection =
      isInScale(72, mtProject.values.padBoardRootNote, mtProject.values.padBoardScale) ? 72 : 73;
    assert(isInScale(bassBeginNoteSecondSection,
                     mtProject.values.padBoardRootNote,
                     mtProject.values.padBoardScale));

    return note == kickBeginNote || note == snareBeginNote || note == closedHihatBeginNote ||
           note == openHihatBeginNote || note == cymbalBeginNote || note == percBeginNote ||
           note == bassBeginNoteSecondSection || note == bassBeginNoteFirstSection;
}

int cInstrumentEditor::convertPadForEditPatchScreen(int pad)
{
    static const uint8_t padBoard[48] = {
        36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 24, 25, 26, 27,
        28, 29, 30, 31, 32, 33, 34, 35, 12, 13, 14, 15, 16, 17, 18, 19,
        20, 21, 22, 23, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
    };

    static const uint8_t patchEditPadBoard[32] = { 28, 29, 30, 31, 32, 33, 34, 35, 20, 21, 22,
                                                   23, 24, 25, 26, 27, 12, 13, 14, 15, 16, 17,
                                                   18, 19, 4,  5,  6,  7,  8,  9,  10, 11 };

    int resultIdx = pad - 4 * ((pad / 12) + 1);

    auto result = std::find(std::begin(padBoard), std::end(padBoard), patchEditPadBoard[resultIdx]);
    assert(result != std::end(padBoard));

    return std::distance(std::begin(padBoard), result);
}

#endif

int16_t cInstrumentEditor::getControlChangeDelta(SampleInstrumentParamsCC param, int cc_value)
{
    int automationIdx = getEnvelopeIdx(param);

    switch (param)
    {
        case kTune:
        {
            int8_t destValue =
              mapClamped<int>(cc_value, 0, 127, MIN_INSTRUMENT_TUNE, MAX_INSTRUMENT_TUNE);

            return (int16_t)destValue - (int16_t)IE->editorInstrument->tune;
        }
        break;
        case kFinetune:
        {
            int8_t destValue =
              mapClamped<int>(cc_value, 0, 127, MIN_INSTRUMENT_FINETUNE, MAX_INSTRUMENT_FINETUNE);

            return (int16_t)destValue - (int16_t)IE->editorInstrument->fineTune;
        }
        break;
        case kVolume:
        {
            uint8_t destValue = mapClamped<int>(cc_value, 0, 127, 0, MAX_INSTRUMENT_VOLUME);

            return (int16_t)destValue - (int16_t)IE->editorInstrument->volume;
        }
        case kPanning:
        {
            uint8_t destValue = mapClamped<int>(cc_value, 0, 127, PANNING_MIN, PANNING_MAX);

            return (int16_t)destValue - (int16_t)IE->editorInstrument->panning;
        }
        case kFiltertype:
        {
            uint8_t destValue = mapClamped<int>(cc_value, 0, 127, 0, filterModeCount);

            return (int16_t)destValue -
                   ((int16_t)IE->editorInstrument->filterType + IE->editorInstrument->filterEnable);
        }
        case kCutoff:
        {
            uint8_t destValue = mapClamped<int>(cc_value, 0, 127, 0, 100);
            uint8_t cutoff = (uint8_t)mapClamped(
              (float)IE->editorInstrument->cutOff, MIN_CUTOFF, MAX_CUTOFF, 0.0f, 100.0f);

            return (int16_t)destValue - (int16_t)cutoff;
        }
        case kResonance:
        {
            uint8_t destValue = mapClamped<int>(cc_value, 0, 127, 0, 100);
            uint8_t resonance = (uint8_t)mapClamped(
              (float)IE->editorInstrument->resonance, RESONANCE_MIN, RESONANCE_MAX, 0.0f, 100.0f);

            return (int16_t)destValue - (int16_t)resonance;
        }
        case kOverdrive:
        {
            uint8_t destValue = mapClamped<int>(cc_value, 0, 127, SEND_MIN, SEND_MAX);

            return (int16_t)destValue - (int16_t)IE->editorInstrument->overdrive;
        }
        case kBitDepth:
        {
            uint8_t destValue = mapClamped<int>(cc_value, 0, 127, BIT_DEPTH_MIN, BIT_DEPTH_MAX);

            return (int16_t)destValue - (int16_t)IE->editorInstrument->bitDepth;
        }
        case kReverbSend:
        {
            uint8_t destValue = mapClamped<int>(cc_value, 0, 127, SEND_MIN, SEND_MAX);

            return (int16_t)destValue - (int16_t)IE->editorInstrument->reverbSend;
        }
        case kDelaySend:
        {
            uint8_t destValue = mapClamped<int>(cc_value, 0, 127, SEND_MIN, SEND_MAX);

            return (int16_t)destValue - (int16_t)IE->editorInstrument->delaySend;
        }
        case kVolumeAutomationType:
        case kPanningAutomationType:
        case kCutoffAutomationType:
        case kWavetablePositionAutomationType:
        case kGranularPositionAutomationType:
        case kFinetuneAutomationType:
        {
            const int envIdx = automationDestToEnvSlot(automationIdx);
            uint8_t destValue = mapClamped<int>(cc_value, 0, 127, 0, 2);

            return (int16_t)destValue -
                   (int16_t)((IE->editorInstrument->isAutomationDestEnabled(automationIdx) ? 1 : 0) +
                             IE->editorInstrument->envelope[envIdx].loop);
        }
        case kVolumeAttack_ShapeLFO:
        case kPanningAttack_ShapeLFO:
        case kCutoffAttack_ShapeLFO:
        case kWavetablePositionAttack_ShapeLFO:
        case kGranularPositionAttack_ShapeLFO:
        case kFinetuneAttack_ShapeLFO:
        {
            const int envIdx = automationDestToEnvSlot(automationIdx);
            if (IE->editorInstrument->envelope[envIdx].loop)
            {
                uint8_t destValue = mapClamped<int>(cc_value, 0, 127, 0, lfoShapeCount - 1);

                return (int16_t)destValue - (int16_t)IE->editorInstrument->lfo[envIdx].shape;
            }
            else
            {
                constexpr int kMicroRange = 64;
                uint16_t destValue =
                  cc_value >= kMicroRange
                    ? mapClamped<int>(cc_value,
                                      kMicroRange,
                                      127,
                                      ENVELOPE_MICRO_VAL,
                                      ENVELOPE_MICRO_VAL +
                                        ((ATTACK_MAX - ENVELOPE_MICRO_RANGE) / 100))
                    : mapClamped<int>(cc_value, 0, kMicroRange, 0, ENVELOPE_MICRO_VAL);
                uint16_t attack =
                  IE->editorInstrument->envelope[envIdx].attack > ENVELOPE_MICRO_RANGE
                    ? mapClamped<int>(IE->editorInstrument->envelope[envIdx].attack,
                                      ENVELOPE_MICRO_RANGE,
                                      ATTACK_MAX,
                                      ENVELOPE_MICRO_VAL,
                                      ENVELOPE_MICRO_VAL +
                                        ((ATTACK_MAX - ENVELOPE_MICRO_RANGE) / 100))
                    : mapClamped<int>(IE->editorInstrument->envelope[envIdx].attack,
                                      0,
                                      ENVELOPE_MICRO_RANGE,
                                      0,
                                      ENVELOPE_MICRO_VAL);

                return (int16_t)destValue - (int16_t)attack;
            }
        }
        case kVolumeDecay_SpeedLFO:
        case kPanningDecay_SpeedLFO:
        case kCutoffDecay_SpeedLFO:
        case kWavetablePositionDecay_SpeedLFO:
        case kGranularPositionDecay_SpeedLFO:
        case kFinetuneDecay_SpeedLFO:
        {
            const int envIdx = automationDestToEnvSlot(automationIdx);
            if (IE->editorInstrument->envelope[envIdx].loop)
            {
                uint8_t speedConstrain = envIdx == envAmp ? CONSTRAIN_SPEED_AMP_ENVELOPE
                                                          : CONSTRAIN_SPEED_OTHER_ENVELOPE;
                uint8_t destValue = mapClamped<int>(cc_value, 0, 127, 0, speedConstrain);

                return (int16_t)destValue - (int16_t)IE->editorInstrument->lfo[envIdx].speed;
            }
            else
            {
                constexpr int kMicroRange = 64;
                uint16_t destValue =
                  cc_value > kMicroRange
                    ? mapClamped<int>(cc_value,
                                      kMicroRange,
                                      127,
                                      ENVELOPE_MICRO_VAL,
                                      ENVELOPE_MICRO_VAL +
                                        ((ATTACK_MAX - ENVELOPE_MICRO_RANGE) / 100))
                    : mapClamped<int>(cc_value, 0, kMicroRange, 0, ENVELOPE_MICRO_VAL);

                uint16_t decay =
                  IE->editorInstrument->envelope[envIdx].decay > ENVELOPE_MICRO_RANGE
                    ? mapClamped<int>(IE->editorInstrument->envelope[envIdx].decay,
                                      ENVELOPE_MICRO_RANGE,
                                      ATTACK_MAX,
                                      ENVELOPE_MICRO_VAL,
                                      ENVELOPE_MICRO_VAL +
                                        ((ATTACK_MAX - ENVELOPE_MICRO_RANGE) / 100))
                    : mapClamped<int>(IE->editorInstrument->envelope[envIdx].decay,
                                      0,
                                      ENVELOPE_MICRO_RANGE,
                                      0,
                                      ENVELOPE_MICRO_VAL);

                return (int16_t)destValue - (int16_t)decay;
            }
        }
        case kVolumeSustain:
        case kPanningSustain:
        case kCutoffSustain:
        case kWavetablePositionSustain:
        case kGranularPositionSustain:
        case kFinetuneSustain:
        {
            const int envIdx = automationDestToEnvSlot(automationIdx);
            float destValue = map((float)cc_value, 0.0f, 127.0f, 0.0f, SUSTAIN_MAX);

            return (int16_t)(100.0f *
                             (destValue - IE->editorInstrument->envelope[envIdx].sustain));
        }
        case kVolumeRelease:
        case kPanningRelease:
        case kCutoffRelease:
        case kWavetablePositionRelease:
        case kGranularPositionRelease:
        case kFinetuneRelease:
        {
            const int envIdx = automationDestToEnvSlot(automationIdx);
            constexpr int kMicroRange = 64;
            uint16_t destValue =
               cc_value > kMicroRange
                ? mapClamped<int>(cc_value,
                                  kMicroRange,
                                  127,
                                  ENVELOPE_MICRO_VAL,
                                  ENVELOPE_MICRO_VAL + ((ATTACK_MAX - ENVELOPE_MICRO_RANGE) / 100))
                : mapClamped<int>(cc_value, 0, kMicroRange, 0, ENVELOPE_MICRO_VAL);
            uint16_t release =
              IE->editorInstrument->envelope[envIdx].release > ENVELOPE_MICRO_RANGE
                ? mapClamped<int>(IE->editorInstrument->envelope[envIdx].release,
                                  ENVELOPE_MICRO_RANGE,
                                  ATTACK_MAX,
                                  ENVELOPE_MICRO_VAL,
                                  ENVELOPE_MICRO_VAL + ((ATTACK_MAX - ENVELOPE_MICRO_RANGE) / 100))
                : mapClamped<int>(IE->editorInstrument->envelope[envIdx].release,
                                  0,
                                  ENVELOPE_MICRO_RANGE,
                                  0,
                                  ENVELOPE_MICRO_VAL);

            return (int16_t)destValue - (int16_t)release;
        }
        case kVolumeAmount:
        case kPanningAmount:
        case kCutoffAmount:
        case kWavetablePositionAmount:
        case kGranularPositionAmount:
        case kFinetuneAmount:
        {
            const int envIdx = automationDestToEnvSlot(automationIdx);
            float amount = IE->editorInstrument->envelope[envIdx].loop
                             ? IE->editorInstrument->lfo[envIdx].amount
                             : IE->editorInstrument->envelope[envIdx].amount;
            float destValue = map((float)cc_value, 0.0f, 127.0f, 0.0f, AMOUNT_MAX);

            return (int16_t)(100.0f * (destValue - amount));
        }
        default: return 0;
    }
}

void cInstrumentEditor::changeControlChangeParam(SampleInstrumentParamsCC param, int16_t delta)
{
    int automationIdx = getEnvelopeIdx(param);

    switch (param)
    {
        case kTune: changeParamsTune(delta); break;
        case kFinetune: changeParamsFineTune(delta); break;
        case kVolume: changeParamsVolume(delta); break;
        case kPanning: changeParamsPanning(delta); break;
        case kFiltertype: changeFilterType(delta); break;
        case kCutoff: changeFilterCutoff(delta); break;
        case kResonance: changeFilterResonance(delta); break;
        case kOverdrive: changeParamsOverdrive(delta); break;
        case kBitDepth: changeParamsBitDepth(delta); break;
        case kReverbSend: changeParamsReverbSend(delta); break;
        case kDelaySend: changeParamsDelaySend(delta); break;

        case kVolumeAutomationType:
        case kPanningAutomationType:
        case kCutoffAutomationType:
        case kWavetablePositionAutomationType:
        case kGranularPositionAutomationType:
        case kFinetuneAutomationType: changeEnvState(automationIdx, delta); break;
        case kVolumeAttack_ShapeLFO:
        case kPanningAttack_ShapeLFO:
        case kCutoffAttack_ShapeLFO:
        case kWavetablePositionAttack_ShapeLFO:
        case kGranularPositionAttack_ShapeLFO:
        case kFinetuneAttack_ShapeLFO:
            if (IE->editorInstrument->envelope[automationDestToEnvSlot(automationIdx)].loop)
            {
                changeLfoShape(automationIdx, delta);
            }
            else
            {
                changeEnvAttack(automationIdx, delta);
            }

            break;
        case kVolumeDecay_SpeedLFO:
        case kPanningDecay_SpeedLFO:
        case kCutoffDecay_SpeedLFO:
        case kWavetablePositionDecay_SpeedLFO:
        case kGranularPositionDecay_SpeedLFO:
        case kFinetuneDecay_SpeedLFO:
            if (IE->editorInstrument->envelope[automationDestToEnvSlot(automationIdx)].loop)
            {
                changeLfoSpeed(automationIdx, delta);
            }
            else
            {
                changeEnvDecay(automationIdx, delta);
            }
            break;
        case kVolumeSustain:
        case kPanningSustain:
        case kCutoffSustain:
        case kWavetablePositionSustain:
        case kGranularPositionSustain:
        case kFinetuneSustain: changeEnvSustain(automationIdx, delta); break;
        case kVolumeRelease:
        case kPanningRelease:
        case kCutoffRelease:
        case kWavetablePositionRelease:
        case kGranularPositionRelease:
        case kFinetuneRelease: changeEnvRelease(automationIdx, delta); break;
        case kVolumeAmount:
        case kPanningAmount:
        case kCutoffAmount:
        case kWavetablePositionAmount:
        case kGranularPositionAmount:
        case kFinetuneAmount:
            if (IE->editorInstrument->envelope[automationDestToEnvSlot(automationIdx)].loop)
            {
                changeLfoAmount(automationIdx, delta);
            }
            else
            {
                changeEnvAmount(automationIdx, delta);
            }
            break;
        default: break;
    }
}

int cInstrumentEditor::getEnvelopeIdx(SampleInstrumentParamsCC param)
{
    constexpr int kNoEnvelope = -1;

    switch (param)
    {
        case kVolumeAutomationType:
        case kVolumeAttack_ShapeLFO:
        case kVolumeDecay_SpeedLFO:
        case kVolumeSustain:
        case kVolumeRelease:
        case kVolumeAmount: return enAutomationDest::autoDestVolume;
        case kPanningAutomationType:
        case kPanningAttack_ShapeLFO:
        case kPanningDecay_SpeedLFO:
        case kPanningSustain:
        case kPanningRelease:
        case kPanningAmount: return enAutomationDest::autoDestPanning;
        case kCutoffAutomationType:
        case kCutoffAttack_ShapeLFO:
        case kCutoffDecay_SpeedLFO:
        case kCutoffSustain:
        case kCutoffRelease:
        case kCutoffAmount: return enAutomationDest::autoDestCutoff;
        case kWavetablePositionAutomationType:
        case kWavetablePositionAttack_ShapeLFO:
        case kWavetablePositionDecay_SpeedLFO:
        case kWavetablePositionSustain:
        case kWavetablePositionRelease:
        case kWavetablePositionAmount:
            return (IE->editorInstrument && IE->editorInstrument->playMode == playModeGranular)
                     ? enAutomationDest::autoDestGranLen
                     : enAutomationDest::autoDestWtPos;
        case kGranularPositionAutomationType:
        case kGranularPositionAttack_ShapeLFO:
        case kGranularPositionDecay_SpeedLFO:
        case kGranularPositionSustain:
        case kGranularPositionRelease:
        case kGranularPositionAmount: return enAutomationDest::autoDestGranPos;
        case kFinetuneAutomationType:
        case kFinetuneAttack_ShapeLFO:
        case kFinetuneDecay_SpeedLFO:
        case kFinetuneSustain:
        case kFinetuneRelease:
        case kFinetuneAmount: return enAutomationDest::autoDestFinetune;
        default: return kNoEnvelope;
    }
}

//==============================================================================================

void cInstrumentEditor::cancelPopups()
{
    if (mtPopups.getStepPopupState() != stepPopupNone)
    {
        mtPopups.hideStepPopups();

        if (isSampleInstrument(mtProject.values.lastUsedInstrument) && isEffectsWindow)
        {
            start(0);
            switchToEffectScreen();
        }
#if GUI_HANDLE_SYNTH
        else if (IE->mode == mtInstEditModeSynthPatchEdit)
        {
            start(4);
        }
#endif
        else
        {
            start(0);
        }
    }
}

//======================================================================================================================
//==============================================================================================

// ##############################################################################################
// ###############################          PAD BOARD #################################
// ##############################################################################################

void cInstrumentEditor::lightUpPadBoard()
{
    padsBacklight.clearGlobalPads(0, 1, 1);

    if (mode == mtInstEditModeInstrSynth)
    {
        if (instListPos < kSampleInstrumentsCount)
        {
            padsBacklight.setBackLayer(1, mtConfig.values.padsLightBack, instListPos);
        }
        return;
    }
#if GUI_HANDLE_SYNTH

    if (mode == mtInstEditModeSynth || mode == mtInstEditModeSynthMacros)
    {
        if (isSynthInstrument(mtProject.values.lastUsedInstrument))
        {
            int synthIdx = instrumentIdxToSynthIdx(mtProject.values.lastUsedInstrument);
            const auto& synthTypes = getSynth()->getSynthTypes();
            int synthType = std::clamp<int>(mtProject.values.synthPart[synthIdx].synthType,
                                            0,
                                            std::max<int>(0, static_cast<int>(synthTypes.size()) - 1));

            if (!synthTypes.empty() &&
                synthTypes.at(synthType) == polysynth::InstrumentTypeId::kDrumSynth)
            {
                for (int padIdx = 0; padIdx < 48; padIdx++)
                {
                    padsBacklight.setBackLayer(
                      isSynthPercSectionBegin(mtPadBoard.getNoteFromPad(padIdx)) ? 1 : 0,
                      mtConfig.values.padsLightBack,
                      padIdx);
                }
            }
            else
            {
                for (int padIdx = 0; padIdx < 48; padIdx++)
                {
                    padsBacklight.setBackLayer(0, mtConfig.values.padsLightBack, padIdx);
                }
            }
        }
        return;
    }
#endif

#if GUI_HANDLE_SYNTH
    if (IE->mode == mtInstEditModeSynthPatchEdit)
    {
        int sectionToPad = (sectionListPos / 4) * 12 + (sectionListPos % 4);

        for (uint8_t pad = 0; pad < 48; pad++)
        {
            if (pad % 12 < 4)
            {
                if (pad == sectionToPad)
                {
                    padsBacklight.setFrontLayer(1, mtConfig.values.padsLightFront, pad);
                }
                else
                {
                    padsBacklight.setBackLayer(1, mtConfig.values.padsLightBack, pad);
                }
            }
            else
            {
                if (isSynthInstrument(mtProject.values.lastUsedInstrument))
                {
                    int synthIdx = instrumentIdxToSynthIdx(mtProject.values.lastUsedInstrument);
                    const auto& synthTypes = getSynth()->getSynthTypes();
                    int synthType = std::clamp<int>(
                      mtProject.values.synthPart[synthIdx].synthType,
                      0,
                      std::max<int>(0, static_cast<int>(synthTypes.size()) - 1));

                    if (!synthTypes.empty() &&
                        synthTypes.at(synthType) == polysynth::InstrumentTypeId::kDrumSynth)
                    {

                        int playedPad = clamp<int>(IE->convertPadForEditPatchScreen(pad), 0, 47);
                        padsBacklight.setBackLayer(
                          isSynthPercSectionBegin(mtPadBoard.getNoteFromPad(playedPad)) ? 1 : 0,
                          mtConfig.values.padsLightBack,
                          pad);
                    }
                }
            }
        }

        return;
    }
#endif

    bool isBeatSlice = false;

    if (mtProject.values.lastUsedInstrument < kSampleInstrumentsCount)
    {
        isBeatSlice =
          mtProject.instrument[mtProject.values.lastUsedInstrument].playMode == playModeBeatSlice;
    }

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

static void functMiniMiddle(uint8_t state)
{
    if (IE->mode == mtInstEditModeInstrSynth)
    {
        return;
    }

    static uint8_t lastState = buttonRelease;

    if (state == lastState)
    {
        return;
    }

    lastState = state;
    // Hold is set to button function with Press state . We have to handle it by rejecting
    // repetitive states if we don't want hold.

    if (mtProject.values.lastUsedInstrument < kSampleInstrumentsCount &&
        mtProject.instrument[mtProject.values.lastUsedInstrument].playMode == playModeBeatSlice)
    {
        int note = mtProject.instrument[mtProject.values.lastUsedInstrument].selectedSlice +
                   FIRST_BEAT_SLICE_NOTE;

        if (state == buttonPress)
        {
            sequencer.handleNoteOn(Sequencer::GRID_OUTSIDE_PATTERN, note, -1, 12);
        }
        else if (state == buttonRelease)
        {
            sequencer.handleNoteOff(Sequencer::GRID_OUTSIDE_PATTERN, note, 0, 12);
        }
    }
    else
    {
        functPads(12, state, 0);
    }
}

static void functPads(uint8_t pad, uint8_t state, int16_t)
{
    if (IE->mode == mtInstEditModeInstrSynth)
    {
        if (state == buttonPress)
        {
            int16_t delta = pad - IE->instListPos;
            IE->changeInstList(delta);
        }
        return;
    }

    bool isBeatSlice = false;

    if (mtProject.values.lastUsedInstrument < kSampleInstrumentsCount)
    {
        isBeatSlice =
          mtProject.instrument[mtProject.values.lastUsedInstrument].playMode == playModeBeatSlice;
    }

    if (state == buttonPress)
    {
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

static void functInstrument(uint8_t state)
{

    if (mtPopups.isNoteKeyboardPopupActive())
    {
        mtPopups.toggleNoteKeyboardPopup();

        onNoteKeyboardHide();
    }

    if (state == buttonRelease)
    {
        if (mtProject.values.lastUsedInstrument < kSampleInstrumentsCount)
        {
            IE->editorInstrument = &mtProject.instrument[mtProject.values.lastUsedInstrument];
        }
        IE->cancelPopups();

        IE->lightUpPadBoard();
    }
    else if (state == buttonPress)
    {
        if (IE->mode == mtInstEditModeMidi)
        {
            // duzo czcionek w CC, schowajmy listy
            for (uint8_t i = 0; i < 6; i++)
            {
                display.setControlHide(IE->ccNameListControl[i]);
            }
            display.setControlHide(IE->blankSpaceCover[0]);
        }
        mtPopups.showStepPopup(stepPopupInstr, mtProject.values.lastUsedInstrument);
    }
}
static void onNoteKeyboardHide()
{
    if (IE->isEffectsWindow)
    {
        IE->start(0);
        IE->switchToEffectScreen();
    }
#if GUI_HANDLE_SYNTH
    else if (IE->mode == mtInstEditModeSynthPatchEdit)
    {
        sequencer.stopManualNotes();
        IE->mode = mtInstEditModeSynthPatchEdit;
        IE->lightUpPadBoard();
        IE->showSynthPatchEditorScreen();
        IE->setSynthPatchEditorScreenFunct();
    }
#endif
    else
    {
        IE->start(0);
    }
}
static void functNote(uint8_t value)
{
#ifdef TRACKER_MINI
    if (value == buttonPress)
    {
        if (IE->buttons->isButtonPressed(interfaceButtonShift) && !IE->keyboardManager.getState())
        {
            bool popupShown = mtPopups.toggleNoteKeyboardPopup();
            if (!popupShown)
            {
                onNoteKeyboardHide();
            }
        }
        else if (mtPopups.isNoteKeyboardPopupActive())
        {
            int padIdx = mtPopups.getNoteKeyboardFirstPadIdx();
            mtPopups.showStepPopup(stepPopupNote, padIdx);
        }
    }
    else if (value == buttonRelease)
    {
        if (mtPopups.getStepPopupState() != stepPopupNone)
        {
            IE->cancelPopups();

            if (mtPopups.isNoteKeyboardPopupActive())
            {
                mtPopups.toggleNoteKeyboardPopup(InterfacePopups::noteKeyboardOption::kForceShow);
            }
        }
    }
#else
    if (value == buttonPress)
    {
        mtPopups.showStepPopup(stepPopupNote, -1);
    }
    else if (value == buttonRelease)
    {
        IE->cancelPopups();
    }
#endif
}

void cInstrumentEditor::setDefaultValue(uint8_t place)
{
    switch (place)
    {
        case 0: setDefaultVolume(); break;
        case 1: setDefaultPanning(); break;
        case 2: setDefaultTune(); break;
        case 3: setDefaultFinetune(); break;
        case 4: setDefaultFilterType(); break;
        case 5: setDefaultCutoff(); break;
        case 6: setDefaultResonance(); break;
        case 7: setDefaultDelaySend(); break;
        case 12:
        {
            const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
            if (IE->editorInstrument->envelope[envSlot].loop == 1)
                setDefaultLfoShape();
            else if (IE->editorInstrument->envelope[envSlot].loop >= 2)
                setDefaultWtWave();
            else
                setDefaultEnvAttack();
            break;
        }
        case 13:
        {
            const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
            if (IE->editorInstrument->envelope[envSlot].loop == 1 ||
                IE->editorInstrument->envelope[envSlot].loop == 3)
                setDefaultLfoSpeed();
            else
                setDefaultEnvDecay();
            break;
        }
        case 14:
        {
            const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
            if (IE->editorInstrument->envelope[envSlot].loop == 1)
                setDefaultLfoAmount();
            else if (IE->editorInstrument->envelope[envSlot].loop >= 2)
                setDefaultWtWarp();
            else
                setDefaultEnvSustain();
            break;
        }
        case 15:
        {
            const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
            if (IE->editorInstrument->envelope[envSlot].loop == 0)
                setDefaultEnvRelease();
            else if (IE->editorInstrument->envelope[envSlot].loop == 2)
                setDefaultWtSustain();
            else if (IE->editorInstrument->envelope[envSlot].loop == 3)
                setDefaultWtWarp();
            break;
        }
        case 16:
        {
            const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
            if (IE->editorInstrument->envelope[envSlot].loop == 0 ||
                IE->editorInstrument->envelope[envSlot].loop == 2)
                setDefaultEnvAmount();
            else if (IE->editorInstrument->envelope[envSlot].loop == 3)
                setDefaultLfoAmount();
            break;
        }
        case 30: setDefaultMidiVelocity(); break;
        case 31: setDefaultMidiOffset(); break;
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37: setDefaultMidiCC(place - 22); break;
        default: break;
    }
}

static void functEffectSelectReverb()
{
    IE->selectedPlaceSends = 2;
    IE->showSendScreenFrame();
}

static void functEffectSelectDelay()
{
    IE->selectedPlaceSends = 3;
    IE->showSendScreenFrame();
}

static void functEffectSelectOverdrive()
{
    IE->selectedPlaceSends = 0;
    IE->showSendScreenFrame();
}

static void functEffectSelectBitDepth()
{
    IE->selectedPlaceSends = 1;
    IE->showSendScreenFrame();
}

static void functOpenInstrumentSynthesizer()
{
    IE->isEffectsWindow = false;

    IE->clearAllNodes();
    IE->cancelMultiFrame();

    IE->start(3);
}

static void functEffectSavePti()
{
    const uint8_t instrIdx = mtProject.values.lastUsedInstrument;
    if (!isSampleInstrument(instrIdx) || IE->editorInstrument == nullptr ||
        !IE->editorInstrument->isActive || IE->editorInstrument->sample.length == 0)
    {
        mtPopups.show(1, "Instrument slot is empty");
        return;
    }

    sequencer.stopManualNotes();

    std::string defName = "instrument";
    if (IE->editorInstrument->sample.file_name[0] != '\0')
    {
        defName = IE->editorInstrument->sample.file_name;
        if (defName.size() > 4)
        {
            std::string ext = defName.substr(defName.size() - 4);
            if (ext[0] == '.' && (ext[1] == 'p' || ext[1] == 'P') &&
                (ext[2] == 't' || ext[2] == 'T') && (ext[3] == 'i' || ext[3] == 'I'))
            {
                defName.resize(defName.size() - 4);
            }
        }
    }

    GlobalFileBrowserConfig cfg;
    cfg.mode = GlobalFileBrowserConfig::Mode::kSaveGenericFile;
    cfg.title = "Save Instrument (.pti)";
    cfg.initialDirectory = "/Instruments";
    cfg.defaultFileName = defName;
    cfg.extensionFilter = ".pti";
    cfg.maxNameLength = sizeof(strInstrument::strSampleBankSlot::file_name) - 1;
    cfg.contextKey = "save_pti";
    cfg.returnModule = IE;
    cfg.returnModuleOptions = 5;

    cfg.onConfirm = [instrIdx](const std::string& fullPath)
    {
        sequencer.stopManualNotes();
        bool ok = newFileManager.saveInstrumentToPtiFile(instrIdx, fullPath.c_str());
        mtPopups.show(ok ? 0 : 1, ok ? "Instrument saved" : "Error saving .pti");
    };

    openGlobalFileBrowser(cfg);
}

static void functEffectBack()
{
    IE->isEffectsWindow = false;
    IE->setDefaultScreenFunct();
    IE->showInstrumentParams();
}

void cInstrumentEditor::setDefaultVolume()
{
    IE->editorInstrument->volume = defaultInstrumentParams.volume;
    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(VOLUME_MASK);
    }
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    IE->showParamsVolume();
}

void cInstrumentEditor::setDefaultPanning()
{
    IE->editorInstrument->panning = defaultInstrumentParams.panning;
    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(PANNING_MASK);
    }
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    IE->showParamsPanning();
}

void cInstrumentEditor::setDefaultTune()
{
    IE->editorInstrument->tune = defaultInstrumentParams.tune;
    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(TUNE_MASK);
    }
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    IE->showParamsTune();
}

void cInstrumentEditor::setDefaultFinetune()
{
    IE->editorInstrument->fineTune = defaultInstrumentParams.fineTune;
    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(FINETUNE_MASK);
    }
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    IE->showParamsFineTune();
}

void cInstrumentEditor::setDefaultFilterType()
{
    IE->editorInstrument->filterEnable = defaultInstrumentParams.filterEnable;
    IE->editorInstrument->filterType = defaultInstrumentParams.filterType;

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    IE->showFilterType();
}

void cInstrumentEditor::setDefaultCutoff()
{
    if (!IE->editorInstrument->filterEnable)
        return;
    IE->editorInstrument->cutOff = defaultInstrumentParams.cutOff;
    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(CUTOFF_MASK);
    }
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    IE->showFilterCutOff();
}

void cInstrumentEditor::setDefaultResonance()
{
    if (!IE->editorInstrument->filterEnable)
        return;
    IE->editorInstrument->resonance = defaultInstrumentParams.resonance;
    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(RESONANCE_MASK);
    }
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    IE->showFilterResonance();
}

void cInstrumentEditor::setDefaultDelaySend()
{
    IE->editorInstrument->delaySend = defaultInstrumentParams.delaySend;
    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(DELAY_SEND_MASK);
    }
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    IE->showParamsDelaySend();
}

void cInstrumentEditor::setDefaultReverbSend()
{
    IE->editorInstrument->reverbSend = defaultInstrumentParams.reverbSend;
    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(REVERB_SEND_MASK);
    }
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    IE->showParamsReverbSend();
}

void cInstrumentEditor::setDefaultOverdrive()
{
    IE->editorInstrument->overdrive = defaultInstrumentParams.overdrive;
    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(OVERDRIVE_MASK);
    }
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    IE->showParamsOverdrive();
}

void cInstrumentEditor::setDefaultBitDepth()
{
    IE->editorInstrument->bitDepth = defaultInstrumentParams.bitDepth;
    for (uint8_t i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(BIT_DEPTH_MASK);
    }
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    IE->showParamsBitDepth();
}

// env screen
void cInstrumentEditor::setDefaultEnvAttack()
{
    const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
    if (!IE->editorInstrument->isAutomationDestEnabled(IE->selectedEnvelope))
        return;

    IE->editorInstrument->envelope[envSlot].attack =
      defaultInstrumentParams.envelope[envSlot].attack;
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    IE->showEnvAttack();
}

void cInstrumentEditor::setDefaultEnvDecay()
{
    const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
    if (!IE->editorInstrument->isAutomationDestEnabled(IE->selectedEnvelope))
        return;

    IE->editorInstrument->envelope[envSlot].decay =
      defaultInstrumentParams.envelope[envSlot].decay;
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    IE->showEnvDecay();
}

void cInstrumentEditor::setDefaultEnvSustain()
{
    const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
    if (!IE->editorInstrument->isAutomationDestEnabled(IE->selectedEnvelope))
        return;

    IE->editorInstrument->envelope[envSlot].sustain =
      defaultInstrumentParams.envelope[envSlot].sustain;
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    IE->showEnvSustain();
}

void cInstrumentEditor::setDefaultEnvRelease()
{
    const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
    if (!IE->editorInstrument->isAutomationDestEnabled(IE->selectedEnvelope))
        return;

    IE->editorInstrument->envelope[envSlot].release =
      defaultInstrumentParams.envelope[envSlot].release;
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    IE->showEnvRelease();
}

void cInstrumentEditor::setDefaultEnvAmount()
{
    const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
    if (!IE->editorInstrument->isAutomationDestEnabled(IE->selectedEnvelope))
        return;

    IE->editorInstrument->envelope[envSlot].amount =
      defaultInstrumentParams.envelope[envSlot].amount;
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    IE->showEnvAmount();
}

void cInstrumentEditor::setDefaultLfoShape()
{
    const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
    IE->editorInstrument->lfo[envSlot].shape =
      defaultInstrumentParams.lfo[envSlot].shape;
    uint32_t statusByte = 0;
    switch (envSlot)
    {
        case envAmp: statusByte = LFO_AMP_SEND_MASK; break;
        case envCutoff: statusByte = LFO_FILTER_SEND_MASK; break;
        case envWtPos: statusByte = LFO_WT_POS_SEND_MASK; break;
        case envGranPos: statusByte = LFO_GRAN_POS_SEND_MASK; break;
        case envPan: statusByte = LFO_PANNING_SEND_MASK; break;
        case envFinetune: statusByte = LFO_FINETUNE_SEND_MASK; break;
        default: break;
    }
    for (int i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(statusByte);
    }
    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);
    IE->showLfoShape();
}

void cInstrumentEditor::setDefaultLfoSpeed()
{
    const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
    IE->editorInstrument->lfo[envSlot].speed =
      defaultInstrumentParams.lfo[envSlot].speed;

    uint32_t statusByte = 0;
    switch (envSlot)
    {
        case envAmp: statusByte = LFO_AMP_SEND_MASK; break;
        case envCutoff: statusByte = LFO_FILTER_SEND_MASK; break;
        case envWtPos: statusByte = LFO_WT_POS_SEND_MASK; break;
        case envGranPos: statusByte = LFO_GRAN_POS_SEND_MASK; break;
        case envPan: statusByte = LFO_PANNING_SEND_MASK; break;
        case envFinetune: statusByte = LFO_FINETUNE_SEND_MASK; break;
        default: break;
    }
    for (int i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(statusByte);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    IE->showLfoSpeed();
}

void cInstrumentEditor::setDefaultLfoAmount()
{
    const uint8_t envSlot = automationDestToEnvSlot(IE->selectedEnvelope);
    IE->editorInstrument->lfo[envSlot].amount =
      defaultInstrumentParams.lfo[envSlot].amount;

    uint32_t statusByte = 0;
    switch (envSlot)
    {
        case envAmp: statusByte = LFO_AMP_SEND_MASK; break;
        case envCutoff: statusByte = LFO_FILTER_SEND_MASK; break;
        case envWtPos: statusByte = LFO_WT_POS_SEND_MASK; break;
        case envGranPos: statusByte = LFO_GRAN_POS_SEND_MASK; break;
        case envPan: statusByte = LFO_PANNING_SEND_MASK; break;
        case envFinetune: statusByte = LFO_FINETUNE_SEND_MASK; break;
        default: break;
    }
    for (int i = 0; i < 8; i++)
    {
        instrumentPlayer[i].setRefreshBit(statusByte);
    }

    newFileManager.setInstrumentStructChanged(mtProject.values.lastUsedInstrument);

    IE->showLfoAmount();
}

void cInstrumentEditor::setDefaultMidiVelocity()
{
    uint8_t* temp_velocity =
      &mtProject.values
         .midiInstrument[mtProject.values.lastUsedInstrument - kSampleInstrumentsCount]
         .velocity;

    *temp_velocity = DEFAULT_MIDI_VELOCITY;

    IE->showParamsVelocity();

    newFileManager.setProjectStructChanged();
}

void cInstrumentEditor::setDefaultMidiOffset()
{
    uint8_t* temp_offset =
      &mtProject.values
         .midiInstrument[mtProject.values.lastUsedInstrument - kSampleInstrumentsCount]
         .offset;

    *temp_offset = 0;

    IE->showParamsOffset();

    newFileManager.setProjectStructChanged();
}

void cInstrumentEditor::setDefaultMidiCC(uint8_t index)
{
    uint8_t* temp_cc =
      &mtProject.values
         .midiInstrument[mtProject.values.lastUsedInstrument - kSampleInstrumentsCount]
         .ccOut[index];

    switch (index)
    {
        case 0: *temp_cc = MIDI_CC_MODWHEEL; break;
        case 1: *temp_cc = MIDI_CC_GLIDE; break;
        case 2: *temp_cc = MIDI_CC_CHAN_VOLUME; break;
        case 3: *temp_cc = MIDI_CC_EXPRESSION; break;
        case 4: *temp_cc = MIDI_CC_SUSTAIN; break;
        case 5: *temp_cc = MIDI_CC_SPECIAL_PC; break;
        default: return;
    }

    IE->showParamsCC(index);

    newFileManager.setProjectStructChanged();
}

uint8_t cInstrumentEditor::getActualMidiInstrumentCCValue(int ccIndex)
{
    assert(ccIndex < 6);

    if (mtProject.values.lastUsedInstrument < MIDI_INSTRUMENTS_START_IDX ||
        mtProject.values.lastUsedInstrument >= MIDI_INSTRUMENTS_START_IDX + kMidiInstrumentsCount)
    {
        return 0;
    }

    return clamp<int>(
      mtProject.values
        .midiInstrument[mtProject.values.lastUsedInstrument - MIDI_INSTRUMENTS_START_IDX]
        .ccOut[ccIndex],
      0,
      ccNumbersCount - 1);
}

float cInstrumentEditor::calcInstrumentVolumeDecybelValue(uint8_t val)
{
    return 20 * log10f((float)ampLogValues[val]);
}

float cInstrumentEditor::calcInstrumentSendsDecybelValue(uint8_t val)
{
    return 20 * log10f((float)sendsAmpLogValues[val]);
}

static void functMenu(uint8_t state)
{
    if (state == buttonPress)
    {
        mtMenuPopup.showModulePopup(IE);
    }
    else if (state == buttonRelease)
    {
        mtMenuPopup.hideModulePopup(IE);
    }
}

void cInstrumentEditor::addNode(editFunct_t funct, uint8_t nodeNum, uint8_t reverseInput)
{
    if (selectNodes[nodeNum].isActive == 0)
    {
        selectNodes[nodeNum].reverseInput = reverseInput;
        selectNodes[nodeNum].isActive = 1;
        selectNodes[nodeNum].editFunct = funct;
    }
}

void cInstrumentEditor::removeNode(uint8_t nodeNum)
{
    selectNodes[nodeNum].isActive = 0;
    selectNodes[nodeNum].editFunct = NULL;
}

bool cInstrumentEditor::isAnyNodeActive()
{
    for (uint8_t node = 0; node < MAX_SELECT_NODES; node++)
    {
        if (IE->selectNodes[node].isActive == 1)
        {
            return true;
        }
    }

    return false;
}

void cInstrumentEditor::stepThroughNodes(int16_t value,
                                         bool invert) /*source: 1 - przycisk, 0 - enkoder*/
{
    int16_t nodeVal;
    for (uint8_t node = 0; node < MAX_SELECT_NODES; node++)
    {
        nodeVal = value;
        if (selectNodes[node].isActive)
        {
            if (selectNodes[node].editFunct != NULL)
            {
                /*odwracanie wartosci dla list przy wprowadzaniu z przycisku*/
                if (invert == true)
                {
                    if (selectNodes[node].reverseInput == 1)
                    {
                        nodeVal = (-nodeVal);
                    }
                }

                selectNodes[node].editFunct(nodeVal);
            }
        }
    }
}

void cInstrumentEditor::clearAllNodes()
{
    for (uint8_t node = 0; node < MAX_SELECT_NODES; node++)
    {
        selectNodes[node].isActive = 0;
        selectNodes[node].editFunct = NULL;
    }
}

void cInstrumentEditor::cancelMultiFrame()
{
    for (uint8_t i = 0; i < MAX_SELECT_NODES; i++)
    {
        IE->frameData.multisel[i].isActive = 0;
    }

    IE->frameData.multiSelActiveNum = 0;
}

void cInstrumentEditor::setPlayButtonAction()
{
    ip->setButtonObj(interfaceButtonPlay, buttonPress, functPlayAction);
}
