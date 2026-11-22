#ifndef INTERFACE_PERFORMANCEMODE_H_
#define INTERFACE_PERFORMANCEMODE_H_

#include "core/modulesBase.h"

#include "mtStructs.h"

#include "mtAudioEngine.h"

enum mtPerformanceMode
{
    mtPerformanceMaster,
    mtPerformanceFxes,

};

//------------------------------------------------------------------------------

const uint8_t trackMasterModeCount = 2;

const char trackMasterLabels[trackMasterModeCount][5] = {
    "On",
    "Off",
};

// const char * const performanceLfoSpeedLabels[20] =
//{
//		"6",
//		"4",
//		"3",
//		"2",
//		"3/2",
//		"1",
//		"3/4",
//		"1/2",
//		"3/8",
//		"1/3",
//		"1/4",
//		"316",
//		"1/6",
//		"1/8",
//		"/12",
//		"/16",
//		"/24",
//		"/32",
//		"/48",
//		"/64"
//};

class cPerformanceMode : public ModuleBase
{

  public:
    virtual void update();
    virtual void start(uint32_t options);
    virtual void stop();

    virtual void initDisplayControls();
    virtual void destroyDisplayControls() {}
    virtual uint8_t getCurrentModeIdx() { return tracksPage; }
    virtual void hideControlsUnderMiniMenu()
    {
        dontClearPerformanceMode = 1;
        hideFxNames();
    }

    cPerformanceMode() {}
    ~cPerformanceMode() {}

    uint8_t getTrackForButton(uint8_t button) const
    {
        return button + (tracksPage ? kSampleTracks : 0);
    }

    bool isButtonActiveOnCurrentPage(uint8_t button) const
    {
        return getTrackForButton(button) < getProjectMaxTracks();
    }

    //----------------------------------
    void setDefaultScreenFunct();
    void showMuteButtons(uint8_t state);

    void setPerformanceFxes();
    void lightUpPadBoard();
    bool turnOff();
    void turnOffAtEndOfPattern();
    void onMaxTracksChanged(uint8_t maxTracks);
    bool canPerformanceModeContinueAfterSwitchingTo(uint8_t button);

    void toggleTrackPerformanceState(uint8_t track);
    void refreshPerformanceValuesForTrack(uint8_t track, uint8_t place);
    void clearOtherFilterTypes(uint8_t track, uint8_t choosenType);

    uint8_t wasPatternChangedOnTrack(uint8_t track);

    void toggleEditState();
    void setProjectSaveFlags();
    //----------------------------------
    void showDefaultScreen();

    void showPerformanceMaster();
    void showPerformanceFxes();

    void showEditFrame(uint8_t place);
    void hideEditFrame(uint8_t place);

    void showPerformaceValue(uint8_t place);
    void showAllPerformanceValues();
    void showTracksState(uint8_t shiftPressed = 0);
    void showTracksPatterns();
    void showFxNames(uint8_t place);
    void hideFxNames();
    //----------------------------------

    void setFxValue(int value);

    void setPlaceNewFx(uint8_t place, uint8_t newFx);
    uint8_t fxAlredyTaken(uint8_t fx);
    // CC
    void refreshAllCC();
    uint8_t currentCCValueForSlot[12];
    bool isCCToReceiveForSlot[12];

    uint8_t currentCCValueForTrack[kMaxAllTracksInTrackers] = { 0 };
    bool isCCToReceiveForTrack[kMaxAllTracksInTrackers] = { false };

    //----------------------------------

    strMultiLabelData multiLabelData[12];
    strFrameData frameData;

    hControl bgLabel;
    hControl label[8];
    hControl textLabel[12];
    hControl value1Label[12];
    hControl blankSpaceCover = -1;

    hControl frameControl;

    // typ trybu/ekranu
    uint8_t mode = mtPerformanceFxes;
    uint8_t refreshMaster = 1;
    uint8_t refreshTrackState = 1;
    uint8_t refreshTrackPattern = 1;

    uint8_t performanceEditState = 0;
    uint8_t performanceEditPlace = 0;
    uint8_t tracksPage = 0;

    uint8_t dontClearPerformanceMode = 0;

    //--------performance values
    // gui
    char fxValuesText[12][5][7] = { 0 };
    char trackPaternText[8][12]; // do labeli przyciskow pod ekranem

    // funct
    uint8_t trackPatternChange[kMaxAllTracksInTrackers] = { 0 }; // zarzazdanie pattern-trackami

    // blinkowanie na zmienianycnm tracku
    void blinkTrackUntilSwitch();
    void colorTracksLabel(uint8_t track, uint8_t state);

    uint8_t refreshBlinkingTrack = 0;
    uint8_t blinkInterval = 0;
    uint8_t blinkState = 0;
};

extern cPerformanceMode performanceMode;

void cbkTurnOffPerformanceMode(void);
void cbkPerformancePlaybackModeToggle(void);

//#define FX_VALUE(x) PM->fxValues[x][PM->activeFxValues[x]]
//#define FX_VALUE(x) PM->placesTempValues[x]

#endif /* INTERFACE_INSTRUMENTEDITOR_H_ */
