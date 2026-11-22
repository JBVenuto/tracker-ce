#include "fileManagerDefs.h"
#include "mtStructs.h"

#include "core/interfacePopups.h"

#include "fileManager.h"
#include "patternEditor/patternEditor.h"
#include <algorithm>
static cPatternEditor* PTE = &patternEditor;

const uint8_t UNDO_CAPACITY = 20;
__NOINIT(EXTERNAL_RAM) Sequencer::strPattern undoPatternBuffer[UNDO_CAPACITY];
__NOINIT(EXTERNAL_RAM) uint8_t undoPatternBuffer_Indexes[UNDO_CAPACITY];

struct strUndoPatternBuffer_Selection
{
    int8_t currentTrack;
    int16_t currentStep;

    int8_t selectState;

    uint8_t editParam_;

    int8_t selectStartTrack;
    int16_t selectStartStep;
    int8_t selectEndTrack;
    int16_t selectEndStep;
};

__NOINIT(EXTERNAL_RAM) strUndoPatternBuffer_Selection undoPatternBufferSelection[UNDO_CAPACITY];

struct strUndo
{
    uint8_t currentIndex = 0;    // gdzie jesteśmy w tablicy
    uint8_t storedCount = 0;     // ile razy zrzuciliśmy
    uint8_t redoPossibility = 0; // ile razy zrzuciliśmy

} static undo;

const uint8_t UNDO_SONG_CAPACITY = 10;
__NOINIT(EXTERNAL_RAM) Sequencer::strPattern undoSongBuffer[UNDO_SONG_CAPACITY];
__NOINIT(EXTERNAL_RAM) uint8_t undoSongBufferIndexes[UNDO_SONG_CAPACITY];

struct strUndoSong
{
    uint8_t nextIndex = 0;       // gdzie jesteśmy w tablicy
    uint8_t storedCount = 0;     // ile razy zrzuciliśmy
    uint8_t redoPossibility = 0; // ile razy zrzuciliśmy
} static undoSong;

void cFileManager::storePatternSelection(uint8_t index)
{
    strUndoPatternBuffer_Selection* dest = &undoPatternBufferSelection[index];
    strTrackerPattern* src = &PTE->trackerPattern;

    dest->editParam_ = PTE->editParam;

    dest->currentTrack = src->actualTrack;
    dest->currentStep = src->actualStep;

    dest->selectState = src->selectState;

    dest->selectStartTrack = src->selectStartTrack;
    dest->selectStartStep = src->selectStartStep;
    dest->selectEndTrack = src->selectEndTrack;
    dest->selectEndStep = src->selectEndStep;
}
void cFileManager::restorePatternSelection(uint8_t index)
{
    strTrackerPattern* dest = &PTE->trackerPattern;
    strUndoPatternBuffer_Selection* src = &undoPatternBufferSelection[index];
    const uint8_t maxTracks = getProjectMaxTracks();

    PTE->editParam = src->editParam_;

    dest->actualTrack =
      static_cast<int8_t>(std::min<int>(std::max<int>(src->currentTrack, 0), maxTracks - 1));
    dest->actualStep = src->currentStep;

    if (src->selectState == 2 &&
        (src->selectStartTrack >= maxTracks || src->selectEndTrack >= maxTracks))
    {
        dest->selectState = 1;
        dest->selectStartTrack = dest->actualTrack;
        dest->selectEndTrack = dest->actualTrack;
        dest->selectColumn = 0;
    }
    else
    {
        dest->selectState = src->selectState;
        dest->selectStartTrack = static_cast<int8_t>(
          std::min<int>(std::max<int>(src->selectStartTrack, 0), maxTracks - 1));
        dest->selectEndTrack =
          static_cast<int8_t>(std::min<int>(std::max<int>(src->selectEndTrack, 0), maxTracks - 1));
    }

    dest->selectStartStep = src->selectStartStep;
    dest->selectEndStep = src->selectEndStep;
}
void cFileManager::storePatternUndoRevision()
{
    if (undo.currentIndex > (UNDO_CAPACITY - 1))
    {
        // zabezpieczenie przed kraksą
        undo.currentIndex = 0;
        undo.storedCount = 0;
    }

    undoPatternBuffer[undo.currentIndex] = *sequencer.getActualPattern();
    undoPatternBuffer_Indexes[undo.currentIndex] = mtProject.values.actualPattern;
    storePatternSelection(undo.currentIndex);

    undo.redoPossibility = 0;
    if (undo.currentIndex >= (UNDO_CAPACITY - 1))
    {
        undo.currentIndex = 0;
        undo.storedCount++;
    }
    else
    {
        undo.currentIndex++;
        undo.storedCount++;
    }
    if (undo.storedCount >= (UNDO_CAPACITY - 1))
        undo.storedCount =
          UNDO_CAPACITY - 1; // jeden slot musi zostać na zbuforowanie ostatniego REDO
}

void cFileManager::storeSongUndoRevision(uint8_t patternIndex)
{
#ifdef FILEMANAGER_DEBUGLOG
    report("store song undo revision", patternIndex);
#endif
    uint8_t undoIndex = undoSong.nextIndex;
    undoSong.nextIndex += 1;
    if (undoSong.nextIndex == UNDO_SONG_CAPACITY)
    {
        undoSong.nextIndex = 0;
    }

    char patternToLoad[PATCH_SIZE]{ 0 };
    snprintf(patternToLoad, PATCH_SIZE, "Workspace/patterns/pattern_%02d.mtp", patternIndex);
    if (!readPatternFile(patternToLoad, &undoSongBuffer[undoIndex]))
    {
        sequencer.clearPattern(&undoSongBuffer[undoIndex]);
    }

    undoSongBufferIndexes[undoIndex] = patternIndex;
    undoSong.redoPossibility = 0;
    if (undoSong.storedCount < UNDO_SONG_CAPACITY)
    {
        undoSong.storedCount += 1;
    }
#ifdef FILEMANAGER_DEBUGLOG
    char line[82];
    snprintf(line,
             82,
             ">>>pattern %d stored: nextIndex: %d, storedCount: %d, redoPossibility: %d",
             patternIndex,
             undoSong.nextIndex,
             undoSong.storedCount,
             undoSong.redoPossibility);
    report(line);
#endif
}

void cFileManager::undoSongPattern()
{
    if (undoSong.storedCount == 0)
    {
#ifdef FILEMANAGER_DEBUGLOG
        report("no song undo data");
#endif
        mtPopups.show(0, "No undo data :(");
        return;
    }

    uint8_t undoIndex = undoSong.nextIndex;
    if (undoIndex > 0)
    {
        undoIndex -= 1;
    }
    else
    {
        undoIndex = UNDO_SONG_CAPACITY - 1;
    }

#ifdef FILEMANAGER_DEBUGLOG
    report("undo song pattern");
#endif

    char filePath[PATCH_SIZE]{ 0 };
    uint8_t bufferedPatternNumber = undoSongBufferIndexes[undoIndex];
    snprintf(filePath, PATCH_SIZE, "Workspace/patterns/pattern_%02d.mtp", bufferedPatternNumber);

    writePatternFile(filePath, &undoSongBuffer[undoIndex]);
    if (updatePatternBitmask(bufferedPatternNumber, &undoSongBuffer[undoIndex]))
    {
        setProjectStructChanged();
        autosaveProjectStruct_ASAP();
    }
    undoSong.nextIndex = undoIndex;
    undoSong.storedCount -= 1;
    undoSong.redoPossibility++; // TODO: make redo work

#ifdef FILEMANAGER_DEBUGLOG
    char line[82];
    snprintf(line,
             82,
             "<<<pattern %d restored: nextIndex: %d, storedCount: %d, redoPossibility: %d",
             bufferedPatternNumber,
             undoSong.nextIndex,
             undoSong.storedCount,
             undoSong.redoPossibility);
    report(line);
#endif
}

void cFileManager::undoPattern()
{
    bool doUndo = 0;
    uint8_t oldIndex = undo.currentIndex;

    if (undo.currentIndex > 0 && undo.storedCount > 0)
    {
        undo.currentIndex--;
        undo.storedCount--;
        doUndo = 1;
    }
    else if (undo.currentIndex == 0 && undo.storedCount > 0)
    {
        undo.currentIndex = UNDO_CAPACITY - 1;
        undo.storedCount--;
        doUndo = 1;
    }
    else
    {
        mtPopups.show(0, "No undo data.");
    }

    if (doUndo)
    {

        undoPatternBuffer[oldIndex] = *sequencer.getActualPattern();
        undoPatternBuffer_Indexes[oldIndex] = mtProject.values.actualPattern;
        storePatternSelection(oldIndex);

        *sequencer.getActualPattern() = undoPatternBuffer[undo.currentIndex];
        mtProject.values.actualPattern = undoPatternBuffer_Indexes[undo.currentIndex];
        restorePatternSelection(undo.currentIndex);

        setActualPatternStructChanged();
        undo.redoPossibility++;
    }
}
void cFileManager::redoPattern()
{
    bool doRedo = 0;

    if (undo.currentIndex < UNDO_CAPACITY - 1 && undo.redoPossibility > 0)
    {
        undo.currentIndex++;
        undo.storedCount++;
        doRedo = 1;
    }
    else if (undo.currentIndex == UNDO_CAPACITY - 1 && undo.redoPossibility > 0)
    {
        undo.currentIndex = 0;
        undo.storedCount++;
        doRedo = 1;
    }
    else
    {
        mtPopups.show(0, "No redo data.");
    }

    if (doRedo)
    {
        undo.redoPossibility--;
        *sequencer.getActualPattern() = undoPatternBuffer[undo.currentIndex];
        mtProject.values.actualPattern = undoPatternBuffer_Indexes[undo.currentIndex];
        restorePatternSelection(undo.currentIndex);

        setActualPatternStructChanged();
    }
}
