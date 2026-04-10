#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "save.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

static int cmp16(const void* a, const void* b) {
  return (int)(*(const int16_t*)a) - (int)(*(const int16_t*)b);
}

static void compute_stretch(const int16_t* values, int count,
                             double* out_min, double* out_max) {
  int16_t* sorted = (int16_t*)malloc((size_t)count * sizeof(int16_t));
  memcpy(sorted, values, (size_t)count * sizeof(int16_t));
  qsort(sorted, (size_t)count, sizeof(int16_t), cmp16);
  *out_min = sorted[(int)(count * 0.02)];
  *out_max = sorted[(int)(count * 0.98)];
  free(sorted);
}

// Возвращает плоский буфер uint8_t: 3 байта на пиксель (R, G, B)
static uint8_t* hsi_to_rgb(int16_t** pixel_matrix, const hsi_header* header,
                            int r_band, int g_band, int b_band) {
  int total = header->lines * header->samples;
  int band_idx[3] = {r_band, g_band, b_band};

  int16_t* channel_vals[3];
  for (int c = 0; c < 3; c++) {
    channel_vals[c] = (int16_t*)malloc((size_t)total * sizeof(int16_t));
    for (int i = 0; i < total; i++) {
      channel_vals[c][i] = pixel_matrix[i][band_idx[c]];
    }
  }

  double mins[3], maxs[3];
  for (int c = 0; c < 3; c++) {
    compute_stretch(channel_vals[c], total, &mins[c], &maxs[c]);
    if (maxs[c] <= mins[c]) maxs[c] = mins[c] + 1;
  }

  uint8_t* image = (uint8_t*)malloc((size_t)total * 3);
  for (int i = 0; i < total; i++) {
    if (pixel_matrix[i][0] == -1 || pixel_matrix[i][0] == 0) {
      image[i * 3 + 0] = 0;
      image[i * 3 + 1] = 0;
      image[i * 3 + 2] = 0;
      continue;
    }
    for (int c = 0; c < 3; c++) {
      double val = (channel_vals[c][i] - mins[c]) / (maxs[c] - mins[c]);
      if (val < 0.0) val = 0.0;
      if (val > 1.0) val = 1.0;
      val = pow(val, 1.0 / 2.2);
      image[i * 3 + c] = (uint8_t)(val * 255.0);
    }
  }

  for (int c = 0; c < 3; c++) free(channel_vals[c]);
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
