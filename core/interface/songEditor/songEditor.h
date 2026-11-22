#ifndef INTERFACE_SONGEDITOR_H_
#define INTERFACE_SONGEDITOR_H_

#include "core/modulesBase.h"

#include "mtStructs.h"

typedef enum
{
    iconPlay,
    iconLoop,
    iconPerformance,
} icon_t;

typedef enum
{
    playerUp,
    playerDown,
    playerLeft,
    playerRight,
    playerHome,
} player_direction_t;

typedef enum
{
    selDirNone = 0x00U,
    selDirLeft = 0x01U,
    selDirRight = 0x02U,
    selDirUp = 0x04U,
    selDirDown = 0x08U,

} select_direction_t;

class cSongEditor : public ModuleBase
{

  public:
    virtual void update();
    virtual void start(uint32_t options);
    virtual void stop();

    virtual void initDisplayControls();
    virtual void destroyDisplayControls() {}

    cSongEditor() {}
    virtual ~cSongEditor() {}

    void showDefaultScreen();

    void activateLabelsBorder();

    void showTempoValue();
    void refreshTempoField();
    void refreshAddSlotLabel(bool isButtonPressed);

    localList_t patternsList;

    strFrameData frameData;
    strLabelData doubleArrowRed;
    strLabelData labelDataRed;

    hControl label[8] = { -1 };
    hControl barControl = -1;
    hControl songPlayerControl = -1;

    hControl frameControl = -1;
    hControl bgLabel;

    uint8_t lastSelectedPlace = 1;
    uint8_t selectedPlace = 0;

    void setDefaultScreenFunct();

    void changeSongPositionSelection(int16_t value);
    void changeSelectedPlayListPattern(int value);

    void readSong();

    void changeGlobalTempo(float value);
    void setDefaultGlobalTempo();

    //--------------------------------------------------------------

    void refreshButtonsLabels();

    uint8_t playButtonsState;

    //--------------------------------------------------------------
    // patterns
    void refreshPatternsList();
    void cleanPatternsListResources();

    uint16_t songLength = 1;
    uint8_t selectedSongPosition = 0;

    //--------------------------------------------------------------
    //  global params
    char globalTempoVal[10];
    char patternLengthVal[6];

    //--------------------------------------------------------------

    void showIcon(icon_t iconType, uint8_t position);
    void hideIcon();
    void handleEntryIcon();
    //--------------------------------------------------------------

    void switchToNewPattern();

    uint8_t exitOnButtonRelease;
    bool pressedSongButton = false;

    player_data_t songPlayerData;
    uint32_t* patternBitmask;

    void refreshSongPlayerControl();
    void refreshSongPlayerSelectedSlot();
    void startSongPlayer();
    void stopSongPlayer();
    void refreshSongPlayer(uint8_t source);
    void onMaxTracksChanged(uint8_t maxTracks);

    void stopPlayingSelection();

    void walkOnSongPlayer(player_direction_t dir);
    void selectSpecificTrack(uint8_t track, uint8_t songPosition);
    void selectSongPosition(uint8_t songPosition);

    elapsedMillis songPlayerRefreshTimer;

    song_selection_t copyCurrentData;
    uint8_t isSomethingCopied = 0;
    uint8_t isCopyingInProgress = 0;
    bool clearSelectionAfterCopying = false;
    uint8_t copyElementMax;
    uint8_t currentCopyElement;
    uint8_t currSelDirection;
    uint8_t currSelDirection2;

    uint8_t isBusy = 0;

    uint8_t isDeletingInProgress = 0;
    uint8_t deleteElementMax;
    uint8_t currentDeleteElement;

    void tapTempo_tap();
    void tapTempo_update();
    void tapTempo_end();
    bool tapTempo_still_going();
    void tapTempo_set(float value);

    // GREEN/RED EDIT MODE
    uint8_t editMode;
    void refreshEditMode();
    int16_t lastSongListPos = -1;
    void lightUpPadBoard();

  private:
    uint8_t tapTempoCount = 0;
    char tapText[12] = { 0 };
    bool tapTempoActive = 0;
    float tapTempoSum = 0.0;
    elapsedMillis tapTempoTimer = 0;

    char** patternNames = nullptr;
    static constexpr char kEmptyPatternNameText[2] = "";
};

extern cSongEditor songEditor;

void cbkSongPlaybackModeToggle(void);

#endif /* INTERFACE_SONGEDITOR_H_ */
