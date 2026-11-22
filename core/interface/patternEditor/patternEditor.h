
#ifndef INTERFACE_PATTERNEDITOR_H_
#define INTERFACE_PATTERNEDITOR_H_

#include "core/modulesBase.h"

#include "mtKeyboardManager.h"
#include "mtSequencer.h"

uint8_t isMultiSelection();
uint8_t isMultiSelectionOnOneLine();
int16_t getActualStep();
int16_t getActualTrack();

uint8_t isEditMode();
void sendSelection();
void sendCopySelection();
void sendPasteSelection();
void changeSelectedStepValue(int16_t value);

class cPatternEditor : public ModuleBase
{

  public:
    virtual void update();
    virtual void start(uint32_t options);
    virtual void stop();

    virtual void initDisplayControls();
    virtual void destroyDisplayControls() {}

    virtual void hideControlsUnderMiniMenu();

    void refreshLastUsedInstrument();

    cPatternEditor();
    virtual ~cPatternEditor() {}

    //----------------------------------

    void showDefaultScreen();
    void setDefaultScreenFunct();
    void setSwitchModuleFunct();

    void setNotePopupFunct();
    void setInstrPopupFunct();
    void setVolPopupFunct();
    void setFxListPopupFunct();

    void setKeyboardExportFunctions();
    void setOverwriteRenderFunct();

    void setKeyboardNamePatternFunctions(void (*confirmFunct)(void), bool autoNameEnabled);

    void focusOnActual();
    void moveCursorByStep();
    void moveCursorByStep(uint8_t);
    void moveCursorByStepParallel();

    void refreshPatternFromOutsideModule();
    void refreshPattern();
    void onMaxTracksChanged(uint8_t maxTracks);
    void readPatternState();
    void setPlayheadRangeFromSelection();

    void refreshEditStateFromOutsideModule();
    void refreshEditState();

    void focusOnPattern();
    void unfocusPattern();

    void changeActualTempo(int16_t value);
    void changeActualPattern(int16_t value);
    void setActualPattern(int16_t value);
    void changeActualPatternLength(int16_t value);
    void changeActualPatternLengthByDefinedValues(int16_t value);
    void setActualPatternLength(int16_t value);
    void changeActualPatternEditStep(int16_t value);
    void changeActualPatternEditStepByDefinedValues(int16_t value);
    void setActualPatternEditStep(int16_t value);

    void changeFillData(int16_t value, bool rollListOver = false);
    void changeRandomiseData(int16_t value);
    void setFillData(uint16_t value);
    void setRandomiseData(uint16_t value);

    void changneFillDataByPad(uint8_t pad);
    void changneRandomiseDataByPad(uint8_t pad);

    void setStepButtonState(uint8_t button, uint8_t state);
    void showNotePopup();
    void showInstrumentPopup();
    void showFx1Popup();
    void showFx2Popup();

    void loadPatternsStatus();
    void lightUpPadBoard();
    void clearPadBoard();

    void showMuteButtons(uint8_t state, bool midiTracks);
    void showRecordArmingButtons(uint8_t state, bool midiTracks);
    void setMuteFunct(uint8_t state);
    void setRecArmingFunct(uint8_t state);
    void setProjectSaveFlags();

    void previousTrack();
    void nextTrack();
    void previousStep();
    void nextStep();
    void changeActualStep(uint8_t direction);
    //----------------------------------

    void activateLabelsBorder();
    void activateFillPopupBorder();
    void activateRandomisePopupBorder();
    void changeFillDataByActionButton(uint8_t place);
    bool isFillPlaceValid(uint8_t place);
    void setFillPlace(uint8_t place, int8_t dir = 0);
    void changeFillPlace(int8_t diff);
    void refreshFillPlace();

    void showPattern(uint8_t forceShow = 0, uint8_t shiftPressed = 0);
    void showLength(uint8_t forceShow = 0);
    void showStep();

    void refreshPatternParams();
    void showFxInfo();

    void showEditModeLabels();
    void hideEditModeLabels();

    void fillInstrumentNames();

    void showFillNote();
    void showFillInstr();
    void showFillFx();
    void showFillMicVol();

    void showFillPopup();
    void refreshFillType();
    void refreshFillFrom();
    void refreshFillTo();
    void refreshFillParam();
    void refreshFillStep();
    void refreshFillDensity();
    void hideFillPopup();

    void activateGreenSelection();
    void activateRedSelection();
    void deactivateSelection();
    void playheadRecMode();
    void playheadNormalMode();

    void startExportSelection();
    void endExportSelection();

    void showKeyboardExport();
    void showKeyboardNameEdit(const char* title, bool autoNameEnabled);
    void hideKeyboardExport();
    void showOverwriteExportDialog();
    void hideOverwriteExportDialog();
    void showExportProgress();
    void refreshExportProgress();
    void hideExportProgress();
    void showFullMemoryInBank();
    void showFullInstrumentInBank();

    //----------------------------------
    void exportSelection();
    void setClockWarningScreenFunct();
    void showClockWarning();
    void setPreviousUsedFx(uint8_t newValue);
    uint8_t getPreviousUsedFx();
    void setPreviousUsedFxValue(uint8_t newValue);
    uint8_t getPreviousUsedFxValue();

    uint8_t saveClockConfig;
    //----------------------------------
    // Funct
    uint16_t lastPatternPosition;
    uint8_t refreshPlayheadOnScreen();

    uint16_t lastShownPatternOnTitleBar = 0;
    uint16_t lastShownPattern = 0;
    uint16_t lastShownPattern_blinkState = 0;

    uint16_t lastLength = 0;

    uint8_t isCursorInSelection();
    bool isSelectionOrCursorInMidiTracks();

    Sequencer::strPattern* seq;

    static strTrackerPattern trackerPattern;

    uint8_t editMode = 0;
    enum EditParam
    {
        EditNote = 0,
        EditInstr,
        EditFx1,
        EditFx2
    };

    enum PatternArrangement
    {
        kVerticalRevolver = 0,
        kVerticalOriginal = 1,
        kHorizontal = 2,

        kPatternArrangementCount = 3,
    };

    static constexpr uint16_t kStepNotePopupShowDelay = 500;
    static constexpr uint16_t kStepPopupShowDelay = 180;

    uint8_t editParam = 0; // 0 note, 1 instrument, 2 fx1, 3 fx 2
    uint8_t previousEditParam = 0;
    uint8_t previousFxValue = 0;
    uint8_t isSelectingNow = 0;

    uint8_t padsTempData[48];

    uint8_t currentExportState;
    uint8_t lastExportState;
    bool isLoadAfterSave;
    int8_t firstFreeInstrumentSlotFound;

    uint16_t lastPatternNumber =
      0xFFF; // do weryfikowania dezaktywacji zaznaczenia w przypadku zmiany patternu
    uint16_t lastPatternLength = 0xFFF;

    //----------------------------------
    // GUI

    hControl label[8];
    hControl bgLabel;
    hControl patternControl;
    hControl arrangementControl;
    hControl frameControl;
    hControl verticalPatternControl;
    hControl horizontalPatternControl;

    // popups
    hControl patternPopupLabel;
    hControl patternPopupTitleLabel;
    hControl param1PopupListControl;
    hControl val1PopupBar;
    hControl val2PopupBar;
    hControl val3PopupBar;
    hControl valCancelEmptyPopupBar;
    hControl valFillEmptyPopupBar;
    hControl intstrumentsFromPopupListControl;
    hControl intstrumentsToPopupListControl;
    hControl param3PopupListControl;
    hControl param2PopupListControl;

    strLabelData labelDataRed;
    strLabelData labelDataGreen;
    strLabelData labelDataMore;

    strFrameData frameData;

    int8_t selectedPlace = -1; // przytrzymany przycisk pod ekranem
    bool undoRedoOpen = 0;     // true jesli operujemy jogiem na undoRedo

    // parametry
    char cTempo[7];
    char cPattern[5];
    char cLength[5];
    char cStep[5];
    char cTitlePattern[PATTERN_NAME_LENGTH + 1];

    bool refreshAfterReleaseFx;

    uint64_t patternOccupyMap;

    //------------------------------------------------------
    // fill
    uint8_t fillState = 0;
    uint8_t undoRedoButtonReleaseBlocker =
      0; // block release after return to patternEditor default view

    int8_t fillPlace = 0;
    //
    strList fillTypeList;
    //
    strList fillScaleFilterList;
    //
    strList fillFxTypeList;
    //
    strList fillEmptyList;
    //
    strList fillStepList;
    //
    strList fillInstrumentsFromList;
    strList fillInstrumentsToList;
    //
    static constexpr int fillTextLen = 10;
    char fillText1[fillTextLen];
    char fillText2[fillTextLen];
    char fillTextDensity[10];
    char fxTypeText[2];
    //
    struct strFill
    {
        int16_t fillStep;
        int16_t type;
        int16_t from;
        int16_t to;
        int16_t param;
        int16_t density[Sequencer::fillStepEnd__];
    };
    static strFill fillData[6];
    uint8_t getFillDataIdx() const
    {
        return (isMicVolView && editParam >= EditFx1) ? (editParam + 2) : editParam;
    }
    void initExternalRamState();

    bool isMicVolView = false;
    bool toggledMicVolOnPress = false;

    // int16_t fillStep = 1;

    //-------------------------------------------------------
    // popups
    void cancelPopups();

    uint8_t getStepFx();
    int8_t getStepVol();

    uint8_t dontShowPopupsUntilButtonRelease = 0;
    uint8_t insertOnPopupHideDisabled = 0;

    uint8_t wasNotesEditBefore = 0;

    uint8_t stepButtonsState[4] = { 0, 0, 0, 0 };
    elapsedMillis stepButtonsTimer[4];

    //-------------------------------------------------------
    // pojedyncze parametry / 8 trackow
    uint8_t patternViewMode = 0; // przechowuje bitowo pokazywane parametry dla 8 trackow
    uint8_t patternViewChanged = 0;
    uint8_t selectedParams[2];
    uint8_t patternButtonReleaseActive = 0;

    void setPatternViewMode(uint8_t param);
    void changePatternViewMode(uint8_t param);

    //--------------------------------------------------------
    // powiekszona funkcjolanosc przyciskow pod ekranem - 2 strony
    const uint8_t actionButtonsPagesCount = 3;
    uint8_t actualActionButtonsPage = 0;

    void setActionButtonPage1Funct();
    void setActionButtonPage2Funct();
    void setActionButtonPage3Funct();
    void refreshActualActionButtonPage();

    void showActionButtonPage1Labels();
    void showActionButtonPage2Labels();
    void showActionButtonPage3Labels();

    //--------------------------------------------------------
    // title bar
    void showTitleBar();
    void showTitleBarPattern();
    void showActualParamOnBar(bool forceShow = false);
    void hideActualParamOnBar();

    uint8_t lastTitleBarParamValue = 0xFF;
    uint8_t lastTitleBarFx = 0xFF;
    uint8_t lastTitleBarParam = 0xFF;
    char rightTitleBarLabel[40];
};

extern cPatternEditor patternEditor;

#endif /* INTERFACE_PATTERNEDITOR_H_ */
