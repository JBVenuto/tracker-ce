# SD Card Image Viewer (`Config > Info`)

The **Polyend Tracker Community Edition (CE)** reactivates the hardware's built-in FT812 JPEG image viewer across all three devices (**Original Tracker**, **Tracker+**, and **Tracker Mini**) and expands it into a **3-folder user image viewer** accessible under **`Config > Info`**.

Use it to store hardware synth/MIDI CC charts, music theory and scale references, live setlists, track notes, or custom artwork right on your microSD card.

---

## 1. Accessing the Image Viewer

1. Open **Config** (`[Shift] + [Master]` or via the module menu).
2. Select **`Info`** in the left column.
3. Choose one of the four entries:
   - **`About`** - Displays the firmware version, license info, and credits.
   - **`Reference`** - Opens images from `/Images/Reference/`
   - **`Notes`** - Opens images from `/Images/Notes/`
   - **`Custom`** - Opens images from `/Images/Custom/`

---

## 2. SD Card Folder Structure & File Naming

Create an `Images` directory at the root of your microSD card with any of the three supported subfolders (`Reference`, `Notes`, `Custom`):

```text
SD Card Root/
└── Images/
    ├── Reference/
    │   ├── 1.jpg
    │   ├── 2.jpg
    │   └── ... (up to 48.jpg)
    ├── Notes/
    │   ├── 1.jpg
    │   └── ... (up to 48.jpg)
    └── Custom/
        ├── 1.jpg
        └── ... (up to 48.jpg)
```

### File Naming Rules
- Name your files **`1.jpg`** through **`48.jpg`** (or **`1.jpeg`** through **`48.jpeg`**).
- Each folder supports up to **48 images**, corresponding directly to **Pads `1`–`48`** on the desktop Tracker / Tracker+.
- Numbering does not need to be contiguous (for example, if you only place `1.jpg`, `5.jpg`, and `10.jpg`, the Left/Right buttons and Jog-wheel will cycle cleanly through those three images, while pressing Pad 5 jumps directly to `5.jpg`).

---

## 3. Image Format Requirements

The Tracker's FT812 graphics coprocessor decodes JPEGs directly in hardware (`CMD_LOADIMAGE`) and requires **Baseline JPEG** files:

| Property | Requirement |
| :--- | :--- |
| **Format** | **Baseline JPEG** (`.jpg` or `.jpeg`) - **Progressive JPEGs are not supported** by the FT812 hardware decoder. |
| **Chroma Subsampling** | `4:2:0` or `4:4:4` standard YCbCr (or grayscale). Avoid CMYK JPEGs. |
| **Maximum Dimensions** | **`800 × 390` pixels** (smaller images are supported and rendered from the top-left of the `800 × 390` viewport). |
| **Maximum File Size** | Up to **`650 KB`** per image (`800 × 390` JPEGs are typically `40 KB`–`150 KB`). |

---

## 4. Converting Images for the Tracker

If an image fails to load, it is almost always either **larger than `800 × 390` px** or saved as a **Progressive JPEG**. You can convert any PNG/JPEG to a Tracker-compatible `800 × 390` Baseline JPEG using any of the following tools:

### Option A: Using ImageMagick (`magick` / `convert`)
```bash
# Resize to fit within 800x390 (letterboxed on black) and save as Baseline JPEG:
magick input.png -resize 800x390 -background black -gravity center -extent 800x390 -interlace none -sampling-factor 4:2:0 -quality 90 1.jpg
```

### Option B: Using `ffmpeg`
```bash
# Scale and pad to 800x390 Baseline JPEG:
ffmpeg -i input.png -vf "scale=800:390:force_original_aspect_ratio=decrease,pad=800:390:(ow-iw)/2:(oh-ih)/2:black" -q:v 2 -pix_fmt yuvj420p 1.jpg
```

### Option C: In GIMP / Photoshop
1. Resize or crop the canvas to **`800 × 390` px** (or smaller).
2. Export as **`.jpg`** and **uncheck "Progressive"** in the JPEG export options.

---

## 5. Hardware Controls

| Action | Original Tracker & Tracker+ | Tracker Mini |
| :--- | :--- | :--- |
| **Next Image** | **Right Arrow**, **Screen Button 8 (`>>`)**, or turn **Jog-wheel clockwise** | **Right D-Pad** or **Screen Button 8 (`>>`)** |
| **Previous Image** | **Left Arrow**, **Screen Button 1 (`<<`)**, or turn **Jog-wheel counter-clockwise** | **Left D-Pad** or **Screen Button 1 (`<<`)** |
| **Jump Directly to Image `1`-`48`** | Press **Pad `1`-`48`** | - |
| **Exit Back to Config** | **Screen Button 7 (`Close`)** (or **Button 8 (`Close`)** when folder is empty) | **Screen Button 7 (`Close`)** (or **Middle Button** / **Button 8** when folder is empty) |
