#include "ptfx_loader.h"
#include "mtEffectModular.h"
#include "Platform.h"
#include "TrackerImpl.h"
#include "mtAudioEngine.h"
#include "mtStructs.h"

#include <cmath>
#include <cstring>
#include <cstdlib>

extern uint8_t externalRamScratchBuff[];

static uint32_t s_secondarySampleFrames = 0;
static uint8_t s_secondarySampleChannels = 0;
static char s_selectedFilePaths[12][kMaxFilePathLen + 1]{};

static FileSystem* getFileSystem()
{
    TrackerImpl* tracker = TrackerImpl::getInstance();
    if (!tracker)
        return nullptr;
    Platform* platform = tracker->getPlatform();
    if (!platform)
        return nullptr;
    return platform->getFileSystem();
}

static void hostSetExtraSamples(uint32_t prependSamples, uint32_t appendSamples)
{
    sampleEditorModular.setExtraSamples(prependSamples, appendSamples);
}

static int32_t hostReadFile(const char* path, void* buffer, uint32_t maxBytes)
{
    FileSystem* fs = getFileSystem();
    if (!fs || !path || !buffer || maxBytes == 0)
        return -1;

    auto [isOpen, fileHandle] = fs->openFile(path, FileSystem::FileOpenMode::kRead);
    if (!isOpen)
        return -1;

    uint8_t* dst = static_cast<uint8_t*>(buffer);
    uint32_t totalRead = 0;
    while (totalRead < maxBytes)
    {
        uint32_t chunk = maxBytes - totalRead;
        if (chunk > 256)
            chunk = 256;
        int32_t n = fs->readFile(dst + totalRead, chunk, fileHandle);
        if (n <= 0)
            break;
        totalRead += static_cast<uint32_t>(n);
        if (static_cast<uint32_t>(n) < chunk)
            break;
    }
    fs->closeFile(fileHandle);
    return static_cast<int32_t>(totalRead);
}

static int32_t hostWriteFile(const char* path, const void* buffer, uint32_t numBytes)
{
    FileSystem* fs = getFileSystem();
    if (!fs || !path || !buffer)
        return -1;

    if (fs->exists(path))
    {
        fs->remove(path);
    }

    auto [isOpen, fileHandle] = fs->openFile(path, FileSystem::FileOpenMode::kWriteCreateNew);
    if (!isOpen)
        return -1;

    const uint8_t* src = static_cast<const uint8_t*>(buffer);
    uint32_t totalWritten = 0;
    while (totalWritten < numBytes)
    {
        uint32_t chunk = numBytes - totalWritten;
        if (chunk > 256)
            chunk = 256;
        int32_t n = fs->writeFile(src + totalWritten, chunk, fileHandle);
        if (n <= 0)
            break;
        totalWritten += static_cast<uint32_t>(n);
        if (static_cast<uint32_t>(n) < chunk)
            break;
    }
    fs->closeFile(fileHandle);
    return static_cast<int32_t>(totalWritten);
}

static const int16_t* hostGetSecondarySample(uint32_t* outFrames, uint8_t* outChannels)
{
    return PtfxLoader::getSecondarySample(outFrames, outChannels);
}

static const char* hostGetSelectedFilePath(uint8_t paramIndex)
{
    return PtfxLoader::getSelectedFilePath(paramIndex);
}

static const char* hostGetTextParam(uint8_t paramIndex)
{
    return PtfxLoader::getSelectedFilePath(paramIndex);
}

// Host API Function Pointer Table
static const PtfxHostApi s_ptfxHostApi = {
    .sample_rate             = 44100,
    .sinf                    = ::sinf,
    .cosf                    = ::cosf,
    .tanf                    = ::tanf,
    .tanhf                   = ::tanhf,
    .powf                    = ::powf,
    .expf                    = ::expf,
    .sqrtf                   = ::sqrtf,
    .logf                    = ::logf,
    .log10f                  = ::log10f,
    .fminf                   = ::fminf,
    .fmaxf                   = ::fmaxf,
    .fabsf                   = ::fabsf,
    .memcpy                  = ::memcpy,
    .memset                  = ::memset,
    .rand                    = ::rand,
    .set_extra_samples       = hostSetExtraSamples,
    .read_file               = hostReadFile,
    .write_file              = hostWriteFile,
    .get_secondary_sample    = hostGetSecondarySample,
    .get_selected_file_path  = hostGetSelectedFilePath,
    .get_text_param          = hostGetTextParam,
};

bool PtfxLoader::loadSecondaryWavFile(const char* filepath)
{
    s_secondarySampleFrames = 0;
    s_secondarySampleChannels = 0;

    FileSystem* fs = getFileSystem();
    if (!fs || !filepath || filepath[0] == '\0')
        return false;

    auto [isOpen, fileHandle] = fs->openFile(filepath, FileSystem::FileOpenMode::kRead);
    if (!isOpen)
        return false;

    uint8_t riffHdr[12]{};
    if (fs->readFile(riffHdr, 12, fileHandle) != 12 ||
        memcmp(riffHdr, "RIFF", 4) != 0 || memcmp(riffHdr + 8, "WAVE", 4) != 0)
    {
        fs->closeFile(fileHandle);
        return false;
    }

    uint16_t audioFormat = 0;
    uint16_t numChannels = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataSize = 0;
    bool foundFmt = false;
    bool foundData = false;

    for (int chunkIter = 0; !foundData && chunkIter < 32; ++chunkIter)
    {
        uint8_t chunkHdr[8]{};
        if (fs->readFile(chunkHdr, 8, fileHandle) != 8)
            break;

        uint32_t chunkSize = 0;
        memcpy(&chunkSize, chunkHdr + 4, 4);

        if (memcmp(chunkHdr, "fmt ", 4) == 0)
        {
            if (chunkSize < 16 || chunkSize > 0x7FFFFFFEu)
            {
                fs->closeFile(fileHandle);
                return false;
            }
            uint8_t fmtBuf[16]{};
            if (fs->readFile(fmtBuf, 16, fileHandle) != 16)
            {
                fs->closeFile(fileHandle);
                return false;
            }
            memcpy(&audioFormat, fmtBuf + 0, 2);
            memcpy(&numChannels, fmtBuf + 2, 2);
            memcpy(&bitsPerSample, fmtBuf + 14, 2);
            foundFmt = true;

            uint32_t rem = ((chunkSize + 1u) & ~1u) - 16u;
            if (rem > 0 && !fs->seekFileCur(static_cast<int>(rem), fileHandle))
            {
                fs->closeFile(fileHandle);
                return false;
            }
        }
        else if (memcmp(chunkHdr, "data", 4) == 0)
        {
            dataSize = chunkSize;
            foundData = true;
        }
        else
        {
            if (chunkSize > 0x7FFFFFFEu)
            {
                fs->closeFile(fileHandle);
                return false;
            }
            uint32_t paddedSize = (chunkSize + 1u) & ~1u;
            if (paddedSize > 0 && !fs->seekFileCur(static_cast<int>(paddedSize), fileHandle))
            {
                fs->closeFile(fileHandle);
                return false;
            }
        }
    }

    const bool supportedFormat =
      (audioFormat == 1 && (bitsPerSample == 8 || bitsPerSample == 16 ||
                            bitsPerSample == 24 || bitsPerSample == 32)) ||
      (audioFormat == 3 && bitsPerSample == 32);

    if (!foundFmt || !foundData || !supportedFormat ||
        (numChannels != 1 && numChannels != 2) || dataSize == 0)
    {
        fs->closeFile(fileHandle);
        return false;
    }

    uint32_t srcSampleBytes = bitsPerSample / 8u;
    uint32_t frameBytes = static_cast<uint32_t>(numChannels) * srcSampleBytes;
    if (frameBytes == 0 || frameBytes > 8)
    {
        fs->closeFile(fileHandle);
        return false;
    }

    uint32_t maxFrames = kPtfxSecondarySampleBytes / (numChannels * sizeof(int16_t));
    uint32_t frameCount = dataSize / frameBytes;
    if (frameCount > maxFrames)
    {
        frameCount = maxFrames;
    }
    if (frameCount == 0)
    {
        fs->closeFile(fileHandle);
        return false;
    }

    auto* dst = reinterpret_cast<int16_t*>(externalRamScratchBuff + kPtfxSecondarySampleOffset);
    uint8_t tmp[256];
    uint32_t framesPerChunk = static_cast<uint32_t>(sizeof(tmp)) / frameBytes;
    uint32_t framesRead = 0;

    while (framesRead < frameCount)
    {
        uint32_t toRead = frameCount - framesRead;
        if (toRead > framesPerChunk)
            toRead = framesPerChunk;

        uint32_t chunkBytes = toRead * frameBytes;
        int32_t n = fs->readFile(tmp, chunkBytes, fileHandle);
        if (n <= 0)
            break;

        uint32_t gotFrames = static_cast<uint32_t>(n) / frameBytes;
        if (gotFrames == 0)
            break;

        for (uint32_t f = 0; f < gotFrames; ++f)
        {
            for (uint16_t ch = 0; ch < numChannels; ++ch)
            {
                const uint8_t* p = tmp + f * frameBytes + ch * srcSampleBytes;
                int32_t sample = 0;
                if (audioFormat == 1 && bitsPerSample == 8)
                {
                    sample = (static_cast<int32_t>(p[0]) - 128) << 8;
                }
                else if (audioFormat == 1 && bitsPerSample == 16)
                {
                    int16_t s16 = 0;
                    memcpy(&s16, p, 2);
                    sample = s16;
                }
                else if (audioFormat == 1 && bitsPerSample == 24)
                {
                    int32_t s24 = (static_cast<int32_t>(p[2]) << 24) |
                                  (static_cast<int32_t>(p[1]) << 16) |
                                  (static_cast<int32_t>(p[0]) << 8);
                    sample = s24 >> 16;
                }
                else if (audioFormat == 1 && bitsPerSample == 32)
                {
                    int32_t s32 = 0;
                    memcpy(&s32, p, 4);
                    sample = s32 >> 16;
                }
                else if (audioFormat == 3 && bitsPerSample == 32)
                {
                    float flt = 0.0f;
                    memcpy(&flt, p, 4);
                    if (flt < -1.0f)
                        flt = -1.0f;
                    else if (flt > 1.0f)
                        flt = 1.0f;
                    sample = static_cast<int32_t>(flt * 32767.0f);
                }

                if (sample < -32768)
                    sample = -32768;
                else if (sample > 32767)
                    sample = 32767;

                dst[(framesRead + f) * numChannels + ch] = static_cast<int16_t>(sample);
            }
        }
        framesRead += gotFrames;
        if (gotFrames < toRead)
            break;
    }

    fs->closeFile(fileHandle);

    s_secondarySampleFrames = framesRead;
    s_secondarySampleChannels = static_cast<uint8_t>(numChannels);
    return s_secondarySampleFrames > 0;
}

const int16_t* PtfxLoader::getSecondarySample(uint32_t* outFrames, uint8_t* outChannels)
{
    if (outFrames)
        *outFrames = s_secondarySampleFrames;
    if (outChannels)
        *outChannels = s_secondarySampleChannels;
    if (s_secondarySampleFrames == 0)
        return nullptr;
    return reinterpret_cast<const int16_t*>(externalRamScratchBuff + kPtfxSecondarySampleOffset);
}

void PtfxLoader::setSelectedFilePath(uint8_t paramIndex, const char* filepath)
{
    if (paramIndex >= 12)
        return;
    if (!filepath)
    {
        s_selectedFilePaths[paramIndex][0] = '\0';
        return;
    }
    strncpy(s_selectedFilePaths[paramIndex], filepath, kMaxFilePathLen);
    s_selectedFilePaths[paramIndex][kMaxFilePathLen] = '\0';
}

const char* PtfxLoader::getSelectedFilePath(uint8_t paramIndex)
{
    if (paramIndex >= 12)
        return "";
    return s_selectedFilePaths[paramIndex];
}

void PtfxLoader::clearSelectedFiles()
{
    s_secondarySampleFrames = 0;
    s_secondarySampleChannels = 0;
    memset(s_selectedFilePaths, 0, sizeof(s_selectedFilePaths));
}

uint8_t* PtfxLoader::getCodeBuffer()
{
    // First 128 KB (IMX) / 32 KB (MK66) of externalRamScratchBuff
    return externalRamScratchBuff;
}

uint32_t PtfxLoader::getMaxCodeSize()
{
#ifdef PLATFORM_IMX
    return kPtfxCodeBufferSize; // 128 KB
#else
    return 32 * 1024; // 32 KB
#endif
}

void* PtfxLoader::getScratchBuffer()
{
    // 640 KB of externalRamScratchBuff ([128 KB .. 768 KB], preserving code buffer)
    return reinterpret_cast<void*>(externalRamScratchBuff + kPtfxScratchOffset);
}

uint32_t PtfxLoader::getMaxScratchSize()
{
    return kPtfxScratchTopOffset - kPtfxScratchOffset; // 655,360 bytes (640 KB)
}

const PtfxHostApi* PtfxLoader::getHostApi()
{
    return &s_ptfxHostApi;
}

#ifdef PLATFORM_IMX
static void cleanInvalidateDCache(void* addr, uint32_t size)
{
    // SCB_BASE (0xE000ED00) + 0x270 = 0xE000EF70 (SCB->DCCIMVAC)
    volatile uint32_t* const scb_dccimvac = reinterpret_cast<volatile uint32_t*>(0xE000EF70);
    uintptr_t p   = reinterpret_cast<uintptr_t>(addr) & ~static_cast<uintptr_t>(0x1F);
    uintptr_t end = reinterpret_cast<uintptr_t>(addr) + size;

    asm volatile("dsb 0xf" ::: "memory");
    while (p < end)
    {
        *scb_dccimvac = static_cast<uint32_t>(p);
        p += 32;
    }
    asm volatile("dsb 0xf" ::: "memory");
    asm volatile("isb 0xf" ::: "memory");
}
#endif

void PtfxLoader::syncInstructionCache([[maybe_unused]] void* addr, [[maybe_unused]] uint32_t size)
{
    asm volatile("dsb 0xf" ::: "memory");
    asm volatile("isb 0xf" ::: "memory");

#ifdef PLATFORM_IMX
    // Clean and Invalidate D-Cache to Point of Coherency (SDRAM) and invalidate entire I-Cache on Cortex-M7
    // SCB_BASE (0xE000ED00) + 0x270 = 0xE000EF70 (SCB->DCCIMVAC)
    // SCB_BASE (0xE000ED00) + 0x250 = 0xE000EF50 (SCB->ICIALLU)
    volatile uint32_t* const scb_dccimvac = reinterpret_cast<volatile uint32_t*>(0xE000EF70);
    volatile uint32_t* const scb_iciallu  = reinterpret_cast<volatile uint32_t*>(0xE000EF50);

    uintptr_t p   = reinterpret_cast<uintptr_t>(addr) & ~static_cast<uintptr_t>(0x1F);
    uintptr_t end = reinterpret_cast<uintptr_t>(addr) + size;

    while (p < end)
    {
        *scb_dccimvac = static_cast<uint32_t>(p);
        p += 32;
    }

    asm volatile("dsb 0xf" ::: "memory");
    *scb_iciallu = 0; // Invalidate all instruction cache
    asm volatile("dsb 0xf" ::: "memory");
    asm volatile("isb 0xf" ::: "memory");
#else
    // Disable NXP System MPU on MK66 to allow instruction execution from SRAM/SDRAM
    volatile uint32_t* const mpu_cesr = reinterpret_cast<volatile uint32_t*>(0x4000D000);
    *mpu_cesr = 0;
    // Push and invalidate Kinetis LMEM Processor Code Bus Cache
    volatile uint32_t* const lmem_pcccr = reinterpret_cast<volatile uint32_t*>(0xE0082000);
    *lmem_pcccr = 0x8F000003;
    asm volatile("dsb 0xf" ::: "memory");
    asm volatile("isb 0xf" ::: "memory");
#endif
}

bool PtfxLoader::readHeader(const char* filepath, PtfxHeader& outHeader)
{
    FileSystem* fs = getFileSystem();
    if (!filepath || !fs)
        return false;

    auto [isOpen, fileHandle] = fs->openFile(filepath, FileSystem::FileOpenMode::kRead);
    if (!isOpen)
        return false;

    alignas(32) PtfxHeader tempHeader;
    int32_t bytesRead = fs->readFile(&tempHeader, sizeof(PtfxHeader), fileHandle);
    fs->closeFile(fileHandle);

    if (bytesRead != static_cast<int32_t>(sizeof(PtfxHeader)))
        return false;

    if (tempHeader.magic != PTFX_MAGIC || tempHeader.format_version != 1)
        return false;

    memcpy(&outHeader, &tempHeader, sizeof(PtfxHeader));
    return true;
}

bool PtfxLoader::loadPlugin(const char* filepath,
                           PtfxHeader& outHeader,
                           PtfxInitFunc& outInit,
                           PtfxSetParamFunc& outSetParam,
                           PtfxProcessBlockFunc& outProcessBlock)
{
    outInit         = nullptr;
    outSetParam     = nullptr;
    outProcessBlock = nullptr;

    FileSystem* fs = getFileSystem();
    if (!filepath || !fs)
        return false;

    auto [isOpen, fileHandle] = fs->openFile(filepath, FileSystem::FileOpenMode::kRead);
    if (!isOpen)
        return false;

    // Read PtfxHeader in <= 256-byte chunks (e.g. 256 + 216 = 472 bytes)
    size_t headerBytesRead = 0;
    uint8_t* headerPtr = reinterpret_cast<uint8_t*>(&outHeader);
    while (headerBytesRead < sizeof(PtfxHeader))
    {
        size_t chunk = sizeof(PtfxHeader) - headerBytesRead;
        if (chunk > 256)
            chunk = 256;
        int32_t n = fs->readFile(headerPtr + headerBytesRead, chunk, fileHandle);
        if (n != static_cast<int32_t>(chunk))
        {
            fs->closeFile(fileHandle);
            return false;
        }
        headerBytesRead += static_cast<size_t>(n);
    }

    if (outHeader.magic != PTFX_MAGIC || outHeader.format_version != 1)
    {
        fs->closeFile(fileHandle);
        return false;
    }

#ifndef PLATFORM_IMX
    // MK66 cannot run IMX-only plugins
    if (outHeader.flags & PTFX_FLAG_IMX_ONLY)
    {
        fs->closeFile(fileHandle);
        return false;
    }
#endif

    uint32_t totalCodeSize = outHeader.code_size + outHeader.bss_size;
    if (totalCodeSize > getMaxCodeSize())
    {
        fs->closeFile(fileHandle);
        return false;
    }

    if (!fs->seekFile(outHeader.code_offset, fileHandle))
    {
        fs->closeFile(fileHandle);
        return false;
    }

    uint8_t* codeBuf = getCodeBuffer();
    engine.getSdPlayer()->stop();

#ifdef PLATFORM_IMX
    // Clean and invalidate D-Cache across the executable buffer before loading
    cleanInvalidateDCache(codeBuf, getMaxCodeSize());
#endif

    // Read in <= 256-byte chunks so FatFs buffers sectors in internal DTCM (fp->buf)
    // and copies via CPU rather than performing unaligned DMA directly into SDRAM.
    size_t bytesRead = 0;
    while (bytesRead < outHeader.code_size)
    {
        size_t chunk = outHeader.code_size - bytesRead;
        if (chunk > 256)
            chunk = 256;
        int32_t n = fs->readFile(codeBuf + bytesRead, chunk, fileHandle);
        if (n != static_cast<int32_t>(chunk))
            break;
        bytesRead += static_cast<size_t>(n);
    }
    fs->closeFile(fileHandle);

    if (bytesRead != outHeader.code_size)
        return false;

    // Zero-initialize .bss region immediately after code payload
    if (outHeader.bss_size > 0)
    {
        memset(codeBuf + outHeader.code_size, 0, outHeader.bss_size);
    }

    // Synchronize cache: clean & invalidate D-cache to RAM, invalidate I-cache
    syncInstructionCache(codeBuf, totalCodeSize);

    // Compute ARM Cortex-M Thumb-2 entry points (bit 0 set to 1)
    uintptr_t base = reinterpret_cast<uintptr_t>(codeBuf);
    outInit         = reinterpret_cast<PtfxInitFunc>((base + outHeader.init_offset) | 0x1);
    outSetParam     = reinterpret_cast<PtfxSetParamFunc>((base + outHeader.set_param_offset) | 0x1);
    outProcessBlock = reinterpret_cast<PtfxProcessBlockFunc>((base + outHeader.process_block_offset) | 0x1);

    return true;
}
