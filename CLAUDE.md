# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
# Build
mkdir build && cd build
cmake ..
cmake --build .

# Run with default paths (/.data/hsi.hdr and /.data/hsi.gsd), method defaults to NT
./hsi_compressor

# Explicit paths
./hsi_compressor path/to/hsi.hdr path/to/hsi.gsd [method]

# Explicit paths + override header metadata
./hsi_compressor path/to/hsi.hdr path/to/hsi.gsd <samples> <lines> <byte_order> [method]
# method: 1=NT  2=OT  3=ST  4=AT
```

Input files:
- `.hdr` — ENVI-format text header with image metadata
- `.gsd` — binary pixel data (int16, BIP interleave)

Outputs: `standarts.txt`, `image.txt`, `restored_preview.png`

## Architecture

HSI compression replaces each pixel with a reference to the nearest sub-standard (эталон) rather than storing raw spectral data.

### Data flow

```
read_hdr_file()          → hsi_header (metadata)
load_hsi_data()          → int16_t** pixel_matrix [lines*samples][bands]
compression()            → iterates pixels → check_pixel() → compressed_image
save_standarts()         → standarts.txt
save_compressed_image()  → image.txt
decompress()             → int16_t** restored pixel_matrix
save_rgb_image()         → restored_preview.png
free_*()                 → cleanup
```

### Two-criterion matching (`check_pixel.cpp`)

1. **Level 1:** Compare pixel against base vector `hsi_standarts[i][0]` of each main standard using `pixel_distance()`. If best `epsilon > main_error` → `add_standart()`.
2. **Level 2:** Within the best main standard, compare against all its sub-vectors. If best `epsilon > additional_error` → `add_internal_standart()`. Otherwise assign and store the `kekm_result`.

Pixels with more than half negative band values are replaced by a zero vector before compression (not skipped — every pixel produces an entry in `image[]`).

### KEKM comparison methods (`kekm.h` / `kekm.cpp`)

Four methods, selected via `compression_settings.method`:

| Value | Name | Invariance | `epsilon` formula |
|-------|------|-----------|-------------------|
| 1 `KEKM_NT` | НП | none | `M²(e) + M²(y) − 2·M(ey)` |
| 2 `KEKM_OT` | ОП | orthogonal transforms | `D(e) + D(y) − 2·cov(e,y)` |
| 3 `KEKM_ST` | МП | scaling | `M²(y) − M²(ey)/M²(e)` |
| 4 `KEKM_AT` | АП | affine transforms | `D(y) − cov²(e,y)/D(e)` |

`kekm_result` stores `{epsilon, delta_y, k_m}`. During decompression the inverse transform `ŷ[b] = round(k_m * ref[b] + delta_y)` is applied uniformly for all methods (NT/OT have k_m=1; NT/ST have delta_y=0).

### Key data structures

| Type | Fields |
|------|--------|
| `hsi_header` | `samples`, `lines`, `bands`, `byte_order`, `wavelengths[]` |
| `compression_settings` | `main_error`, `additional_error`, `method` |
| `compressed_image` | `hsi_standarts[main][sub][band]` (3D), `ref_counts[main]`, `image[]`, `size` |
| `standart_data` | `ref_index` (flat), `match` (`kekm_result`) |

Sub-standards are addressed by a **flat index**: `ref_index = Σ ref_counts[0..i-1] + j`. Both `standarts.txt` and `image.txt` use this flat numbering.

### File formats

`standarts.txt`:
```
<total_sub_standards> <bands>
<b0> <b1> ... <bN>    ← flat index 0
...
```

`image.txt`:
```
<samples> <lines>
<ref_index> <epsilon> <delta_y> <k_m>   ← one line per pixel, row-major order
```

### Header organisation

`functions.h` is an umbrella that includes all module headers. Use the specific header in `.cpp` files:

| Header | Declares |
|--------|---------|
| `check_pixel.h` | `add_standart`, `add_internal_standart`, `check_pixel` |
| `compression.h` | `add_standart_data`, `compression` |
| `decompress.h` | `decompress`, `decompress_from_files` |
| `io.h` | `load_hsi_data`, `read_hdr_file` |
| `save.h` | `save_standarts`, `save_compressed_image`, `save_rgb_image` |
| `memory.h` | `free_hsi_data`, `free_compressed_image`, `print_pixel` |

## Compression settings

```cpp
compression_settings settings = {5000, 2000, method};
//                                ^     ^
//                          main_error  additional_error
```

Higher thresholds → fewer standards → smaller output, lower quality.
`main_error` controls how many distinct material classes are created; `additional_error` controls fine variation within each class.
