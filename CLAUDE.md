# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

### Prerequisites

Only standard tools — no external dependencies:

```bash
# Linux (Ubuntu/Debian)
sudo apt update && sudo apt install build-essential cmake
```

The project uses C++17, CMake ≥ 3.10, and the header-only library `stb_image_write.h` (already in `include/`).

### Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Run

```bash
# Default paths (/.data/hsi.hdr and /.data/hsi.gsd), method = NT
./hsi_compressor

# Explicit paths
./hsi_compressor path/to/hsi.hdr path/to/hsi.gsd

# Full usage with options
./hsi_compressor [hdr_path] [dat_path] [OPTIONS]

Options:
  -m, --method   <1-4>  KEKM method: 1=NT 2=OT 3=ST 4=AT (default: 1)
  -s, --samples  <N>    override number of columns from header
  -l, --lines    <N>    override number of rows from header
  -b, --byte-order <N>  override byte order (0=LE, 1=BE)
  --me           <%>    level-1 threshold, % of pixel norm (default: 5.0)
  --ae           <%>    level-2 threshold, % of pixel norm (default: 2.0)
```

### Input files

- `.hdr` — ENVI-format text header with image metadata (samples, lines, bands, data type, interleave, byte order, wavelengths)
- `.gsd` — binary pixel data (int16, BIP interleave)

### Output files

| File | Format | Description |
|------|--------|-------------|
| `preview.png` | PNG | RGB preview of the original image (bands ≈ 660/550/470 nm, histogram-equalized) |
| `standarts.gsd` | Binary | All sub-standards (references) |
| `image.gsd` | Binary | Per-pixel compressed data (ref index + match params) |
| `restored_preview.png` | PNG | RGB preview of the decompressed image |

## Architecture

HSI compression replaces each pixel with a reference to the nearest sub-standard (эталон) rather than storing raw spectral data.

### Data flow

```
read_hdr_file()               → hsi_header (metadata from .hdr)
load_hsi_data()               → int16_t** pixel_matrix [lines*samples][bands]
save_rgb_image()              → preview.png (RGB of original)
compression()                 → iterates pixels → check_pixel() → compressed_image
  ├─ select_pixel()           → zero-replace pixels with >50% negative bands
  ├─ check_pixel()            → two-level matching (see below)
  └─ add_standart_data()      → appends result to image[] (dynamic array, doubles capacity)
save_standarts_gsd()          → standarts.gsd (binary)
save_compressed_image_gsd()   → image.gsd (binary)
decompress()                  → int16_t** restored pixel_matrix (from in-memory data)
save_rgb_image()              → restored_preview.png
free_hsi_data() / free_compressed_image() → cleanup
```

Three decompression paths:
1. **From memory** (`decompress()`) — used by default in `main()`
2. **From text files** (`decompress_from_files()`) — legacy
3. **From GSD binary files** (`decompress_from_gsd_files()`) — commented out in `main()`, for standalone decompression

### Two-level matching (`check_pixel.cpp`)

Thresholds are **adaptive percentages**: before comparing, `check_pixel` computes
`norm = pixel_norm(pixel, bands, method)` and scales each threshold:
`threshold = (pct / 100.0) * norm`.

`pixel_norm` returns `M²(y)` for NT/ST and `D(y)` for OT/AT — the same quantity
that appears in the denominator of the normalised epsilon for each method.

1. **Level 1:** Compare pixel against base vector `hsi_standarts[i][0]` of each main standard using `pixel_distance()`. If best `epsilon > main_error_pct/100 * norm` → `add_standart()` (creates a new main standard).
2. **Level 2:** Within the best main standard, compare against all its sub-vectors. If best `epsilon > additional_error_pct/100 * norm` → `add_internal_standart()` (adds sub-standard). Otherwise assign and store the `kekm_result`.

Pixels with more than half negative band values are replaced by a zero vector before compression (`select_pixel()` in `compression.cpp`). They are not skipped — every pixel produces an entry in `image[]`.

### KEKM comparison methods (`kekm.h` / `kekm.cpp`)

Four methods, selected via `compression_settings.method`:

| Value | Name | Invariance | `epsilon` formula | `k_m` | `delta_y` |
|-------|------|-----------|-------------------|-------|-----------|
| 1 `KEKM_NT` | НП | none | `M²(e) + M²(y) − 2·M(ey)` | 1.0 | 0.0 |
| 2 `KEKM_OT` | ОП | orthogonal transforms | `D(e) + D(y) − 2·cov(e,y)` | 1.0 | `mean(y) − mean(e)` |
| 3 `KEKM_ST` | МП | scaling | `M²(y) − (M(ey))²/M²(e)` | `M(ey)/M²(e)` | 0.0 |
| 4 `KEKM_AT` | АП | affine transforms | `D(y) − cov²(e,y)/D(e)` | `cov(e,y)/D(e)` | `mean(y) − k_m·mean(e)` |

Helper functions in `kekm.cpp`: `mean()`, `moment2()` (second moment), `disp()` (variance), `cov()` (covariance), `cross()` (cross-moment `M(ey)`).

`kekm_result` stores `{epsilon, delta_y, k_m}`. During decompression the inverse transform `ŷ[b] = round(k_m * ref[b] + delta_y)` is applied uniformly for all methods, with clamping to int16 range `[-32768, 32767]`.

### RGB preview generation (`save_rgb.cpp`)

- Selects bands closest to R=660nm, G=550nm, B=470nm
- Applies **histogram equalization** per channel (not percentile stretch or gamma — builds a full CDF-based LUT mapping `int16 → uint8`)
- Writes PNG via `stb_image_write.h`

### Key data structures

| Type | Fields |
|------|--------|
| `hsi_header` | `samples`, `lines`, `bands`, `data_type`, `interleave[4]`, `byte_order`, `header_offset`, `wavelengths[]` |
| `compression_settings` | `main_error_pct` (%), `additional_error_pct` (%), `method` (`kekm_method` enum) |
| `compressed_image` | `hsi_standarts[main][sub][band]` (3D), `num_ref`, `ref_counts[main]`, `image[]` (`standart_data**`), `size` |
| `standart_data` | `ref_index` (flat index), `match` (`kekm_result`) |
| `kekm_result` | `epsilon`, `delta_y`, `k_m` (all `double`) |

Sub-standards are addressed by a **flat index**: `ref_index = Σ ref_counts[0..i-1] + j`. Both binary and text formats use this flat numbering.

### File formats

**Binary GSD** (primary):

`standarts.gsd`:
```
[int32]  total_sub_standards
[int32]  bands
[int16 × total × bands]  — BIP, flat order (same layout as input .gsd)
```

`image.gsd`:
```
[int32]  samples
[int32]  lines
per pixel (row-major):
  [int32]  ref_index
  [double] epsilon
  [double] delta_y
  [double] k_m
```

**Text** (legacy): `standarts.txt` / `image.txt` — same data in human-readable form. Read/written by `save_standarts()` / `save_compressed_image()` and `decompress_from_files()`.

### Header organisation

`functions.h` is an umbrella that includes all module headers. Use the specific header in `.cpp` files:

| Header | Declares |
|--------|---------|
| `check_pixel.h` | `add_standart`, `add_internal_standart`, `check_pixel` |
| `compression.h` | `add_standart_data`, `compression` |
| `decompress.h` | `decompress`, `decompress_from_files`, `decompress_from_gsd_files` |
| `io.h` | `load_hsi_data`, `read_hdr_file` |
| `save.h` | `save_standarts`, `save_compressed_image`, `save_standarts_gsd`, `save_compressed_image_gsd`, `save_rgb_image` |
| `memory.h` | `free_hsi_data`, `free_compressed_image`, `print_pixel` |
| `kekm.h` | `kekm_result`, `kekm_method`, `kekm_nt/ot/st/at`, `pixel_distance`, `pixel_norm` |
| `hsi_header.h` | `hsi_header` struct |
| `compressed_image.h` | `compressed_image` struct |
| `compression_settings.h` | `compression_settings` struct |
| `standart_data.h` | `standart_data` struct |

## Compression settings

Thresholds are percentages of the pixel's own energy/variance (adaptive):

```cpp
compression_settings settings = {5.0, 2.0, method};
//                                ^    ^
//                    main_error_pct   additional_error_pct  (both in %)
```

CLI: `--me 5.0 --ae 2.0`

Higher % → fewer standards → smaller output, lower quality.
`main_error_pct` controls how many distinct material classes are created;
`additional_error_pct` controls fine variation within each class.

Adaptive threshold per pixel: `threshold = (pct / 100.0) * pixel_norm(pixel, bands, method)`
where `pixel_norm` = `M²(y)` for NT/ST, `D(y)` for OT/AT.

## Build notes

- Compiler flags: `-Wall -Wextra -Werror -pedantic` (GCC/Clang); `/W4 /WX` (MSVC)
- `stb_image_write.h` needs `#pragma GCC diagnostic` around its include to suppress `-Wmissing-field-initializers` (already done in `save_rgb.cpp`)
