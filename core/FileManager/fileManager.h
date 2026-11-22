
#ifndef FILEMANAGER_FILEMANAGER_H_
#define FILEMANAGER_FILEMANAGER_H_

#include "Platform.h"
#include "mtSequencer.h"
#include "mtStructs.h"
#include "polyassert.h"

#include <atomic>
#include <optional>
#include <stdint.h>

#include "InstrumentCopy.h"
#include "InstrumentDelete.h"
#include "ProjectOpen.h"
#include "ProjectSave.h"
#include "RecordingSave.h"
#include "SampleImport.h"
#include "SampleMemoryReload.h"
#include "SampleMemorySave.h"
#include "WorkspaceProjectLoad.h"
#include "WorkspaceProjectSave.h"

#include "PatternsMetadata.h"

#ifdef FEATURE_SYNTH
#define FILEMANAGER_HANDLE_SYNTH 1
#else
#define FILEMANAGER_HANDLE_SYNTH 0
#endif

//#define FILEMANAGER_DEBUGLOG 1

constexpr uint32_t targetSampleRate = 44100;

enum fileManagerStatus
{
    fmIdle = 0,
    fmBrowsingSamples,
    fmBrowsingFirmwares,
    fmBrowsingGames,
    fmImportingSamplesToWorkspace,
    fmCopyingInstrumentsInWorkspace,
    fmDeleteingInstruments,
    fmPreviewSampleFromSd,
    fmLoadingProjectfromWorkspace,
    fmLoadingProjectFromProjects,
    fmImportingMod,
    fmExportingMod,
    fmSavingProjectToWorkspace,
    fmSavingProjectToProjects,
    fmLoadingPatternFromWorkspace,
    fmExportingSoundSong,
    fmExportingSoundSongStems,
    fmExportingSoundPattern,
    fmExportingSoundPatternStems,
    fmExportingSoundRenderSelection,
    fmExportingProjectSamples,
    fmSavingRecordedSound,
    fmSavingImportingRecordedSound,
    fmImportingSampleFromSampleEditor,
    fmReloadingSamples,
    fmDrawingSampleWaveform,

    fmLoadEnd,
    fmSaveEnd,
    fmBrowseSamplesEnd,
    fmBrowseFirmwaresEnd,
    fmBrowseGamesEnd,
    fmImportSamplesEnd,
    fmCopyingInstrumentsEnd,
    fmDeleteInstrumentsEnd,
    fmLoadPatternEnd,
    fmExportSoundEnd,
    fmExportModEnd,
    fmSaveRecordedSoundEnd,
    fmImportSampleFromSampleEditorEnd,
    fmReloadSamplesEnd,
    fmDrawingSampleWaveformEnd,

    fmError,
    fmLoadError,
    fmImportModError,
    fmExportModError,
    fmSaveError,
    fmCopyError,
    fmBrowseSamplesError,
    fmBrowseFirmwaresError,
    fmBrowseGamesError,
    fmImportSamplesError,
    fmCopyingInstrumentsError,
    fmDeleteInstrumentsError,
    fmLoadPatternError,
    fmExportSoundError,
    fmSaveRecordedError,
    fmImportSampleFromSampleEditorError,
    fmReloadSamplesError,
    fmDrawingSampleWaveformError,

};

enum fileManagerOperation
{
    fmNoOperation = 0,
    fmLoadWorkspaceProject,    // 1
    fmSaveWorkspaceProject,    // 2
    fmOpenProject,             // 3
    fmCopyWorkspaceToProjects, // 4

    fmBrowseSamples,   // 5
    fmBrowseProjects,  // 6
    fmBrowseMods,      // 7
    fmBrowseFirmwares, // 8
    fmBrowseGames,     // 8.5 ;>

    fmImportSamplesToWorkspace,   // 8.55 :>
    fmCopyInstrumentsInWorkspace, // 9
    fmPreviewSamplesFromSD,       // 10
    fmDeleteInstruments,          // 11

    fmLoadWorkspacePattern, // 12

    fmExportSound,          // 13
    fmExportProjectSamples, // 14
    fmSaveRecordedSound,    // 15

    fmImportSampleFromSampleEditor, // 16

    fmImportModFile, //
    fmExportModFile, //

    fmReloadSamples,

    fmDrawSampleWaveform,
};

enum enImportModMode
{
    importModFiletype_notSupported,
    importModFiletype_mod,
    importModFiletype_it,
};

struct strProjectFile;
struct strInstrumentFile;
class SdDir;

extern SdDir sdLocation;
extern FileSystem::FileHandle wavfile;
class CrcCalculator;
class Synth;

class cFileManager
{
  public:
    void init(FileSystem* fileSystem, Synth* synth, bool stereoSampleMode);
    void update();
    void setCrcCalculator(CrcCalculator* crcCalculator);
    //-------------------------------------------------
    inline uint8_t getStatus() { return status; }
    void setStatus(uint8_t status) { this->status = status; }

    uint8_t getCurrentOperation() { return currentOperation; }
    void setCurrentOperation(uint8_t operation) { currentOperation = operation; }
    void resetCurrentOperationStep() { currentOperationStep = 0; }

    uint8_t getProgress();
    FileSystem* getFileSystem() { return fileSystem; }
    Synth* getSynth() { return synth; }
    PatternsMetadata* getPatternsMetadata() { return &patternsMetadata; }

    void clearStatus()
    {
        if (status >= fmLoadEnd)
            status = fmIdle;
    }

    // flagi zmian
    void clearChangeFlags();
    bool isProjectChanged();
    void setAllChangeFlags();

    void setActualPatternStructChanged();
    void setPatternStructChanged(uint8_t pattern);
    bool isPatternStructChanged(uint8_t pattern);
    void clearPatternChanged(uint8_t pattern);

    void setInstrumentStructChanged(uint8_t instrument);
    void setInstrumentSampleChanged(uint8_t instrument);
    bool isInstrumentStructChanged(uint8_t instrument);
    bool isInstrumentSampleChanged(uint8_t instrument);
    void clearInstrumentChanged(uint8_t instrument);
    void clearSampleChanged(uint8_t instrument);

    bool isProjectStructChanged();
    void setProjectStructChanged();
    void clearProjectChangedFlag();

    // getery
    bool projectExist(char* path, char* name);
    uint8_t getBrowsedFilesList(char*** list, uint32_t** memoryList);
    uint8_t getProjectsList(char*** list);
    uint8_t getModsList(char*** list);
    uint8_t getFirmwaresList(char*** list);
    uint8_t getGamesList(char*** list);

    uint8_t getCurrentPatternIndex(); // want to load
    uint8_t getLoadedPatternIndex();  // finished loading

    // setery
    void autosaveProjectStruct_ASAP();

    void stopAudioEngine();
    void startAudioEngine();

    //-------------------------------------------------
    // metody glowne
    bool createNewProjectInWorkspace();
    bool openProjectFromWorkspace();

    bool openProject(char* path, char* name);
    bool importModAfterLoadNewProject(char* path, char* name);
    bool openRecentProject(int indexOnRecentList);

    bool saveProjectToWorkspace(bool forceSaveAll = false);

    // Runs pending workspace saves to completion instead of leaving them to
    // autosave. Refuses while another operation is using the card.
    bool flushWorkspaceSave();

    // For while something else, like a USB host, owns the card. Changes made
    // in the meantime are saved once the suspension ends.
    void setWorkspaceSaveSuspended(bool suspended);

    bool saveProject(char* projectPathToSave = nullptr, char* projectNameToSave = nullptr);

    bool importSamplesToProject(
      uint8_t fileFrom,
      uint8_t fileTo,
      uint8_t instrumentSlot,
      uint16_t dataFrom,
      uint16_t dataTo,
      bool isLofi,
      FileManagerOperation::SampleLoadMode loadMode = FileManagerOperation::SampleLoadMode::kRam);
    bool importSampleToProject(char* filePath, char* fileName, uint8_t instrumentSlot);
    bool copyInstrumentsInWorkspace(uint8_t copyInstrSrc,
                                    uint8_t copyInstrCount,
                                    uint8_t copyInstrDest);

    bool browseSdCard(uint8_t* index);
    bool browseFirmwares();
    bool browseGames();

    void createPatchesFolder();

    const char* getProjectsDefaultPath();
    const char* getModsDefaultPath();
    void createProjectsAndModFolders();

    bool previevSamplefromSD(uint8_t index, uint16_t from, uint16_t to);
    bool stopPrevievSamplefromSD();

    bool deleteInstruments(uint8_t instrumentSlotFrom, uint8_t instrumentSlotTo);

    void loadWorkspacePattern(uint8_t index);
    const Sequencer::strPattern* loadWorkspacePatternToTempBuffer(uint8_t index);

    bool deleteProjectOrFolder(char* projectPath);
    bool deleteMod(char* modPathWithName);

    bool exportSound(uint8_t mode);
    bool exportSoundRenderSelection(char* filePath);
    bool exportSoundCancel();
    bool saveRecordedSound(char* dirPath,
                           char* fileName,
                           std::optional<int8_t> importSlot,
                           int16_t* leftChannelAddress,
                           int16_t* rightChannelAddress,
                           uint32_t length);
    bool exportProjectSamples();
    void exportProjectSamplesCancel();

    bool importSampleFromSampleEditor(int16_t* memoryAddressL,
                                      int16_t* memoryAddressR,
                                      uint32_t samplesCount,
                                      uint8_t instrumentIdx);

    bool reloadSamplesFromWorkspace();

    bool drawSampleWaveform(int16_t file_index,
                            std::vector<int8_t*>& lower,
                            std::vector<int8_t*>& upper);
    uint32_t getImportingSamplesCount();

    // to na pozniej \/

    bool loadWorkspacePatternNow(uint8_t index);
    bool saveWorkspacePatternNow(uint8_t index);
    bool loadPerformanceTrackToBuffer(uint8_t pattIndex, uint8_t trackIndex);
    bool isPatternLoading();
    bool isPatternPreloaded(uint8_t index);
    void forgetPreloadedPattern();
    bool isPatternEmpty(uint8_t index);

    uint8_t getNextSongPattern();
    uint8_t getSongPattern(uint8_t pos);
    std::optional<int> getPatternPosInSong(uint8_t patternToSearch);
    void setSongPos(uint8_t pos);
    bool switchNextPatternInSong();
    void copySongTracks(char* currentProjectPath,
                        uint8_t srcPatternIdx,
                        uint8_t destPatternIdx,
                        uint8_t trackStartSrc,
                        uint8_t trackStartDest,
                        uint8_t tracksNum);
    void deleteTracks(char* currentProjectPath,
                      uint8_t srcPatternIdx,
                      uint8_t trackStartSrc,
                      uint8_t tracksNum);

    void updateActualPatternBitmask();
    bool updatePatternBitmask(uint8_t patternIdx, Sequencer::strPattern* sourcePattern);

    // UNDO/REDO
    void storePatternUndoRevision();
    void storeSongUndoRevision(uint8_t index);
    void undoSongPattern();
    void undoPattern();
    void redoPattern();

    void storePatternSelection(uint8_t index);
    void restorePatternSelection(uint8_t index);

    float importMod_getProgress();
    uint8_t exportItFile_getProgress();

    uint8_t exportItFile();

    //-------------------------------------------------
    // globalne
    void getCurrentProjectPathWithName(char* path, int maxLen);
    char* getCurrentProjectName() { return currentProjectName; }
    char* getCurrentProjectPath() { return currentProjectPatch; }
    void clearCurrentProjectPath() { currentProjectPatch[0] = 0; }
    void clearCurrentProjectName() { currentProjectName[0] = 0; }
    char* getProjectNamefromProjectFile() { return projectNamefromProjectFile; }
    char* getProjectPathfromProjectFile() { return projectPatchfromProjectFile; }

    void addCurrentProjectToRecentList();
    bool getProjectRecentList(char** list, int& listLen);
    bool readProjectRecentList();
    void writeProjectRecentList();
    bool validateRecentListFile();

    void checkLoopParameters(struct strInstrument*);

    char** getExplorerList();
    int getExplorerListLength();
    char* getExplorerPath();
    void createEmptyInstrumentInWorkspace(int idx, char* name, bool lofi = false);
    bool copySampleIntoWorkspaceInstrument(int idx,
                                           char* sourceFilePath,
                                           char* sourceFileName,
                                           uint16_t sampleStart,
                                           uint16_t sampleEnd,
                                           bool lofi = false);
    void moveSampleMemory(int firstSlotToMoveInMemory, int32_t memory_offset);

    uint8_t calcFirstSlotToMoveInMemory(uint8_t calcStartSlot);
    void resetBrowse();

    bool isFilePtiInstrument(char* file);
    uint32_t getSampleMemorySize(int fromSlot, int toSlot);
    void setDefaultActiveInstrument(struct strInstrument* targetInstrument);
    void removeWavExtensionFromName(char* fileName);

    void copySampleNameWithoutExtension(char* sampleName, char* fileName, int maxLength);

    bool loadInstrumentFormFileStruct(strInstrument* instrument,
                                      strInstrumentFile* instrumentFile,
                                      uint32_t instrumentFileSize,
                                      uint32_t& instrumentFileDataOffsetOut);

    void completeLoadedInstrumentStruct(int idx, bool success, uint32_t samplesCount = 0);

    bool saveSampleDataToWorkspace(int idx);

    bool saveInstrumentStructToWorkspace(int idx);

    void loadProjectValues();

    void saveProjectFileToWorkspace();
    void loadPatternFromWorkspace(uint8_t index);

    bool loadProjectFileFormFileStruct(strMtProject* project, strProjectFile* pFile);
    bool writeProjectFileToFileStruct(strMtProject* project, strProjectFile* pFile);

    bool needImportModFileAfterNewProject() { return importModFileAfterNewProject; }
    void clearImportModFileAfterNewProject() { importModFileAfterNewProject = false; }

    void savePatternToWorkspace();

    void clearWorkspace();
    bool createWorkspaceDirs();

    bool createCurrentProjectFolderTree();

    bool isInstrumentStereo(int idx);
    void setInstrumentSampleAddress(int idx, uint16_t channelCount, uint32_t samplesCount);
    bool promoteStreamedInstrumentToRam(int idx);
    bool demoteRamInstrumentToStream(int idx,
                                     const char* backingPtiPath,
                                     uint32_t dataOffset,
                                     bool forceStream = false);

    bool isWorkspaceInstrumentPtiFileValid(int idx);
    bool resolveProjectInstrumentPtiPath(int idx, char* outPath, size_t maxLen);
    bool getPtiFileDataOffset(const char* path, uint32_t& dataOffsetOut);
    bool isPtiFileValid(char* path,
                        bool& stereo,
                        uint32_t instrumentFileDataOffset,
                        uint32_t expectedSamplesCountPerChannel);

    uint32_t getWaveSizeAndHederIfValid(const char* fileName, strWavFileHeader& header);
    uint32_t getWaveSizeIfValid(const char* filename);
    uint32_t getWaveSizeAfterConversion(const char* filename, bool countStereo);

    bool startNewInstrumentSampleLoad(int idx, char* samplePath, uint32_t& instrumentFileDataOffset);
    bool loadSampleData(int idx,
                        uint8_t& sampleLoadStep,
                        uint32_t instrumentFileDataOffset,
                        bool loadPtiOnly);

    uint8_t saveInstrument(uint8_t idx);
    bool saveInstrumentToPtiFile(uint8_t idx, const char* destPath);
    void writeInstrumentToFileStruct(strInstrument* instrument, strInstrumentFile* instrumentFile);

    FileManagerOperation::SampleChannelMode getSampleMode()
    {
        return stereoSampleMode ? FileManagerOperation::kLeftAndRight : FileManagerOperation::kLeft;
    }

    void checkVersionAndTranslate(Sequencer::strPattern* pattern);

  private:
    friend FileManagerOperation;
    FileSystem* fileSystem = nullptr;
    Synth* synth;

    SampleImport sampleImport;
    WorkspaceProjectLoad workspaceProjectLoad;
    WorkspaceProjectSave workspaceProjectSave;
    ProjectOpen projectOpen;
    ProjectSave projectSave;
    InstrumentCopy instrumentCopy;
    InstrumentDelete instrumentDelete;
    SampleMemoryReload sampleMemoryReload;
    SampleMemorySave sampleMemorySave;
    RecordingSave recordingSave;
    int currentPattern = -1; // -1 for asserts
    int loadedPattern = -1;  // -1 for asserts

    PatternsMetadata patternsMetadata;

    bool stereoSampleMode = false;
    bool workspaceSaveSuspended = false;

    // na cele zewnetrzne (popupy itp)
    uint8_t status;
    uint8_t progress;

    // wewnetrzne stany maszynki itp
    uint8_t currentOperation;
    uint8_t currentOperationStep;
    // Pattern that loadWorkspacePattern() was asked for and update() has not started yet.
    volatile uint8_t requestedPattern = 0;
    // Pattern that a finished loadWorkspacePattern() left in the sequencer's buffer pattern;
    // 0 once the buffer has been used for anything else.
    volatile uint8_t preloadedPattern = 0;
    char currentCopyDestPath[PATCH_SIZE];
    bool forceOperation;

    // stringi
    char currentProjectPatch[PATCH_SIZE] = { 0 }; // can be 0 if root folder
    char currentProjectName[PROJECT_NAME_SIZE] = { 0 };
    char projectNamefromProjectFile[PROJECT_NAME_SIZE] = { 0 };
    char projectPatchfromProjectFile[PATCH_SIZE - PROJECT_NAME_SIZE] = { 0 };

    // do obliczania progresu
    uint32_t totalMemoryToTranfser;
    uint32_t actualMemoryTransfered;

    // flagi zmian w aktywnym projekcie
    struct strChangesFlags
    {
        uint8_t save_asap;
        uint8_t project;
        uint8_t instrument[kSampleInstrumentsCount];
        uint8_t sample[kSampleInstrumentsCount];
        uint8_t pattern[PATTERN_INDEX_MAX + 1];
    } changesFlags;

    // metody wewnetrzne ------------------------------------
    void report(const char* text, uint8_t value = 0);
    void report(const char* text, const char* text2);
    void moveToNextOperationStep();
    void moveToPrevOperationStep();
    void calcTotalMemoryToTransfer();

    // glowne update / workspace ------------------------------------
    void updateImportModFile();
    void updateExportModFile();
    void updateBrowseSamples();
    void updateBrowseFirmwares();
    void updateBrowseGames();
    void startRequestedPatternLoad();
    void updateLoadWorkspacePattern();
    void updateExportSound();
    void updateExportProjectSamples();
    void updateSaveRecordedSound();
    void updateDrawSampleWaveform();

    void autoSaveProjectToWorkspace();
    void stopAutosave();

    void stopOperationWithError(uint8_t error);

    bool createNewEmptyProjectInWorkspace();

    uint8_t organizeWorkspaceStep;

    // project ------------------------------------

    void modifyWorkspaceProjectFile();

    void writeProjectFile(const char* name, strMtProject* proj);
    bool readProjectFile(const char* name, strMtProject* proj);
    void getDefaultProject(struct strMtProject* source);
    void getDefaultSong(struct strSong* source);
    void getDefaultValues(struct strMtValues* source);

    // pattern ------------------------------------

    bool continuePatternProcess();

    void loadPatternFromWorkspaceFinish();

    bool validatePatternFileData(const char* filePath,
                                 const Sequencer::strPattern* pattern,
                                 size_t fileSize);
    bool writePatternFile(const char* filePath, Sequencer::strPattern* sourcePattern);
    bool readPatternFile(const char* filePath, Sequencer::strPattern* destPattern);
    bool saveActualPattern(const char* path, uint8_t index);
    bool loadPattern(const char* path, uint8_t index);

    void translateVolumeFxInPatternFromFileVersion1ToFileVersion2(
      Sequencer::strPattern* patternToTranslate);
    void translateBeatSliceNoteInPatternFromFileVersionLessThan3(
      Sequencer::strPattern* patternToTranslate);
    void translateVolumeFxInPatternFromFileVersion3ToFileVersion4(
      Sequencer::strPattern* patternToTranslate);
    void translatePatternFromFileVersion4ToFileVersion5(Sequencer::strPattern* patternToTranslate);
    void translatePatternFromFileVersion5ToFileVersion6(Sequencer::strPattern* patternToTranslate);
    void translatePatternWithDifferentTrackCount(Sequencer::strPattern* pattern);

    uint32_t calcProjectPatternsSize();

    uint16_t projectPatternsCount = 0;
    uint16_t copiedPatternsCount = 0;

    // instrument ------------------------------------
    void getEmptyInstrument(struct strInstrument* source);

    uint8_t currentInstrument = 0;
    uint8_t lastActiveInstrument = 0; // do przydzielania pamieci
    uint8_t deleteEndInstrument;

    // samples ------------------------------------

    void deleteSamplesFromWorkspace();

    void moveMemory(uint8_t* memoryStart, uint8_t* memoryEnd, int32_t memoryOffset);

    uint32_t getInstrumentSampleSizeIfValid(const char* filename);

    uint32_t currentInstrumentFileStructSize;
    uint8_t deleteEndSample;

    uint32_t newSamplesSize = 0;
    uint32_t oldSamplesSize = 0;

    uint8_t importCurrentFile;
    int8_t importStartSlot;
    int8_t importEndSlot;

    uint32_t samplesMemoryCopied;

    // browsers
    // project
    static const uint8_t list_length_max = 101; // 100 + parent folder
    uint8_t projectsListLength = 0;
    static char* projectsList[list_length_max];

    void browseFirmwaresLocation();
    uint8_t firmwaresListLength = 0;
    static char* firmwaresList[list_length_max];

    void browseModsLocation();
    uint8_t modsListLength = 0;
    static char* modsList[list_length_max];

    void browseGamesLocation();

    void browseCurrentLocation();
    void listOnlyFolderNames();
    void processDirFileSizes();
    void listOnlyWavFromActualPath();
    void browseFinish();
    void goUpInActualPath();

    uint8_t explorerListLength = 0;
    static char* explorerList[list_length_max];
    char explorerCurrentPath[255] = { '/', 0 };
    uint16_t explorerCurrentOffset;
    uint8_t explorerDirLevel = 0;
    uint8_t explorerFolderCount;

    uint8_t listWavStage;

    static const uint8_t PREVIOUS_POSITION_LIFO = 20;
    uint8_t explorerPositionTable[PREVIOUS_POSITION_LIFO];
    uint8_t explorerCurrentPosition;

    uint8_t openCalcStart = 0;
    uint8_t openCalcEnd = 0;
    uint8_t openCurrentPos = 0;

    static uint32_t currentFolderMemoryFileUsage[list_length_max];

    // export sound
    void exportSoundEnd();
    void exportSoundGetStatus();
    uint8_t getExportProgress();
    void exportSamplesStart();
    uint32_t getTotalSamplesMemoryExported();

    // recording
    void saveRecordingEnd();
    char* getRecordingDirPath();
    char* getRecordingFileName();
    void saveRecordedSoundContinue();
    void saveRecordedSoundFinish();
    uint8_t recordingImportSlot;

    /// IMPORT .MOD .IT etc

    enImportModMode checkImportFileType();

    bool importModFileAfterNewProject = 0;

    uint8_t importModFileType = importModFiletype_mod;

    struct strImportFileDataCommon
    {
        char filename[PATCH_SIZE]; // tylko nazwa
        char path[PATCH_SIZE];     // cala sciezka

    };
    static strImportFileDataCommon impFileData;

    /// IMPORT .MOD

    void importModFile_Init();
    void importModFile_GetInstrumentData();
    void importModFile_SaveInstrument();

    void importModFile_SongInit();
    void importModFile_Patterns();
    void importModFile_ImportWave();
    void importModFile_WriteWave();
    void importModFile_Finish();
    void importModFile_Error();

    int8_t periodToNote(uint16_t period);
    void printNote(uint8_t note);

    /// IMPORT MOD KONIEC

    // import IT
    void importItFile_Init();
    void importItFile_ProcessHeader();
    void importItFile_ProcessSong();
    void importItFile_ProcessOffsets();

    void importItFile_ProcessInstruments();
    void importItFile_LoadSamples();
    void importItFile_ProcessPatterns();
    void importItFile_ProcessPattern(uint32_t patternOffset, uint16_t length, uint16_t rows);
    void importItFile_WriteWaves();
    void importItFile_Finish();

    void importItFile_setPattern(uint8_t index, uint8_t length);
    void importItFile_setStep(uint8_t step,
                              uint8_t track,
                              uint8_t note,
                              int16_t volume,
                              uint8_t instrument,
                              uint8_t fx,
                              uint8_t fxVal);
    void importItFile_savePattern();
    void importItFile_initPatternBuffer(uint32_t pattLength, uint32_t patternOffset);

    uint8_t importItFile_getNextPatternByte();

    float importItFile_getProgress();

    uint32_t getInstrumentOffset(uint8_t index);
    uint32_t getSampleOffset(uint8_t index);
    uint32_t getPatternOffset(uint8_t index);
    uint32_t getFileVariable(uint32_t subFileOffset, uint32_t offset, uint8_t varSize);

    // import IT koniec

    // EXPORT MOD
    void exportItFile_Init();
    void exportItFile_InitHeader();
    void exportItFile_ProcessInstruments();
    void exportItFile_ProcessSamples();
    void exportItFile_ProcessWaves();
    void exportItFile_ProcessPatterns();
    void exportItFile_Finish();
    void exportItFile_Error();

    void exportItFile_storeInstrumentOffset();
    void exportItFile_storeSampleOffset();
    void exportItFile_storePatternOffset();
    void exportItFile_storeWaveOffset();

    //  draw sample waweform
    void drawSampleWaveformInit();
    void drawSampleWaveformUpdate();
    void drawSampleWaveformFinish();

    // debug Log
    void saveDebugLogToSd();

    CrcCalculator* crcCalculator;
};

extern int16_t externalRamSampleMemory[];

extern cFileManager newFileManager;
#endif /* FILEMANAGER_FILEMANAGER_H_ */
