# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
# Build
mkdir build && cd build
cmake ..
cmake --build .

# Run with default paths (/.data/hsi.hdr and /.data/hsi.gsd)
./hsi_compressor

# Run with explicit file paths
./hsi_compressor path/to/hsi.hdr path/to/hsi.gsd

# Run with explicit paths and override header metadata
./hsi_compressor path/to/hsi.hdr path/to/hsi.gsd <samples> <lines> <byte_order>
```

Input files:
- `.hdr` — ENVI-format text header with image metadata
- `.gsd` — binary pixel data (int16, BIP interleave format)

## Current state of main.cpp

The compression pipeline is currently **commented out**. `main.cpp` only loads the data and calls `save_rgb_image()`. To re-enable compression, uncomment the relevant lines and the `free_compressed_image()` call.

Active output: `rgb_preview.png`

Inactive (commented) outputs: `standarts.txt`, `image.txt`

## Architecture

The algorithm compresses HSI by replacing each pixel with a reference to the nearest "standard" (эталон) instead of storing raw spectral data.

### Data flow

```
read_hdr_file()         → parse .hdr metadata into hsi_header
load_hsi_data()         → load binary pixel matrix [pixel_idx][band]
compression()           → iterate pixels → check_pixel() for each
save_standarts()        → write reference standards to file
save_compressed_image() → write pixel-to-standard mappings to file
save_rgb_image()        → render RGB preview PNG (R=660nm, G=550nm, B=470nm)
free_*()                → cleanup
```

### Two-criterion matching (check_pixel.cpp)

The core algorithm operates in two hierarchical levels:

1. **Level 1 (main references):** Compute MSE against each main standard's base vector `hsi_standarts[i][0]`. If best MSE > `main_error` → create a new main standard via `add_standart()`.
2. **Level 2 (internal references):** Within the best main standard, compute MSE against all its internal sub-vectors. If best MSE > `additional_error` → create a new internal standard via `add_internal_standart()`. Otherwise assign the pixel to the matching sub-reference.

A pixel is skipped in `compression()` if `pixel[0] == -1 || pixel[0] == 0`.

### Key data structures

| Type | Purpose |
|------|---------|
| `hsi_header` | `samples`, `lines`, `bands`, `byte_order`, `interleave`, `wavelengths[]` |
| `compression_settings` | `main_error` / `additional_error` thresholds |
| `compressed_image` | `hsi_standarts[main][sub][band]` (3D), `ref_counts[main]`, `image[]` |
| `standart_data` | Per-pixel result: `main` index, `additional` index, `mse` |

### Memory layout

- Pixel matrix: `int16_t* pixel_matrix[lines * samples]` — each pointer is one pixel vector of length `bands`
- Standards: `hsi_standarts` grows via `realloc` at both the main and sub-reference levels
- `image[]` in `compressed_image` grows via `add_match_result()` which doubles capacity on overflow

### RGB preview (save_rgb.cpp)

Selects bands nearest to 660/550/470 nm using `header->wavelengths`. Applies a 2%–98% percentile stretch per channel, then gamma-encodes (γ = 2.2) before writing PNG via `stb_image_write`.

## Compression settings

Set in `main.cpp`:

```cpp
compression_settings settings = {26000, 21000};
//                                ^      ^
//                          main_error  additional_error
```

Higher thresholds → fewer standards → smaller output, lower quality. Lower → more standards → larger output, higher quality.
