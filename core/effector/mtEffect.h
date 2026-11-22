#ifndef EFFECTOR_MTEFFECT_H_
#define EFFECTOR_MTEFFECT_H_

#include "mtDataCopyier.h"
#include "mtStructs.h"
#include "polyassert.h"
#include "stdint.h"

extern int16_t* externalRamEffectsBuff;

class mtEffect
{
  public:
    static constexpr int kChannels = 2;
    enum Channel
    {
        kLeft,
        kRight
    };

    enum LoadingResult
    {
        loadingBusy,
        instrumentIsTooLong,
        instrumentIsNotActive,
        loadingSuccessful
    };

    mtEffect()
    {
        for (int idx = 0; idx < kChannels; idx++)
        {
            int16_t* confirmedBuffer = externalRamEffectsBuff + idx * EDIT_BUFF_SIZE;
            int16_t* processedBuffer =
              externalRamEffectsBuff + idx * EDIT_BUFF_SIZE + EDIT_BUFF_SIZE / 2;

            confirmed[idx] = { { confirmedBuffer, 0 }, { confirmedBuffer, 0 } };
            processed[idx] = { { processedBuffer, 0 }, { processedBuffer, 0 } };

            dataCopyier[idx] = new mtDataCopyier(LOAD_BLOCK_SIZE_IN_SAMPLES);
        }
    }

    struct UndoParams
    {
        bool isEnable = false;
    };

    virtual void setParameter(float& value, int idx) = 0;
    virtual bool startProcess() = 0;
    virtual int32_t processBlock() = 0;
    virtual bool getProcessState() = 0;
    virtual uint32_t getExpectedProcessLength() = 0;
    void update();
    void switchToThisEffect(mtEffect* lastEffect);
    void setBaseSampleLength(uint32_t val);
    LoadingResult startLoad(uint8_t instr_idx);
    bool getLoadingState();
    uint8_t getLoadingProgress();
    bool isDataLoaded();
    void clearLoadedData();
    void apply();
    void undo();
    bool processSelection();
    uint8_t getProcessingSelectionProgress();
    bool getProcessingSelectionState();
    bool isDataProcessed();
    void clearProcessedData();
    uint32_t getExpectedProcessedSampleLength();
    int16_t* const getSampleAddresToPreview(Channel channel = Channel::kLeft);
    uint32_t getSampleLengthToPreview();
    int16_t* getAddresToPolyphonicPlay(Channel channel = Channel::kLeft);
    uint32_t getLengthToPolyphonicPlay();
    void changeSelectionRange(uint16_t a, uint16_t b);
    uint16_t getNewStartPoint();
    uint16_t getNewEndPoint();
    UndoParams undoParams;

  protected:
    enum ProcessingState
    {
        idle,
        copyingBeforeProcessing,
        processingSelection,
        copyingAfterProcessing
    };

    enum OperationType
    {
        operationTypeIdle,
        operationTypeLoading,
        operationTypeProcessing,
    } operationType;

    struct ProcessingParams
    {
        bool state;
        uint32_t currentProgressValue;
        uint32_t maxProgressValue;
    };

    struct strProcessingParams
    {
        bool isLoadedData = false;
        bool isApplyOnEnd = false;
        ProcessingParams processParams;
    } processing;

    bool processingWithoutCopying;

    struct strMemoryAreaWithSelection
    {
        struct strMemoryArea
        {
            int16_t* addr;
            uint32_t length;
        } area, selection;
    };

    strMemoryAreaWithSelection confirmed[kChannels];
    strMemoryAreaWithSelection processed[kChannels];

  private:
    static constexpr uint32_t LOAD_BLOCK_SIZE_IN_SAMPLES = 8192;

    struct LoadingParams
    {
        struct PreloadPoints
        {
            uint16_t a;
            uint16_t b;
        } preloadPoints;
        bool isAnyOtherProcessingOnFinish = false;
        ProcessingParams processingParams;
    };

    struct SaveParams
    {
        int16_t* currentSrcAddr = nullptr;
        ProcessingParams processingParams;
    };

    struct ApplyParams
    {
        bool isProcessedData = false;
    };

    void updateCopying();
    void updateProcessing();
    void updateProcessingSelection();
    void updateLoading();
    bool getState(ProcessingParams* ptr);
    uint8_t getProgress(ProcessingParams* ptr);

    LoadingParams loadingParams;
    SaveParams saveParams;
    ApplyParams applyParams;
    mtDataCopyier* dataCopyier[kChannels];
    uint8_t processingState;
    bool endProcessingState = false;
};

#endif /* EFFECTOR_MTEFFECT_H_ */
