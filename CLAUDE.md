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

The CLI is **subcommand-based**: the first argument selects the mode
(`compress` or `decompress`). There is no longer a `-D` flag.

```bash
# Compress: both .hdr and .gsd paths are mandatory
./hsi_compressor compress path/to/hsi.hdr path/to/hsi.gsd

# Compress with method AT and custom thresholds
./hsi_compressor compress hsi.hdr hsi.gsd -m 4 --me 3.0 --ae 1.0

# Decompress: rebuild .dat from GSD files (hdr optional, for byte_order)
./hsi_compressor decompress hsi.hdr --std standarts.gsd --img image.gsd --dat-out out.dat

# Usage
./hsi_compressor compress   <hdr_path> <dat_path> [OPTIONS]
./hsi_compressor decompress [hdr_path]            [OPTIONS]

Options:
  -m, --method   <1-4>  KEKM method: 1=NT 2=OT 3=ST 4=AT (default: 1)
  -s, --samples  <N>    override number of columns from header
  -l, --lines    <N>    override number of rows from header
  -b, --byte-order <N>  override byte order (0=LE, 1=BE)
  -e, --me       <%>    level-1 threshold, % of pixel norm (default: 5.0)
  -a, --ae       <%>    level-2 threshold, % of pixel norm (default: 2.0)
  -S, --shift    <0|1>  shifted placement of new standards (default: 1)
  -c, --sc       <x>    lattice step coefficient: d = sc·δ, 1 ≤ sc ≤ 2
                        (default: 1.0)
  -r, --rgb      <0|1>  generate RGB preview PNGs (default: 1)
  -w, --write    <0|1>  write GSD output files (default: 1)
  -d, --dat-out  <file> output .dat (decompress mode only, default: restored.dat)
  -p, --preview  <file> filename for original preview (default: preview.png)
  -P, --restored <file> filename for restored preview (default: restored_preview.png)
  -o, --std      <file> output file for standards (default: standarts.gsd)
  -i, --img      <file> output file for compressed image (default: image.gsd)
```

In the **`compress`** mode both the `.hdr` and `.gsd` positional paths are
required (no defaults). In the **`decompress`** mode the program reads
`standarts.gsd` + `image.gsd`, rebuilds the pixel matrix via
`decompress_from_gsd_files()`, and writes it to a `.dat` (`--dat-out`, default
`restored.dat`) in the same int16/BIP layout as the input `.gsd`. The `.hdr` is
still read for `byte_order` (not stored in GSD); its path is optional and
defaults to `/.data/hsi.hdr`.

### Input files

- `.hdr` — ENVI-format text header with image metadata (samples, lines, bands, data type, interleave, byte order, wavelengths)
- `.gsd` — binary pixel data (int16, BIP interleave)

### Output files

| File | Format | Description |
|------|--------|-------------|
| `preview.png` | PNG | RGB preview of original image (Gaussian-weighted blend at 660/550/470 nm, histogram-equalized); skipped if `--rgb 0` |
| `standarts.gsd` | Binary | All sub-standards (references); path overridable with `--std` |
| `image.gsd` | Binary | Per-pixel compressed data (ref index + match params); path overridable with `--img` |
| `restored_preview.png` | PNG | RGB preview of decompressed image (same LUT as preview.png for visual comparison); skipped if `--rgb 0` |

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
  │    └─ make_shifted_etalon() → shifted copy of the pixel for new standards
  └─ add_standart_data()      → appends result to image[] (dynamic array, doubles capacity)
save_standarts_gsd()          → standarts.gsd (binary)
save_compressed_image_gsd()   → image.gsd (binary)
decompress()                  → int16_t** restored pixel_matrix (from in-memory data)
save_rgb_image()              → restored_preview.png
free_hsi_data() / free_compressed_image() → cleanup
```

`main()` is a thin entry point: it parses argv into a `cli_options` struct and
dispatches to `run_compress()` / `run_decompress()` in `app.cpp` (the two
subcommands). All mode logic lives in `app.cpp`, not `main.cpp`.

Two decompression paths:
1. **From memory** (`decompress()`) — used in `run_compress()` to render `restored_preview.png`
2. **From GSD binary files** (`decompress_from_gsd_files()`) — entered with the `decompress` subcommand; rebuilds the pixel matrix from `standarts.gsd` + `image.gsd` and writes it to a `.dat` (`--dat-out`)

### Two-level matching (`check_pixel.cpp`)

Thresholds are **adaptive percentages**: before comparing, `check_pixel` computes
`norm = pixel_norm(pixel, bands, method)` and scales each threshold:
`threshold = (pct / 100.0) * norm`.

`pixel_norm` returns `M²(y)` for NT/ST and `D(y)` for OT/AT — the same quantity
that appears in the denominator of the normalised epsilon for each method.

1. **Level 1:** Compare pixel against base vector `hsi_standarts[i][0]` of each main standard using `pixel_distance()`. If best `epsilon > main_error_pct/100 * norm` → `add_standart()` (creates a new main standard).
2. **Level 2:** Within the best main standard, compare against all its sub-vectors. If best `epsilon > additional_error_pct/100 * norm` → `add_internal_standart()` (adds sub-standard). Otherwise assign and store the `kekm_result`.

### Shifted standard placement (`etalon_shift.cpp`)

New standards (both levels) store a **shifted copy** of the pixel rather than the
pixel itself. `make_shifted_etalon` makes one pass over the candidate standards:
for each neighbour whose `epsilon` (in the method's ε metric) is below the target
`threshold·sc`, the running pixel snapshot is pushed **away** from the
method-aligned neighbour `êᵢ = k_m·eᵢ + Δy` along the linear-scale direction
`ρ = √(n·ε)` by `(target_ρ − ρ)`. The shift accumulator is kept in `double` (full
precision between iterations); a rounded int16 snapshot is used for each
`pixel_distance` comparison. `sc` is the single lattice step coefficient
(`settings.step_coef`, CLI `--sc`, passed explicitly to `make_shifted_etalon`).
After creation, `check_pixel` stores the actual `pixel_distance(e_new, p)` in
`result->match` (falling back to identity `{0, 0, 1}` if degenerate/NaN). Disable
with `--shift 0` (`settings.shift_enabled`).

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

- Computes R/G/B channels as **Gaussian-weighted blend** (σ=30 nm) of all bands centred at 660/550/470 nm (not nearest-band selection)
- Applies **CDF-based histogram equalization** per channel (`float` values → `uint8_t` via 65536-bin LUT)
- Optional `ref_matrix` parameter: when generating `restored_preview.png`, the LUT is calibrated on the original image so the two previews share the same tone mapping and are visually comparable
- Writes PNG via `stb_image_write.h`

### Key data structures

| Type | Fields |
|------|--------|
| `hsi_header` | `samples`, `lines`, `bands`, `data_type`, `interleave[4]`, `byte_order`, `header_offset`, `wavelengths[]` |
| `compression_settings` | `main_error_pct` (%), `additional_error_pct` (%), `method` (`kekm_method` enum), `shift_enabled` (0/1), `step_coef` (lattice sc) |
| `compressed_image` | `hsi_standarts[main][sub][band]` (3D), `num_ref`, `ref_counts[main]`, `image[]` (`standart_data**`), `size` |
| `standart_data` | `ref_index` (flat index), `match` (`kekm_result`) |
| `kekm_result` | `epsilon`, `delta_y`, `k_m` (all `double`) |

Sub-standards are addressed by a **flat index**: `ref_index = Σ ref_counts[0..i-1] + j`.

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

### Header organisation

`functions.h` is an umbrella that includes all module headers. Use the specific header in `.cpp` files:

| Header | Declares |
|--------|---------|
| `app.h` | `cli_options` struct, `run_compress`, `run_decompress` (subcommand entry points) |
| `check_pixel.h` | `add_standart`, `add_internal_standart`, `check_pixel` |
| `compression.h` | `add_standart_data`, `compression` |
| `decompress.h` | `decompress`, `decompress_from_gsd_files` |
| `io.h` | `load_hsi_data`, `read_hdr_file` |
| `save.h` | `save_standarts_gsd`, `save_compressed_image_gsd`, `save_hsi_data`, `save_rgb_image` |
| `memory.h` | `free_hsi_data`, `free_compressed_image` |
| `kekm.h` | `kekm_result`, `kekm_method`, `kekm_nt/ot/st/at`, `pixel_distance`, `pixel_norm` |
| `etalon_shift.h` | `make_shifted_etalon` (shifted copy of a pixel for a new standard) |
| `hsi_header.h` | `hsi_header` struct |
| `compressed_image.h` | `compressed_image` struct |
| `compression_settings.h` | `compression_settings` struct |
| `standart_data.h` | `standart_data` struct |

## Compression settings

Thresholds are percentages of the pixel's own energy/variance (adaptive):

```cpp
compression_settings settings = {5.0, 2.0, method, 1, 1.0};
//                                ^    ^            ^  ^
//                    main_error_pct   |  shift_enabled|
//                       additional_error_pct      step_coef (sc)
//                              (both in %)
```

CLI: `--me 5.0 --ae 2.0 --shift 1 --sc 1.0` (lattice step coefficient sc,
1 ≤ sc ≤ 2; at sc = 1 the shift mechanism is geometrically inactive)

Higher % → fewer standards → smaller output, lower quality.
`main_error_pct` controls how many distinct material classes are created;
`additional_error_pct` controls fine variation within each class.

Adaptive threshold per pixel: `threshold = (pct / 100.0) * pixel_norm(pixel, bands, method)`
where `pixel_norm` = `M²(y)` for NT/ST, `D(y)` for OT/AT.

## Build notes

- Compiler flags: `-Wall -Wextra -Werror -pedantic` (GCC/Clang); `/W4 /WX` (MSVC)
- `stb_image_write.h` needs `#pragma GCC diagnostic` around its include to suppress `-Wmissing-field-initializers` (already done in `save_rgb.cpp`)
