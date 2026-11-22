#include "mtEffectModular.h"
#include "mtStructs.h"
#include "fileManager.h"
#include <cstring>
#include <cstdio>

mtEffectModular sampleEditorModular;

mtEffectModular::mtEffectModular()
{
}

void mtEffectModular::configurePlugin(const PtfxHeader& hdr,
                                     void* scratchMem,
                                     uint32_t maxScratchSize,
                                     const PtfxHostApi* hostApi,
                                     PtfxInitFunc initFunc,
                                     PtfxSetParamFunc setParamFunc,
                                     PtfxProcessBlockFunc processBlockFunc)
{
    m_flags            = hdr.flags;
    m_targetLength     = 88200;
    m_prependSamples   = 0;
    m_appendSamples    = 0;
    m_inputLength      = 0;
    m_scratchMem       = scratchMem;
    m_scratchSize      = (hdr.scratch_size <= maxScratchSize) ? hdr.scratch_size : maxScratchSize;
    m_hostApi          = hostApi;
    m_initFunc         = initFunc;
    m_setParamFunc     = setParamFunc;
    m_processBlockFunc = processBlockFunc;

    processingWithoutCopying = (m_flags & PTFX_FLAG_GENERATOR) != 0;

    for (int idx = 0; idx < kChannels; idx++)
    {
        if (confirmed[idx].area.addr == nullptr || processed[idx].area.addr == nullptr)
        {
            int16_t* confirmedBuffer = externalRamEffectsBuff + idx * EDIT_BUFF_SIZE;
            int16_t* processedBuffer =
              externalRamEffectsBuff + idx * EDIT_BUFF_SIZE + EDIT_BUFF_SIZE / 2;

            confirmed[idx].area.addr = confirmedBuffer;
            confirmed[idx].selection.addr = confirmedBuffer;
            processed[idx].area.addr = processedBuffer;
            processed[idx].selection.addr = processedBuffer;
        }
    }

    if (m_scratchMem && m_scratchSize > 0)
        memset(m_scratchMem, 0, m_scratchSize);

    if (m_initFunc)
        m_initFunc(m_scratchMem, m_scratchSize, m_hostApi);

    // Reset per-plugin processing state while preserving loaded/confirmed sample buffers
    clearProcessedData();
    operationType = OperationType::operationTypeIdle;
    processing.processParams.state = false;
    processing.processParams.currentProgressValue = 0;
    processing.isApplyOnEnd = false;
    m_processedSamples = 0;
    m_state = false;

    for (uint8_t i = 0; i < 12; i++)
    {
        if (i < hdr.num_params)
            m_paramValues[i] = hdr.params[i].default_val;
        else
            m_paramValues[i] = 0.0f;
    }
}

void mtEffectModular::unloadPlugin()
{
    m_initFunc         = nullptr;
    m_setParamFunc     = nullptr;
    m_processBlockFunc = nullptr;
    m_state            = false;
    operationType      = OperationType::operationTypeIdle;
    processing.processParams.state = false;
    processing.isApplyOnEnd        = false;
}

void mtEffectModular::setParameter(float& value, int idx)
{
    if (idx >= 0 && idx < 12)
    {
        m_paramValues[idx] = value;
        if (m_setParamFunc)
            m_setParamFunc((uint8_t)idx, value);
    }
}

uint32_t mtEffectModular::getExpectedProcessLength()
{
    bool isGenerator = (m_flags & PTFX_FLAG_GENERATOR) != 0;
    if (isGenerator)
    {
        uint32_t requested = m_prependSamples + m_appendSamples;
        if (requested > 0)
            return requested;
        return (m_targetLength > 0) ? m_targetLength : 44100;
    }
    return confirmed[0].selection.length + m_prependSamples + m_appendSamples;
}

bool mtEffectModular::startProcess()
{
#ifndef PLATFORM_IMX
    // Guard against executing IMX-only plugins on MK66
    if (m_flags & PTFX_FLAG_IMX_ONLY)
        return false;
#endif
    if (!m_processBlockFunc)
        return false;

    bool isGenerator = (m_flags & PTFX_FLAG_GENERATOR) != 0;

    if (m_scratchMem && m_scratchSize > 0)
        memset(m_scratchMem, 0, m_scratchSize);

    m_prependSamples = 0;
    m_appendSamples  = 0;

    if (m_initFunc)
        m_initFunc(m_scratchMem, m_scratchSize, m_hostApi);

    // Re-apply current parameter values after plugin_init()
    if (m_setParamFunc)
    {
        for (uint8_t i = 0; i < 12; i++)
            m_setParamFunc(i, m_paramValues[i]);
    }

    m_inputLength = isGenerator ? 0 : confirmed[0].selection.length;
    m_length = getExpectedProcessLength();
    if (m_length == 0)
        return false;

    m_processedSamples = 0;
    m_state = true;

    for (int ch = 0; ch < kChannels; ch++)
    {
        m_srcAddr[ch] = isGenerator ? nullptr : confirmed[ch].selection.addr;
        if (!isGenerator && m_srcAddr[ch] == nullptr)
        {
            m_srcAddr[ch] = confirmed[0].selection.addr;
        }
        m_dstAddr[ch] = (processed[ch].selection.addr != nullptr) ? processed[ch].selection.addr : processed[ch].area.addr;
    }
    return true;
}

int32_t mtEffectModular::processBlock()
{
    if (!m_state)
        return -1;

    bool isGenerator = (m_flags & PTFX_FLAG_GENERATOR) != 0;
    uint32_t remaining = m_length - m_processedSamples;
    uint32_t blockSize = remaining > 8192 ? 8192 : remaining;

    const int16_t* inPtrs[kChannels] = { nullptr };

    if (!isGenerator)
    {
        uint32_t bodyStart = m_prependSamples;
        uint32_t bodyEnd   = m_prependSamples + m_inputLength;

        if (m_processedSamples < bodyStart)
        {
            uint32_t toBody = bodyStart - m_processedSamples;
            if (blockSize > toBody)
                blockSize = toBody;
        }
        else if (m_processedSamples < bodyEnd)
        {
            uint32_t toAppend = bodyEnd - m_processedSamples;
            if (blockSize > toAppend)
                blockSize = toAppend;
            uint32_t inOffset = m_processedSamples - bodyStart;
            for (int ch = 0; ch < kChannels; ch++)
            {
                const int16_t* base = (confirmed[ch].selection.addr != nullptr)
                                        ? confirmed[ch].selection.addr
                                        : confirmed[0].selection.addr;
                inPtrs[ch] = base + inOffset;
            }
        }
    }

    // Process block across active channels
    int32_t count = m_processBlockFunc(
        isGenerator ? m_srcAddr : inPtrs,
        m_dstAddr,
        blockSize,
        kChannels
    );

    if (count <= 0)
    {
        m_state = false;
    }
    else
    {
        for (int ch = 0; ch < kChannels; ch++)
        {
            m_dstAddr[ch] += count;
        }
        m_processedSamples += count;

        if (m_processedSamples >= m_length)
        {
            m_state = false;
        }
    }

    if (!m_state && m_processedSamples > 0)
    {
        bool wasEmpty = (confirmed[0].area.length == 0);
        bool isGenerator = (m_flags & PTFX_FLAG_GENERATOR) != 0;
        for (int ch = 0; ch < kChannels; ch++)
        {
            processed[ch].selection.length = m_processedSamples;
            if (wasEmpty || isGenerator)
            {
                processed[ch].area.length = m_processedSamples;
            }
            else
            {
                processed[ch].area.length = confirmed[ch].area.length
                    + (int32_t)processed[ch].selection.length
                    - (int32_t)confirmed[ch].selection.length;
            }
        }
    }

    return (count > 0) ? count : 0;
}

void mtEffectModular::onApplyCommitted(int instrumentIdx)
{
    if (instrumentIdx >= 0)
    {
        if ((m_flags & PTFX_FLAG_GENERATOR))
        {
            if (mtProject.instrument[instrumentIdx].isActive == 0)
            {
                newFileManager.setDefaultActiveInstrument(&mtProject.instrument[instrumentIdx]);
                snprintf(mtProject.instrument[instrumentIdx].sample.file_name, SAMPLE_NAME_SIZE, "Gen %02d", instrumentIdx + 1);
            }
            mtProject.instrument[instrumentIdx].isActive = 1;
            mtProject.instrument[instrumentIdx].sample.length = getLengthToPolyphonicPlay();
        }
        if ((m_flags & PTFX_FLAG_WAVETABLE))
        {
            mtProject.instrument[instrumentIdx].playMode = playModeWavetable;
        }
    }
}

bool mtEffectModular::getProcessState()
{
    return m_state;
}
