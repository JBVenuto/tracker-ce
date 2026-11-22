# Wavetable Envelopes (`WT Env`), Wavetable LFOs (`WT LFO`), and Envelope Step FX (`w` / `y`)

This guide explains how **Wavetable Envelopes (`WT Env`)**, **Wavetable LFOs (`WT LFO`)**, and the **Envelope Pattern Step FX (`w` and `y`)** work on the Polyend Tracker, Tracker Mini, and Tracker+, as well as how to load custom user wavetables from your SD card.

---

## 1. Getting Started: Selecting `WT Env` or `WT LFO`

1. Select any sample-based instrument and press **Instrument Parameters** twice to open **Instrument Automation (2/2)**.
2. In the first column (**Destination**), select any of the 7 modulation targets:
   - **Volume**
   - **Panning**
   - **Cutoff**
   - **Wavetable Position**
   - **Granular Position**
   - **Granular Length**
   - **Finetune**
3. Move to the second column (**Type**) and choose:
   - **`WT Env`** - One-shot or sustain-freezing multi-segment wavetable envelope triggered on each Note On.
   - **`WT LFO`** - Tempo-synced looping wavetable LFO that continuously cycles through the selected wavetable shape.

---

## 2. Parameter Reference

### `WT Env` (Wavetable Envelope)

| Column | Parameter | Range | Description |
| :--- | :--- | :--- | :--- |
| **3** | **Wave** | `F 001` – `F 128`<br>`U 001` – `U 128` | Selects one of 128 **Factory** (`F`) or 128 **User** (`U`) wavetable contours. Pressing **Up** (`[+]`) increases the wave number; **Down** (`[-]`) decreases it. |
| **4** | **Duration** | `0.000s` – `20.000s` | Total time for the envelope to travel from the start (`0%`) to the end (`100%`) of the wavetable contour. |
| **5** | **Warp X/Y** | `-7` to `+7` / `-7` to `+7` | **Dual-axis contour shaper** displayed as a split bipolar bar from the center zero line:<br>• **Left bar (`Warp X`, `-7..+7`):** Time-warps the horizontal phase axis. Negative values compress features toward the attack (fast start, drawn-out tail); positive values delay features toward the end (slow build, fast finish).<br>• **Right bar (`Warp Y`, `-7..+7`):** Bends the vertical amplitude curve. `+1..+7` boosts quiet/mid levels upward; `-1..-6` pinches levels downward for sharper transients; **`-7` inverts the entire contour upside down** (`1.0 - y`). |
| **6** | **Sustain** | `Off`, `1%` – `99%` | Controls how note length affects the envelope:<br>• **`Off` (`0`):** True **one-shot mode**. The contour plays through its full `Duration` regardless of when Note Off occurs.<br>• **`1%` – `99%`:** **Timeline freeze point**. While a note is held, the envelope plays up to the chosen percentage of the wavetable timeline and holds that exact level. When the note is released (Note Off), the envelope resumes from that point to the end of the wavetable. |
| **7** | **Amount** | `0%` – `100%` | Modulation depth applied to the destination parameter. |

#### Adjusting `Warp X/Y` in `WT Env`
Because `WT Env` packs both `Warp X` and `Warp Y` into Column 5 (`Warp X/Y`):
- **Original Tracker & Tracker+:**
  - **Warp X:** Turn the **Jog Wheel** or press **Arrow Up / Down**.
  - **Warp Y:** Hold **Shift + Jog Wheel** or **Shift + Arrow Up / Down**.
- **Tracker Mini:**
  - **Warp X:** Press **Arrow Up / Down** or **`[+]` / `[-]`**.
  - **Warp Y:** Hold **Shift + Arrow Up / Down** or press **`[++]` / `[--]`**.
- **Reset:** Pressing **Delete** on `Warp X/Y` resets both `Warp X` and `Warp Y` to `+0 / +0`.

---

### `WT LFO` (Wavetable LFO)

| Column | Parameter | Range | Description |
| :--- | :--- | :--- | :--- |
| **3** | **Wave** | `F 001` – `F 128`<br>`U 001` – `U 128` | Selects the wavetable shape cycled by the LFO. |
| **4** | **Speed** | `128 steps` – `1/64 step`<br>*(Volume: down to `1/6 step`)* | Tempo-synced LFO cycle rate in pattern steps. |
| **5** | **Warp X** | `-7` to `+7` | Horizontal time-skew of the LFO cycle (displayed as a center-zero bipolar bar). |
| **6** | **Warp Y** | `-7` to `+7` | Vertical amplitude bend of the LFO cycle (`-7` inverts the waveform upside down). |
| **7** | **Amount** | `0%` – `100%` | Modulation depth applied to the destination parameter. |

---

## 3. Pattern Step FX: `w - WT Volume Env` & `y - WT Filter Env`

Two pattern Step FX (located right after `J - Slide down` and before the MIDI CC / Macro effects `a`–`f`) allow per-step expressive control over the **Volume Envelope (`w - WT Volume Env`)** and **Filter Cutoff Envelope (`y - WT Filter Env`)** using a compact 2-digit hexadecimal value **`XY`** (`00` – `FF`), where:
- **`X`** (`0` – `F`, high hex digit) controls the first parameter.
- **`Y`** (`0` – `F`, low hex digit) controls the second parameter.

The behavior of `w` and `y` adapts automatically depending on whether the instrument's Volume or Cutoff automation Type is set to a standard **ADSR `Envelope`** or a **`WT Env`**:

### A. When Type is **`WT Env`** (`XY` = Duration Scale / Warp X Offset)
- **`X` (First Hex Digit, `0`–`F`):** Scales the `WT Env` **Duration** for that step (`8` = neutral `1.0×` instrument Duration):
  - `0` = `0.05×` (ultra-short percussive blip)
  - `2` = `0.19×` (~5× faster than default)
  - `4` = `0.36×` (~3× faster)
  - `6` = `0.62×`
  - **`8` = `1.00×` (unchanged instrument Duration)**
  - `A` = `1.44×`
  - `C` = `2.06×` (~2× longer)
  - `E` = `2.91×` (~3× longer)
  - `F` = `4.00×` (4× longer than instrument Duration)
- **`Y` (Second Hex Digit, `0`–`F`):** Offsets **Warp X** for that step (`8` = neutral `+0` offset):
  - `0` / `1` = `-7` (maximum skew toward start)
  - `4` = `-4`
  - **`8` = `+0` (unchanged instrument Warp X)**
  - `C` = `+4`
  - `F` = `+7` (maximum skew toward end)

> **Examples with `WT Env` on Cutoff (`y`):**
> - `y88` - Plays the `WT Env` with its exact programmed Duration (`1.0×`) and Warp X (`+0`).
> - `y28` - Plays the `WT Env` ~5× faster (`0.19×` Duration) with unchanged Warp X (`+0`).
> - `y82` - Keeps the programmed Duration (`1.0×`) but skews Warp X by `-6` for a snappier front-loaded contour.
> - `yC4` - Doubles the Duration (`2.06×`) and skews Warp X by `-4`.

### B. When Type is Standard **`Envelope` (ADSR)** (`XY` = Attack / Decay-Release)
- **`X` (First Hex Digit, `0`–`F`):** Sets the **Attack** time on an exponential curve from `0 ms` (`0`) up to `10,000 ms` (`F`).
- **`Y` (Second Hex Digit, `0`–`F`):** Sets the **Release** time on an exponential curve from `5 ms` (`0`) up to `10,000 ms` (`F`). Additionally, when **`X = 0`** (zero attack), `Y` also scales the **Decay** time so `w01`–`w0F` / `y01`–`y0F` can shape tight percussive plucks on a single step.

---

## 4. Factory (`F 001`–`F 128`) vs. User (`U 001`–`U 128`) Wavetables

The Tracker stores **256 wavetable shapes** (each represented as a 128-point, 16-bit unipolar contour in external SDRAM):

| UI Range | Slot Count | Source |
| :--- | :---: | :--- |
| **`F 001` - `F 128`** | 128 | **Factory Wavetables** - Built into the firmware (8 banks of 16 musical contours). |
| **`U 001` - `U 128`** | 128 | **User Wavetables** - Loaded from the `/Envelopes` folder on your SD card, with built-in fallback shapes for any slot that does not have a corresponding `.wav` file on disk. |

### Built-In Factory Banks (`F 001` – `F 128`)
- **`F 001` – `F 016`:** Exponential & logarithmic decays / percussive plucks
- **`F 017` – `F 032`:** Smooth swells, S-curves & Gaussian bells
- **`F 033` – `F 048`:** Double & multi-bounces (damped elastic arcs)
- **`F 049` – `F 064`:** Rhythmic gates & trance steppers (50% and 25% duty cycles)
- **`F 065` – `F 080`:** Sharks, fins & asymmetric triangles
- **`F 081` – `F 096`:** Ripples & exponentially damped oscillations
- **`F 097` – `F 112`:** Accelerating & decelerating trills (chirp gates)
- **`F 113` – `F 128`:** Staircases up & down

### Built-In Fallback Banks for User Slots (`U 001` – `U 128`)
So that `U 001` – `U 128` are immediately useful even when `/Envelopes` does not exist on the SD card (or when you only place a handful of custom `.wav` files), all 128 user slots are pre-initialized at boot with 8 additional procedural banks:
- **`U 001` – `U 016`:** West-Coast wavefolded contours
- **`U 017` – `U 032`:** Syncopated binary pulse patterns
- **`U 033` – `U 048`:** Reverse swells with snappy cutoffs
- **`U 049` – `U 064`:** Dual-peak camel / formant contours
- **`U 065` – `U 080`:** Growing chaos / rising frequency wobble
- **`U 081` – `U 096`:** Karplus-style rippled plucks
- **`U 097` – `U 112`:** Stepped sample-and-hold (S&H) contours
- **`U 113` – `U 128`:** Cinematic morphing risers

When you place a `.wav` file in `/Envelopes` for a given slot (e.g., `001.wav`), it overwrites `U 001` while leaving any other `U xxx` slots without `.wav` files on their built-in fallback shapes.

---

## 5. How to Add Your Own Wavetables in `/Envelopes`

### Step 1: Create the `/Envelopes` Folder on Your SD Card
At the root level of your Tracker SD card, create a folder named **`Envelopes`** (case-insensitive on FAT32):

```text
SD Card Root/
├── Envelopes/
│   ├── 001.wav      --> Loaded into U 001
│   ├── 002.wav      --> Loaded into U 002
│   ├── ...
│   └── 128.wav      --> Loaded into U 128
├── Firmware/
├── Projects/
└── Samples/
```

### Step 2: Name Your `.wav` Files
- **1-Based Numbering (Recommended - matches UI labels `U 001` to `U 128` directly):**
  - Use 3-digit zero-padded numbers: **`001.wav`** through **`128.wav`** (unpadded names like `1.wav` through `128.wav` also work).
  - **`001.wav`** maps to **`U 001`**, **`002.wav`** maps to **`U 002`**, ..., **`128.wav`** maps to **`U 128`**.
  - You can populate as few or as many slots as you like. For example, placing only `001.wav` and `005.wav` will replace `U 001` and `U 005`.
- **0-Based Numbering (Also Supported):**
  - If `/Envelopes/000.wav` (or `0.wav`) is present, the loader automatically uses 0-based indexing (`000.wav` $\rightarrow$ `U 001`, `001.wav` $\rightarrow$ `U 002`, ..., `127.wav` $\rightarrow$ `U 128`).

### Step 3: `.wav` File Technical Requirements
Your `.wav` files must meet the following specifications:
1. **File Format:** Uncompressed **PCM `.wav`** (`RIFF` / `WAVE`, AudioFormat = `1`).
   - *Note:* Compressed `.wav` formats or 32-bit IEEE float (`AudioFormat = 3`) are not supported; export as **16-bit or 24-bit integer PCM**.
2. **Bit Depth:** **8-bit**, **16-bit** (recommended), or **24-bit** integer PCM.
3. **Channels:** **Mono** (recommended) or **Stereo** (if stereo, the left channel is used).
4. **Sample Rate:** Any sample rate (`44100 Hz`, `48000 Hz`, `96000 Hz`, etc.). Because the Tracker stretches the waveform across your chosen envelope `Duration` or LFO `Speed`, the `.wav` header sample rate does not restrict playback speed.
5. **Sample Length:** Any length from **128 samples** up to several seconds:
   - **Single-cycle waveforms** (`128`, `256`, `512`, `1024`, or `2048` samples, such as *Adventure Kid Waveforms (AKWF)*, *Serum*, or *Vital* single-cycle exports) work directly.
   - **Drawn CV/envelope recordings** of any length are automatically resampled across 128 evenly spaced points (`0..127`).

### Step 4: Unipolar vs. Bipolar `.wav` Files (Automatic Detection)
Audio editors and synths export waveforms in one of two ways, and the Tracker automatically detects both:
- **Unipolar Contours (All samples $\ge 0$, from `0.0` to `+1.0`):**
  - Used by CV recorders and envelope generators.
  - Mapped directly: `0.0` = `0%` modulation, `+1.0` = `100%` modulation.
- **Standard Bipolar Audio Waveforms (Contains negative samples, from `-1.0` to `+1.0`):**
  - Used by standard audio editors and single-cycle synth wavetables.
  - Automatically shifted and normalized from `[-1.0, +1.0]` to `[0.0, 1.0]` (`-1.0` $\rightarrow$ `0%`, `0.0` $\rightarrow$ `50%`, `+1.0` $\rightarrow$ `100%`) so negative halves of audio waveforms are never clipped or rectified.

### Step 5: When Are User Wavetables Loaded?
- User wavetables from `/Envelopes` are loaded automatically from the SD card the first time a project uses or previews a **`U 001` – `U 128`** slot in a session.
- If you swap SD cards or add new files to `/Envelopes`, reboot the Tracker (or power-cycle) so the `/Envelopes` directory is rescanned cleanly.
