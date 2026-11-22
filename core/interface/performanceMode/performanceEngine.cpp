
#include "mtStructs.h"

#include "fileManager.h"
#include "mtConfig.h"

#include "mtAudioEngine.h"
#include "mtSequencer.h"

#include "performanceFx.h"

#include "performanceEngine.h"

#include <utility>

void saveProjectPerformanceValues()
{
    newFileManager.setProjectStructChanged();
}
void forceSaveProjectPerformanceValues()
{
    newFileManager.autosaveProjectStruct_ASAP();
}
// void saveConfigPerformanceValse() { saveConfig(); }

const uint8_t performanceDataSourceCount = 2;

struct PerformanceModeCC
{
    uint8_t isSelectedCC[12];
    int16_t ccValues[12];
} performanceModeCC;

strPerformDataSource performanceDataSource[performanceDataSourceCount] = {
    // global
    { mtConfig.values.perfTracksPatterns,
      mtConfig.values.perfFxPlaces,
      &mtConfig.values.perfFxValues[0][0],
      mtConfig.values.perfSelectedValues,
      mtConfig.values.perfTracksState,
      saveConfig,
      saveConfigAsap,
      performanceModeCC.ccValues,
      performanceModeCC.isSelectedCC },

    // project
    { mtProject.values.perfTracksPatterns,
      mtProject.values.perfFxPlaces,
      &mtProject.values.perfFxValues[0][0],
      mtProject.values.perfSelectedValues,
      mtProject.values.perfTracksState,
      saveProjectPerformanceValues,
      forceSaveProjectPerformanceValues,
      performanceModeCC.ccValues,
      performanceModeCC.isSelectedCC }
};

cPerformFx::cPerformFx(uint8_t fxId,
                       const char* fxName,
                       void (*fxSetFunct)(uint8_t, int16_t),
                       void (*fxClearFunct)(uint8_t),
                       const fxSetup* fxSetup)
  : id(fxId)
  , name(fxName)
  , setFunct(fxSetFunct)
  , clearFunct(fxClearFunct)
  , setup(fxSetup)
{
    performance.fxList[id] = this;
}

bool cPerformEngine::turnOff()
{
    bool wasPerformanceMode = sequencer.isPerformanceMode();
    if (wasPerformanceMode)
    {
        sequencer.exitPerformanceMode();
        engine.performanceModeEndAll();
    }

    for (uint8_t track = 0; track < kPerformanceTracks; track++)
    {
        tracksPerformanceState[track] = 0;
    }

    resetAllFx();

    return wasPerformanceMode;
}

void cPerformEngine::forceSaveConfig()
{
    if (sourceDataForceSaveFunct != nullptr)
    {
        sourceDataForceSaveFunct();
    }
}

const char* cPerformEngine::getSlotFxName(uint8_t slot)
{
    return fxList[slotToFx[slot]]->name;
}

// 0-3 zasiweca padem; 4 gasi wszystko - fx none
int8_t cPerformEngine::getSelectedRow(uint8_t slot)
{
    if (slot < 12 && slotToFx[slot] == 0)
        return -1;

    return slot < 12 ? slotToRow[slot] : 0;
}

uint8_t cPerformEngine::getTrackState(uint8_t track)
{
    return tracksPerformanceState[track];
}

uint8_t cPerformEngine::slotChanged(uint8_t slot)
{
    uint8_t flag = slotChangeFlag[slot];
    slotChangeFlag[slot] = 0;
    return flag > 0;
}

void cPerformEngine::setTrackState(uint8_t track, uint8_t state)
{
    tracksPerformanceState[track] = state;
}

// prezepisanie aktualnie zaznaczonych wartosci do chwilowych
void cPerformEngine::loadSelectedValuesToTempValues()
{
    for (uint8_t slot = 0; slot < 12; slot++)
    {
        if (slotToRow[slot] > 0)
        {
            slotsTempValues[slot] = *(slotRowToValue + (slot * 4) + slotToRow[slot]);
        }
        else
        {
            slotsTempValues[slot] = 0;
        }
    }
}

void cPerformEngine::toggleTrackState(uint8_t track)
{
    if (track >= getProjectMaxTracks())
    {
        return;
    }

    tracksPerformanceState[track] = !tracksPerformanceState[track];

    if (tracksPerformanceState[track] == 0)
    {
        for (uint8_t fx = 0; fx < fxCount; fx++)
        {
            resetFxOnTrack(fx, track);
        }
    }
    else
    {
        loadAllSlotsFxOnTrack(track);
    }
}

void cPerformEngine::disarmHiddenTracks(uint8_t maxTracks)
{
    for (uint8_t track = maxTracks; track < kPerformanceTracks; track++)
    {
        if (tracksPerformanceState[track] != 0)
        {
            tracksPerformanceState[track] = 0;
            for (uint8_t fx = 0; fx < fxCount; fx++)
            {
                resetFxOnTrack(fx, track);
            }
        }
    }
}

void cPerformEngine::fillSlotFxValue(uint8_t slot, uint8_t row, char* value)
{
    constexpr uint8_t CC_MODE_ROW = 4;
    int16_t fx_value = 0;

    if (row == CC_MODE_ROW)
    {
        fx_value = slotToCCValue[slot];
        if (fxList[slotToFx[slot]]->setup->alternativeValuesLabels == nullptr)
        {
            if (fx_value == 0)
                strcpy(value, "---");
            else if (fx_value > 0)
                sprintf(value, "%d", fx_value);
            else
                sprintf(value, "%d", fx_value);
        }
        else
        {
            const fxSetup* temp_setup = fxList[slotToFx[slot]]->setup;
            if (fx_value > temp_setup->max)
                fx_value = temp_setup->max;
            strncpy(value,
                    temp_setup->alternativeValuesLabels +
                      (temp_setup->alternativeValuesLabelsLength * fx_value),
                    7);
        }

        return;
    }
    else if (slotToActiveCC[slot])
        return;

    if (row >= 0) // teraz wszystkie wiersze moga byc zmienialne; (row > 0) -> jesli 1 wiersz jest
                  // nie zmienny
    {
        fx_value =
          (row ==
           slotToRow[slot]) // ten zapis ustawia na start zaznaczone wartosci  fxow z tempValues
            ? slotsTempValues[slot] // zamiast poprawnych;
            : *(slotRowToValue + (slot * 4) +
                row); // wymaga wczesniejszego wypelnienia slotsTempValues poprawnymi wartosciami
    }

    if (fxList[slotToFx[slot]]->setup->alternativeValuesLabels == nullptr)
    {
        if (fx_value == 0)
            strcpy(value, "---");
        else
            sprintf(value, "%+d", fx_value);
    }
    else
    {
        const fxSetup* temp_setup = fxList[slotToFx[slot]]->setup;
        if (fx_value > temp_setup->max)
            fx_value = temp_setup->max;
        strncpy(value,
                temp_setup->alternativeValuesLabels +
                  (temp_setup->alternativeValuesLabelsLength * fx_value),
                7);
    }
}

void cPerformEngine::setSelectedRow(uint8_t slot, uint8_t row)
{
    if (slotToFx[slot] == 0)
        slotToRow[slot] = 0;
    else
        slotToRow[slot] = row;
}

void cPerformEngine::resetSelectedRows()
{
    for (int slotIdx = 0; slotIdx < 12; slotIdx++)
    {
        setSelectedRow(slotIdx, 0);
    }
}

void cPerformEngine::setSlotState(uint8_t slot, uint8_t state)
{
    slotsState[slot] = state;
}

void cPerformEngine::resetAllFx()
{
    for (uint8_t fx = 0; fx < fxCount; fx++)
    {
        if (fxList[fx]->setup->isGlobalFx)
        {
            resetGlobalFxValue(fx);
        }
        else
        {
            for (uint8_t track = 0; track < kPerformanceTracks; track++)
            {
                resetFxOnTrack(fx, track);
            }
        }
    }
}

void cPerformEngine::resetFxOnTrack(uint8_t fx, uint8_t track)
{
    if (fx == 0)
        return;

    if (fxList[fx]->setup->isGlobalFx)
    {
        return;
    }

    fxList[fx]->clearFunct(track);
}

void cPerformEngine::resetSlotFx(uint8_t slot)
{
    if (slot >= 12)
        return;

    uint8_t fxIdx = slotToFx[slot];
    if (fxIdx == 0)
        return;

    if (fxList[fxIdx]->setup->isGlobalFx)
    {
        resetGlobalFxValue(fxIdx);
    }
    else
    {
        for (uint8_t track = 0; track < kPerformanceTracks; track++)
        {
            // moze warto czyscic zapobiegawczo na wszystkich trakac? xxx
            if (tracksPerformanceState[track] == 1)
            {
                resetFxOnTrack(fxIdx, track);
            }
        }
    }
}

void cPerformEngine::loadAllSlotsFxOnTrack(uint8_t track)
{
    for (uint8_t slot = 0; slot < 12; slot++)
    {
        uint8_t temp_fx = slotToFx[slot];
        if (temp_fx == 0)
            continue;

        // zaladuj fx tylko na row > 0 i tlyko jesli wartosc != 0
        if (slotToRow[slot] > 0 && slotsTempValues[slot] != 0)
        {
            loadFxValuesOnTrack(slot, track);
        }
    }
}

void cPerformEngine::loadFxValuesOnTrack(uint8_t slot, uint8_t track)
{
    if (slot >= 12)
        return;

    uint8_t temp_fx = slotToFx[slot];
    if (temp_fx == 0)
        return;

    if (fxList[temp_fx]->setup->isGlobalFx)
    {
        return;
    }

    if (slotsTempValues[slot] == 0)
    {
        resetFxOnTrack(temp_fx, track);
        return;
    }

    fxList[temp_fx]->setFunct(track, slotsTempValues[slot]);
}

void cPerformEngine::loadGlobalFxValue(int fxIdx, int value)
{
    if (fxIdx > 0)
    {
        fxList[fxIdx]->setFunct(0, value);
    }
}

void cPerformEngine::resetGlobalFxValue(int fxIdx)
{
    if (fxIdx > 0)
    {
        fxList[fxIdx]->clearFunct(0);
    }
}

void cPerformEngine::loadSlotFxValues(uint8_t slot)
{
    // jezeli cos zdezaktywowalo slot to return
    if (slotsState[slot] == 0)
        return;

    resetOtherSlotsWithSameFx(slot);
    resetSlotsWithBlockedFx(slot);
    slotChangeFlag[slot] = 1;

    if (slotToRow[slot] == 0)
    {
        slotsTempValues[slot] = 0;
        resetSlotFx(slot);
        return;
    }

    // Row 4 marks a slot driven by MIDI CC. It has no stored value to load: the value in
    // use came from the controller.
    if (slotToRow[slot] >= 4)
    {
        return;
    }

    slotsTempValues[slot] = *(slotRowToValue + (slot * 4) + slotToRow[slot]);

    int fxIdx = slotToFx[slot];
    if (fxIdx == 0)
    {
        return;
    }

    if (fxList[fxIdx]->setup->isGlobalFx)
    {
        int newValue = slotsTempValues[slot];
        loadGlobalFxValue(fxIdx, newValue);
    }
    else
    {
        const uint8_t maxTracks = getProjectMaxTracks();
        for (uint8_t track = 0; track < maxTracks; track++)
        {
            if (tracksPerformanceState[track] == 1)
            {
                loadFxValuesOnTrack(slot, track);
            }
        }
    }
}

void cPerformEngine::loadCCValuesOnAffectedTracks(uint8_t slot)
{
    if (!slotToActiveCC[slot])
        return;

    resetOtherSlotsWithSameFx(slot);
    resetSlotsWithBlockedFx(slot);
    slotChangeFlag[slot] = 1;

    slotsTempValues[slot] = slotToCCValue[slot];

    int fxIdx = slotToFx[slot];
    if (fxIdx == 0)
    {
        return;
    }

    if (fxList[fxIdx]->setup->isGlobalFx)
    {
        int newValue = slotsTempValues[slot];
        loadGlobalFxValue(fxIdx, newValue);
    }
    else
    {
        const uint8_t maxTracks = getProjectMaxTracks();
        for (uint8_t track = 0; track < maxTracks; track++)
        {
            if (tracksPerformanceState[track] == 1)
            {
                loadFxValuesOnTrack(slot, track);
            }
        }
    }
}

void cPerformEngine::resetOtherSlotsWithSameFx(uint8_t slot)
{
    for (uint8_t s = 0; s < 12; s++)
    {
        if (s == slot)
            continue;

        if (slotToFx[s] == slotToFx[slot])
        {
            slotsState[s] = 0;
            slotToRow[s] = 0;
            slotChangeFlag[s] = 1;
            slotsTempValues[s] = 0;
        }
    }
}

void cPerformEngine::resetSlotsWithBlockedFx(uint8_t slot)
{
    const fxSetup* temp_setup = fxList[slotToFx[slot]]->setup;
    if (temp_setup->blockedFx1 == 0 && temp_setup->blockedFx2 == 0)
        return;

    for (uint8_t s = 0; s < 12; s++)
    {
        if (s == slot)
            continue;

        if (slotToFx[s] == temp_setup->blockedFx1)
        {
            slotsState[s] = 0;
            slotToRow[s] = 0;
            slotChangeFlag[s] = 1;
            slotsTempValues[s] = 0;
        }

        if (slotToFx[s] == temp_setup->blockedFx2)
        {
            slotsState[s] = 0;
            slotToRow[s] = 0;
            slotChangeFlag[s] = 1;
            slotsTempValues[s] = 0;
        }
    }
}

void cPerformEngine::changeSlotFxValue(uint8_t slot, int16_t value)
{
    if (slot >= 12)
        return;
    if (!slotsState[slot])
        return;
    // if(slotToRow[slot] == 0) return; // pierwszy tez modyfikujemy (od 1.1.17 ?)

    uint8_t fxIdx = slotToFx[slot];
    if (fxIdx == 0)
        return;

    // flaga zmiany wartosci slota
    slotChangeFlag[slot] = 1;

    // modyfikacja wartosci
    const fxSetup* temp_setup = fxList[slotToFx[slot]]->setup;
    if (slotsTempValues[slot] + value > temp_setup->max)
        slotsTempValues[slot] = temp_setup->max;
    else if (slotsTempValues[slot] + value < temp_setup->min)
        slotsTempValues[slot] = temp_setup->min;
    else
        slotsTempValues[slot] += value;

    // The CC row (4) has no place in the table of saved values.
    bool rowIsSaved = slotToRow[slot] > 0 && slotToRow[slot] < 4;
    if (editState && rowIsSaved) // zapis jesli edit mode, tylko jesli nie pierwszy wiersz
    {
        *(slotRowToValue + (slot * 4) + slotToRow[slot]) = slotsTempValues[slot];
        if (sourceDataSaveFunct != nullptr)
            sourceDataSaveFunct();
    }

    if (fxList[fxIdx]->setup->isGlobalFx)
    {
        int newValue = slotsTempValues[slot];
        loadGlobalFxValue(fxIdx, newValue);
    }
    else
    {
        // przeladowanie efektu z nowa wartoscia
        const uint8_t maxTracks = getProjectMaxTracks();
        for (uint8_t track = 0; track < maxTracks; track++)
        {
            if (tracksPerformanceState[track])
                loadFxValuesOnTrack(slot, track);
        }
    }
}

void cPerformEngine::changeSlotFx(uint8_t slot, int8_t change)
{
    uint8_t fx = slotToFx[slot];

    if (slotToRow[slot] > 0)
    {
        // konczy fx tylko jesli w tym slocie byla wybrana jakas wartosc (row > 0)
        if (fxList[fx]->setup->isGlobalFx)
        {
            resetGlobalFxValue(fx);
        }
        else
        {
            for (uint8_t track = 0; track < kPerformanceTracks; track++)
            {
                resetFxOnTrack(fx, track);
            }
        }
    }

    if (fx + change >= fxCount)
        fx = 0;
    else if (fx + change < 0)
        fx = fxCount - 1;
    else
        fx = fx + change;

    *(slotRowToValue + (4 * slot) + 0) = 0;
    *(slotRowToValue + (4 * slot) + 1) = 0;
    *(slotRowToValue + (4 * slot) + 2) = 0;
    *(slotRowToValue + (4 * slot) + 3) = 0;

    slotToRow[slot] = 0;
    slotsTempValues[slot] = 0;
    slotToFx[slot] = fx;

    if (sourceDataSaveFunct != nullptr)
        sourceDataSaveFunct();
}

void cPerformEngine::swapSlotFx(uint8_t slotFrom, uint8_t slotTo)
{
    int16_t temp_to1 = *(slotRowToValue + (4 * slotTo) + 0);
    int16_t temp_to2 = *(slotRowToValue + (4 * slotTo) + 1);
    int16_t temp_to3 = *(slotRowToValue + (4 * slotTo) + 2);
    int16_t temp_to4 = *(slotRowToValue + (4 * slotTo) + 3);

    *(slotRowToValue + (4 * slotTo) + 0) = *(slotRowToValue + (4 * slotFrom) + 0);
    *(slotRowToValue + (4 * slotTo) + 1) = *(slotRowToValue + (4 * slotFrom) + 1);
    *(slotRowToValue + (4 * slotTo) + 2) = *(slotRowToValue + (4 * slotFrom) + 2);
    *(slotRowToValue + (4 * slotTo) + 3) = *(slotRowToValue + (4 * slotFrom) + 3);

    *(slotRowToValue + (4 * slotFrom) + 0) = temp_to1;
    *(slotRowToValue + (4 * slotFrom) + 1) = temp_to2;
    *(slotRowToValue + (4 * slotFrom) + 2) = temp_to3;
    *(slotRowToValue + (4 * slotFrom) + 3) = temp_to4;

    uint8_t temp_to_fx = slotToFx[slotTo];
    slotToFx[slotTo] = slotToFx[slotFrom];
    slotToFx[slotFrom] = temp_to_fx;

    // The selected row, the value in use and the CC state belong to the FX and move with
    // it. Left behind, they would be applied to the other FX, whose range can be different.
    std::swap(slotToRow[slotTo], slotToRow[slotFrom]);
    std::swap(slotsTempValues[slotTo], slotsTempValues[slotFrom]);
    std::swap(slotToActiveCC[slotTo], slotToActiveCC[slotFrom]);
    std::swap(slotToCCValue[slotTo], slotToCCValue[slotFrom]);

    if (sourceDataSaveFunct != nullptr)
        sourceDataSaveFunct();
}

void cPerformEngine::copySlotFx(uint8_t slot)
{
    if (slot >= 12)
        return;

    isSlotCopied = true;
    slotCopied = slot;
}

void cPerformEngine::pasteSlotFx(uint8_t slot)
{
    if (slot >= 12 || slotCopied >= 12)
        return;

    if (!isSlotCopied)
        return;

    *(slotRowToValue + (4 * slot) + 0) = *(slotRowToValue + (4 * slotCopied) + 0);
    *(slotRowToValue + (4 * slot) + 1) = *(slotRowToValue + (4 * slotCopied) + 1);
    *(slotRowToValue + (4 * slot) + 2) = *(slotRowToValue + (4 * slotCopied) + 2);
    *(slotRowToValue + (4 * slot) + 3) = *(slotRowToValue + (4 * slotCopied) + 3);

    slotToFx[slot] = slotToFx[slotCopied];
    slotToRow[slot] = 0;

    if (sourceDataSaveFunct != nullptr)
        sourceDataSaveFunct();
}

// CC mode
void cPerformEngine::setCCValue(uint8_t value, uint8_t slot)
{
    constexpr uint8_t CC_VALUE_MIN = 0;
    constexpr uint8_t CC_VALUE_MAX = 127;

    const fxSetup* slotFxSetup = fxList[slotToFx[slot]]->setup;

    slotToCCValue[slot] =
      mapClamped<int>(value, CC_VALUE_MIN, CC_VALUE_MAX, slotFxSetup->min, slotFxSetup->max);
    setCCMode(slot);
}
void cPerformEngine::setCCMode(uint8_t slot)
{
    slotToActiveCC[slot] = 1;
}

void cPerformEngine::clearCC(uint8_t slot)
{
    slotToCCValue[slot] = 0;
    clearCCMode(slot);
}
void cPerformEngine::clearCCMode(uint8_t slot)
{
    slotToActiveCC[slot] = 0;
}
