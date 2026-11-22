#ifndef TRACKER_PLUGIN_H_
#define TRACKER_PLUGIN_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PTFX_MAGIC 0x58465450 // "PTFX" in Little Endian

// Plugin Type & Capabilities Flags
typedef enum {
    PTFX_FLAG_EFFECT          = 0x0000, // Transforms existing audio (in_buffer -> out_buffer)
    PTFX_FLAG_GENERATOR       = 0x0001, // Generates audio from scratch into out_buffer
    PTFX_FLAG_STEREO          = 0x0002, // Supports dual-channel processing (Tracker Mini/+)
    PTFX_FLAG_WAVETABLE       = 0x0004, // Specialized Polyend Tracker wavetable generator
    PTFX_FLAG_CONVERT_TO_MONO = 0x0008, // Converts stereo instrument to true mono (reclaims 50% RAM)
    PTFX_FLAG_IMX_ONLY        = 0x0010  // Code > 32 KB or requires Cortex-M7 / stereo
} PtfxFlags;

typedef enum {
    PARAM_FLOAT     = 'f', // Rendered as decimal (e.g. 1.25)
    PARAM_INT       = 'd', // Rendered as integer (e.g. 42)
    PARAM_LIST      = 'l', // Rendered as scrollable string list (cList)
    PARAM_FILE      = 'w', // Opens GlobalFileBrowser to pick a .wav (or custom extension in unit[])
    PARAM_SAVE_FILE = 's', // Opens GlobalFileBrowser in Save mode to pick an output file path
    PARAM_TEXT      = 't'  // Opens on-screen keyboard to enter text (up to 64 chars)
} PtfxParamDisplayType;

// Exactly 32 bytes; all float fields are 4-byte aligned (offsets +8, +12, +16, +20)
// When display_type == PARAM_LIST ('l'):
//   unit[0..1] = uint16_t list_offset (little-endian offset into code payload)
//   unit[2]    = uint8_t  item_count  (number of null-terminated option strings)
// When display_type == PARAM_TEXT ('t'):
//   unit[0..1] = uint16_t default_text_offset (little-endian offset into code payload)
//   unit[2]    = uint8_t  max_len             (max editable chars, 1..64; 0 = 64)
typedef struct __attribute__((packed, aligned(4))) {
    char    label[8];      // +0x00: Knob label (max 7 chars + '\0', e.g. "Drive\0")
    float   min_val;       // +0x08: Minimum value (4-byte aligned!)
    float   max_val;       // +0x0C: Maximum value (4-byte aligned!)
    float   default_val;   // +0x10: Initial load value (4-byte aligned!)
    float   step;          // +0x14: Value delta per encoder tick (4-byte aligned!)
    char    unit[6];       // +0x18: Unit string (max 5 chars + '\0') or list_offset/item_count
    uint8_t display_type;  // +0x1E: 'f', 'd', 'l', or 'w' (PtfxParamDisplayType)
    uint8_t padding;       // +0x1F: Pad struct to 32 bytes
} PtfxParamDef;

// Exactly 472 bytes (88-byte prefix + 12 * 32-byte params)
typedef struct __attribute__((packed, aligned(4))) {
    uint32_t     magic;                 // +0x00: Must equal PTFX_MAGIC ("PTFX")
    uint16_t     format_version;        // +0x04: Current version: 1
    uint16_t     flags;                 // +0x06: Bitfield of PtfxFlags
    char         display_name[32];      // +0x08: Pretty UI Name: e.g. "Stereo to Mono (Save RAM)\0"
    char         author[16];            // +0x28: Author/handle: e.g. "Polyend\0"
    uint8_t      num_params;            // +0x38: Number of active parameters (0 to 12)
    uint8_t      reserved[3];           // +0x39: Reserved / 4-byte alignment padding
    uint32_t     scratch_size;          // +0x3C: Requested working RAM in bytes (delays, reverbs, tables)
    uint32_t     code_offset;           // +0x40: Byte offset in file where machine code starts (472)
    uint32_t     code_size;             // +0x44: Byte size of binary payload (.text + .rodata + .data)
    uint32_t     bss_size;              // +0x48: Byte size of zero-initialized .bss following code_size
    uint32_t     init_offset;           // +0x4C: Byte offset of plugin_init() relative to code base
    uint32_t     set_param_offset;      // +0x50: Byte offset of plugin_set_param() relative to code base
    uint32_t     process_block_offset;  // +0x54: Byte offset of plugin_process_block() relative to code base
    PtfxParamDef params[12];            // +0x58: Up to 12 physical encoder parameters (384 bytes)
} PtfxHeader;

#ifdef __cplusplus
static_assert(sizeof(PtfxParamDef) == 32, "PtfxParamDef must be 32 bytes for FPU alignment");
static_assert(sizeof(PtfxHeader) == 472, "PtfxHeader must be 472 bytes");
#endif

// Host API Function Pointer Table
typedef struct {
    uint32_t sample_rate; // 44100
    // Standard Math (Single-Precision Hardware FPU)
    float (*sinf)(float);
    float (*cosf)(float);
    float (*tanf)(float);
    float (*tanhf)(float);
    float (*powf)(float, float);
    float (*expf)(float);
    float (*sqrtf)(float);
    float (*logf)(float);
    float (*log10f)(float);
    float (*fminf)(float, float);
    float (*fmaxf)(float, float);
    float (*fabsf)(float);
    // Memory & Utility Primitives
    void* (*memcpy)(void*, const void*, size_t);
    void* (*memset)(void*, int, size_t);
    int   (*rand)(void);
    // Optional Sample Length Extension (prepend / append samples to selection)
    void  (*set_extra_samples)(uint32_t prepend_samples, uint32_t append_samples);
    // File I/O & Secondary Sample Access (for convolution, vocoder, morphing, presets)
    int32_t (*read_file)(const char* path, void* buffer, uint32_t max_bytes);
    int32_t (*write_file)(const char* path, const void* buffer, uint32_t num_bytes);
    const int16_t* (*get_secondary_sample)(uint32_t* out_frames, uint8_t* out_channels);
    const char* (*get_selected_file_path)(uint8_t param_index);
    const char* (*get_text_param)(uint8_t param_index);
} PtfxHostApi;

// Plugin Function Pointer Types
typedef void (*PtfxInitFunc)(void* scratch_mem, uint32_t scratch_size, const PtfxHostApi* host);
typedef void (*PtfxSetParamFunc)(uint8_t index, float value);
typedef int32_t (*PtfxProcessBlockFunc)(const int16_t** in, int16_t** out, uint32_t samples, uint8_t channels);

#ifdef __cplusplus
}
#endif

#endif /* TRACKER_PLUGIN_H_ */
