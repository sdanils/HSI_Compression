#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "save.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#pragma GCC diagnostic pop

static int find_closest_band(const float* wavelengths, int bands, float target) {
  int best = 0;
  float best_diff = fabsf(wavelengths[0] - target);
  for (int i = 1; i < bands; i++) {
    float diff = fabsf(wavelengths[i] - target);
    if (diff < best_diff) {
      best_diff = diff;
      best = i;
    }
  }
  return best;
}

// Строит LUT гистограммного выравнивания для одного канала.
// Значение v ∈ [-32768, 32767] → out_lut[v + 32768] ∈ [0, 255].
static void build_equalization_lut(const int16_t* values, int total,
                                   uint8_t* out_lut) {
  int* hist = (int*)calloc(65536, sizeof(int));
  for (int i = 0; i < total; i++)
    hist[(int)values[i] + 32768]++;

  // CDF
  long long cdf[65536];
  cdf[0] = hist[0];
  for (int i = 1; i < 65536; i++)
    cdf[i] = cdf[i - 1] + hist[i];

  // Минимальный ненулевой CDF (исключаем нули из нормировки)
  long long cdf_min = 0;
  for (int i = 0; i < 65536; i++) {
    if (hist[i] > 0) { cdf_min = cdf[i]; break; }
  }

  long long denom = (long long)total - cdf_min;
  for (int i = 0; i < 65536; i++) {
    if (denom <= 0 || cdf[i] <= cdf_min) {
      out_lut[i] = 0;
    } else {
      double v = (double)(cdf[i] - cdf_min) / (double)denom * 255.0;
      out_lut[i] = (uint8_t)(v > 255.0 ? 255 : v);
    }
  }

  free(hist);
}

static uint8_t* hsi_to_rgb(int16_t** pixel_matrix, const hsi_header* header,
                            int r_band, int g_band, int b_band) {
  int total = header->lines * header->samples;
  int band_idx[3] = {r_band, g_band, b_band};

  // Собираем значения каждого канала
  int16_t* channel_vals[3];
  for (int c = 0; c < 3; c++) {
    channel_vals[c] = (int16_t*)malloc((size_t)total * sizeof(int16_t));
    for (int i = 0; i < total; i++)
      channel_vals[c][i] = pixel_matrix[i][band_idx[c]];
  }

  // LUT гистограммного выравнивания для каждого канала
  uint8_t* lut[3];
  for (int c = 0; c < 3; c++) {
    lut[c] = (uint8_t*)malloc(65536);
    build_equalization_lut(channel_vals[c], total, lut[c]);
  }

  uint8_t* image = (uint8_t*)malloc((size_t)total * 3);
  for (int i = 0; i < total; i++) {
    for (int c = 0; c < 3; c++)
      image[i * 3 + c] = lut[c][(int)channel_vals[c][i] + 32768];
  }

  for (int c = 0; c < 3; c++) {
    free(channel_vals[c]);
    free(lut[c]);
  }
  return image;
}

void save_rgb_image(int16_t** pixel_matrix, const hsi_header* header,
                    const char* filename) {
  int r_band = find_closest_band(header->wavelengths, header->bands, 660.0f);
  int g_band = find_closest_band(header->wavelengths, header->bands, 550.0f);
  int b_band = find_closest_band(header->wavelengths, header->bands, 470.0f);

  uint8_t* image = hsi_to_rgb(pixel_matrix, header, r_band, g_band, b_band);

  stbi_write_png(filename, header->samples, header->lines, 3, image,
                 header->samples * 3);
  free(image);
}
