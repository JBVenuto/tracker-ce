#ifndef INTERFACE_MASTERPARAMS_H_
#define INTERFACE_MASTERPARAMS_H_

#include "core/modulesBase.h"
#include "mtKeyboardManager.h"

#include "mtStructs.h"

#undef MAX_SELECT_NODES
#define MAX_SELECT_NODES 7

class cMasterParams : public ModuleBase
{

  public:
    virtual void update();
    virtual void start(uint32_t options);
    virtual void stop();

    virtual void initDisplayControls();
    virtual void destroyDisplayControls() {}

    virtual void hideControlsUnderMiniMenu();

    virtual uint8_t getCurrentModeIdx();

    cMasterParams()
    {
        //		label[8];
        //		barControl[8];
        //		editorInstrument;
        //		frameControl;
        //
        //		delayPingpongEnableList;
        //		delaySyncEnableList;
        //		delaySyncRateList;
    }
    virtual ~cMasterParams() {}

    //-----------------------------------------
    struct strTrackLevel;
    void showDefaultConfigScreen();
    void showMasterScreen();

    void changeSelectionInGroup(int16_t value, uint8_t groupNum);
    void shiftHandler();
    // master
    void showVolume();
    void showBassBoost();
    void showStereoBoost();
    void showSaturationType();
    // delay
    void showDelayScreen();
    void showDelayPingPongEnable();
    void showDelaySyncEnable();
    void showDelayRate();
    void showDelayFeedback();
    void showDelayTime();
    void showDelayFilterFrequency();
    // reverb
    void showReverbScreen();
    void showReverbSize();
    void showReverbDamp();
    void showReverbPredelay();
    void showReverbDiffusion();
    // limiter
    void showLimiterScreen();
    void showLimiterAttack();
    void showLimiterRelease();
    void showLimiterThreshold();
    void showLimiterGainReduce();
    void showLimiterSidechain();
    void rebuildLimiterSidechainList();
    uint8_t getEffectiveLimiterSidechain() const;
    uint8_t getLimiterSidechainVisibleIdx(uint8_t sidechainVal) const;
    void onMaxTracksChanged(uint8_t maxTracks);
    // EQ
    void showEqualizerScreen();
    void showEqualizerBand(uint8_t n);

    enum displayMode
    {
        masterValues,
        mixer,
        mixerDelayReverb
    };

    displayMode displayType = displayMode::masterValues;
    displayMode displayTypeBeforeMasterPress = displayMode::masterValues;

    enum
    {
        delayVolumeScreenPosition,
        reverbVolumeScreenPosition,
        dryMixVolumeScreenPosition,
        lineInVolumeScreenPosition,
#ifdef FEATURE_SYNTH
        synthVolumeScreenPositionStart,
        synthVolumeScreenPositionEnd = synthVolumeScreenPositionStart + kSynthParts - 1,
#endif
        screenPositionCount
    };

    static_assert(screenPositionCount <= 8);

    enum
    {
        delayIsSolo = 10,
        reverbIsSolo,
        dryMixIsSolo,
        lineInIsSolo,
    };

    uint8_t selectedPlaceDelay;
    uint8_t selectedPlaceReverb;
    uint8_t selectedPlaceLimiter;
    uint8_t selectedPlaceEqualizer;

    bool isDelayScreen;
    bool isDelayScreenBeforeMasterPress;
    bool isReverbScreen;
    bool isReverbScreenBeforeMasterPress;
    bool isLimiterScreen;
    bool isLimiterScreenBeforeMasterPress;
    bool isEqualizerScreen;
    bool isEqualizerScreenBeforeMasterPress;
    uint8_t selectedPlaceBeforeMasterPress;
    bool showMasterVolumeOnHold;
    // mixer
    void showMixerScreen();
    void showMixerDelayReverbScreen();
    void showLevelBar(uint8_t n, strTrackLevel* level);
    void showTrackVolumeBar(uint8_t n);
    void showDelayVolumeBar();
    void showReverbVolumeBar();
    void showDryMixVolumeBar();
    void showLineInVolumeBar();
#ifdef FEATURE_SYNTH
    void showSynthGain(int synthIdx);
#endif
    void showLineInDelaySend();
    void showLineInReverbSend();
    void switchScreen();
    void returnToScreen();
    void switchToMaster();
    void switchToMixer();
    void switchToMixerDelayReverb();
    void switchToDelayScreen();
    void switchToReverbScreen();
    void switchToLimiterScreen();
    void switchToEqualizerScreen();
    void calcLevel(strTrackLevel* level, float value);
    void resetLevel(strTrackLevel* level);
    void resetMixerLevels();
    void resetMixerDelayReverbLevels();
    void calcTrackLevel(uint8_t n);
    void calcDelayLevel();
    void calcReverbLevel();
    void calcDryMixLevel();
    void calcLineInLevel();
#ifdef FEATURE_SYNTH
    void calcSynthLevels();
#endif

    void gainReduceHandler();

    float calcLevelDecybelValue(uint8_t val);

    struct strTrackLevel
    {
        float measureSum = 0;
        uint8_t measureCounter = 0;
        elapsedMicros timer = 0;
        elapsedMillis redColorTimer = 0;
        uint8_t value;
        uint8_t lastValue;

    } trackLevel[8], reverbLevel[2], delayLevel[2], dryMixLevel[2], lineInLevel[2];
#ifdef FEATURE_SYNTH
    strTrackLevel synthLevel[kSynthParts];
#endif

    struct strGainReduction
    {
        float measureSum = 0;
        uint8_t measureCounter = 0;
        float value;
        float lastValue;
    } gainReduce;

    uint8_t isSolo = 0;
    int8_t soloTrack = -1;
    char mixerLabel[8][7];
    char mixerVolumeLabel[8][12];
    char delayVolumeLabel[12];
    char reverbVolumeLabel[12];
    char dryMixVolumeLabel[12];
    char lineInVolumeLabel[12];
    char lineInDelaySendLabel[12];
    char lineInReverbSendLabel[12];
#ifdef FEATURE_SYNTH
    char synthVolumeLabel[kSynthParts][12];
    char synthVoiceLabel[kSynthParts][10];
#endif

    bool trackIsEdited[8];
    bool ignoreMuteRelease[8];

    bool reverbIsEdited;
    bool ignoreMuteReleaseReverb;
    bool delayIsEdited;
    bool ignoreMuteReleaseDelay;
    bool dryMixIsEdited;
    bool ignoreMuteReleaseDryMix;
    bool lineInIsEdited;
    bool ignoreMuteReleaseLineIn;
#ifdef FEATURE_SYNTH
    bool synthGainIsEdited[kSynthParts];
#endif

    // menu

    //

    //-----------------------------------------

    void setMasterScreenFunct();
    void setDelayScreenFunct();
    // config
    void changeConfigGroupSelection(int16_t value);

    /*	//master
        void changeVolume(int16_t value);
        void changeReverbRoomSize(int16_t value);
        void changeReverbDamping(int16_t value);
        void changeLimiterAttack(int16_t value);
        void changeLimiterRelease(int16_t value);
        void changeLimiterTreshold(int16_t value);
        void changeBitDepth(int16_t value);*/
    // delay

    void changeDelayPingPongEnable(int16_t val, bool rollListOver = false);
    void changeDelaySyncEnable(int16_t val, bool rollListOver = false);
    void changeDelayRate(int16_t val, bool rollListOver = false);
    void changeDelayTime(int16_t val);
    void changeDelayFilterFrequency(int16_t val);
    void changeDelayFeedback(int16_t val);

    void setDefaultDelayPingPongEnable();
    void setDefaultDelaySyncEnable();
    void setDefaultDelayRate();
    void setDefaultDelayTime();
    void setDefaultDelayFilterFrequency();
    void setDefaultDelayFeedback();

    // reverb
    void changeReverbSize(int16_t val);
    void changeReverbDamp(int16_t val);
    void changeReverbPredelay(int16_t val);
    void changeReverbDiffusion(int16_t val);
    // equalizer
    void changeEqualizerBandAdjust(uint8_t n, int16_t val);

    // Mixer
    void changeTrackVolume(int16_t val, uint8_t track);
    void setDefaultTrackVolume(uint8_t track);
    // Mixer Delay Reverb
    void changeReverbVolume(int16_t val);
    void changeDelayVolume(int16_t val);
    void changeDryMixVolume(int16_t val);
    void changeLineInVolume(int16_t val);
    void changeLineInDelaySend(int16_t val);
    void changeLineInReverbSend(int16_t val);
#ifdef FEATURE_SYNTH
    void changeSynthGain(int synthIdx, int16_t val);
    void setDefaultSynthGain(int synthIdx);
#endif
    void setDefaultReverbVolume();
    void setDefaultDelayVolume();
    void setDefaultDryMixVolume();
    void setDefaultLineInVolume();
    void setDefaultLineInDelaySend();
    void setDefaultLineInReverbSend();

    void setDefaultReverbSize();
    void setDefaultReverbDamp();
    void setDefaultReverbPredelay();
    void setDefaultReverbDiffusion();

    // equalizer
    void setDefaultEqualizerBandAdjust(uint8_t n);

    void activateLabelsBorder();
    void refreshDelayFrame();
    void refreshReverbFrame();
    void refreshLimiterFrame();
    void refreshEqualizerFrame();

    void lightUpPadBoard();

    uint8_t processUpdate = 0;

    //----------------------------------

    strFrameData frameData;

    strList pingpongEnableData;
    strList syncEnableData;
    strList syncRateData;
    strList limiterSidechainData;

#ifdef FEATURE_SYNTH
    strBarData barData[8 + kSynthParts];
    uint32_t levelBarColors[8 + kSynthParts][5] = {
        { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }
    };
#else
    strBarData barData[8];
    uint32_t levelBarColors[8][5] = {
        { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 },
    };
#endif
    strList saturationListData;

    const char* const delayPingPongEnable[2] = { (char*)"On", (char*)"Off" };

    const char* const delaySyncEnable[2] = { (char*)"On", (char*)"Off" };

    const char* const delaySyncRates[19] = {
        (char*)"8",    (char*)"6",    (char*)"4",    (char*)"3",    (char*)"2",
        (char*)"3/2",  (char*)"1",    (char*)"3/4",  (char*)"1/2",  (char*)"3/8",
        (char*)"1/3",  (char*)"1/4",  (char*)"3/16", (char*)"1/6",  (char*)"1/8",
        (char*)"1/12", (char*)"1/16", (char*)"1/24", (char*)"1/32",
    };
#ifdef FEATURE_SYNTH
    const char* const limiterSidechainListTxt[11 + kSynthParts] = {
        (char*)"Disable",
        (char*)&mtProject.values.TrackNames[0],
        (char*)&mtProject.values.TrackNames[1],
        (char*)&mtProject.values.TrackNames[2],
        (char*)&mtProject.values.TrackNames[3],
        (char*)&mtProject.values.TrackNames[4],
        (char*)&mtProject.values.TrackNames[5],
        (char*)&mtProject.values.TrackNames[6],
        (char*)&mtProject.values.TrackNames[7],
        (char*)"Line in L",
        (char*)"Line in R",
        (char*)"Synth 1",
        (char*)"Synth 2",
        (char*)"Synth 3"
    };
#else
    const char* const limiterSidechainListTxt[11] = { (char*)"Disable",
                                                      (char*)&mtProject.values.TrackNames[0],
                                                      (char*)&mtProject.values.TrackNames[1],
                                                      (char*)&mtProject.values.TrackNames[2],
                                                      (char*)&mtProject.values.TrackNames[3],
                                                      (char*)&mtProject.values.TrackNames[4],
                                                      (char*)&mtProject.values.TrackNames[5],
                                                      (char*)&mtProject.values.TrackNames[6],
                                                      (char*)&mtProject.values.TrackNames[7],
                                                      (char*)"Line in L",
                                                      (char*)"Line in R" };

#endif

    static constexpr uint8_t kMaxLimiterSidechainItems = LIMITER_SIDECHAIN_MAX + 1;
    const char* limiterSidechainVisibleTexts[kMaxLimiterSidechainItems] = {};
    uint8_t limiterSidechainVisibleValues[kMaxLimiterSidechainItems] = {};
    uint8_t limiterSidechainVisibleCount = kMaxLimiterSidechainItems;

    hControl label[8];
#ifdef FEATURE_SYNTH
    hControl barControl[8 + kSynthParts];
#else
    hControl barControl[8];
#endif
    hControl blankSpaceCover[3];

    hControl bgLabel;
    hControl frameControl;

    hControl delayPingpongEnableList;
    hControl delaySyncEnableList;
    hControl delaySyncRateList;
    hControl trackNameLabel[8];
    hControl trackVolumeBar[8];
    hControl limiterSidechainList;
    hControl saturationList;

    uint8_t selectedPlace = 0;

    uint8_t sourceModuleButton = 0;

    // typ trybu/ekranu
    uint8_t mode = 0;

    strInstrument* editorInstrument;

    uint8_t editTrackNameMode = 0;
    uint8_t actualEditingTrackName = 0;
    //----------------------------------
    //----------------------------------

    char volumeVal[12];
    char delayFeedbackVal[4];
    char delayTimeVal[9];
    char delayFilterFreqVal[9];
    char bassBoostVal[4];
    char stereoBoostVal[4];

    char reverbSizeVal[4];
    char reverbDampVal[4];
    char reverbPredelayVal[4];
    char reverbDiffusionVal[4];

    char limitAttackVal[8];
    char limitReleaseVal[8];
    char limitThresholdVal[12];
    char gainReduceVal[12];

    char eqBandAdjustment[5][12];

    //----------------------------------

    uint8_t getListsActive();
    void setDataForLists(uint8_t listNum, uint8_t max);
    void listDataForLists(uint8_t listNum, uint8_t nameNum, const char* names);

    void processChangeInGroup0();
    void processChangeInGroup1();
    void processChangeInGroup2();
    void processChangeInGroup3();
    void chainProcessChange(uint8_t groupNum, uint8_t forceProcess);

    void resizeToDefaultConfig();
    void resizeToDefaultMaster();
    void resizeBarControl(displayMode val);
    void resizeToSmallConfig(uint8_t labelIdx);

    void showGeneralSubmenus(uint8_t listPosition);
    void showFirmwares(uint8_t listPosition);
    void showMidiSubmenus(uint8_t listPosition);
    void showFirmwareFlashButton();

    void showPatternDividers(uint8_t listPosition);
    void showRadioRegions(uint8_t listPosition);
    void showBrightnessLevels(uint8_t listPosition);

    void showClockIn(uint8_t listPosition);
    void showClockOut(uint8_t listPosition);
    void showTransportIn(uint8_t listPosition);
    void showTransportOut(uint8_t listPosition);
    void showCCouts(uint8_t listPosition);
    void showCCnumber(uint8_t listPosition);
    void showMidiNotesFrom(uint8_t listPosition);
    void showMidiNotesChannels(uint8_t listPosition);

    void changeLabelText(uint8_t labelIdx, const char* text);

    // nzawy trackow
    void editTrackName(uint8_t button);
    void setKeyboardExportFunctions();
    void showKeyboardExport();
    void showEditTracksNamesMode();

    float localDelayLevel[2];
    float localReverbLevel[2];
    float localLineInLevel[2];
    float localDryMixLevel[2];
#ifdef FEATURE_SYNTH
    float localSynthVoiceLevel[kSynthParts];
#endif

    bool refreshVolumeFromCC[12];
    void refreshAllTracksMixerVolume();
    void refreshOtherMixerVolume();

    void hideControlUnderMenu();
};

extern cMasterParams masterParams;

#endif /* INTERFACE_MASTERPARAMS_H_ */
