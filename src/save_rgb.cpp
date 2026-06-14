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

// Gaussian weights centered at target wavelength; normalised to sum = 1.
static void compute_gaussian_weights(const float* wavelengths, int bands,
                                     float target, float sigma, float* weights) {
  float sum = 0.0f;
  for (int i = 0; i < bands; i++) {
    float d = (wavelengths[i] - target) / sigma;
    weights[i] = expf(-0.5f * d * d);
    sum += weights[i];
  }
  if (sum > 0.0f)
    for (int i = 0; i < bands; i++)
      weights[i] /= sum;
}

// Weighted-average spectral value for every pixel in pixel_matrix.
static void compute_channel(int16_t** pixel_matrix, int total, int bands,
                             const float* weights, float* out) {
  for (int i = 0; i < total; i++) {
    float acc = 0.0f;
    for (int b = 0; b < bands; b++)
      acc += weights[b] * (float)pixel_matrix[i][b];
    out[i] = acc;
  }
}

// Find lo/hi percentile values via histogram (65536 bins over [vmin, vmax]).
static void percentile_range(const float* values, int total,
                              float vmin, float vmax,
                              float lo_pct, float hi_pct,
                              float* lo_val, float* hi_val) {
  const int BINS = 65536;
  int* hist = (int*)calloc(BINS, sizeof(int));
  float inv = (vmax > vmin + 1e-6f) ? (float)(BINS - 1) / (vmax - vmin) : 1.0f;

  for (int i = 0; i < total; i++) {
    int b = (int)((values[i] - vmin) * inv + 0.5f);
    if (b < 0) b = 0;
    if (b >= BINS) b = BINS - 1;
    hist[b]++;
  }

  long long lo_count = (long long)(lo_pct / 100.0f * total + 0.5f);
  long long hi_count = (long long)(hi_pct / 100.0f * total + 0.5f);
  long long acc = 0;
  *lo_val = vmin;
  *hi_val = vmax;
  for (int i = 0; i < BINS; i++) {
    acc += hist[i];
    if (acc >= lo_count && *lo_val == vmin)
      *lo_val = vmin + (float)i / (float)(BINS - 1) * (vmax - vmin);
    if (acc >= hi_count) {
      *hi_val = vmin + (float)i / (float)(BINS - 1) * (vmax - vmin);
      break;
    }
  }

  free(hist);
}

// Gaussian sigma in nm — controls how many neighbouring bands blend into each channel.
// Larger sigma → smoother, more bands contribute; smaller → closer to single-band.
static const float RGB_SIGMA = 30.0f;
static const float RGB_TARGETS[3] = {660.0f, 550.0f, 470.0f};
// Saturation multiplier: 1.0 = no change, >1 boosts colour distance from luminance.
static const float RGB_SAT_FACTOR = 1.8f;

static uint8_t clamp_u8(float v) {
  if (v < 0.0f)   return 0;
  if (v > 255.0f) return 255;
  return (uint8_t)(v + 0.5f);
}

static uint8_t* hsi_to_rgb(int16_t** pixel_matrix, const hsi_header* header,
                            int16_t** ref_matrix) {
  int total = header->lines * header->samples;

  float* weights[3];
  for (int c = 0; c < 3; c++) {
    weights[c] = (float*)malloc((size_t)header->bands * sizeof(float));
    compute_gaussian_weights(header->wavelengths, header->bands,
                             RGB_TARGETS[c], RGB_SIGMA, weights[c]);
  }

  float* ch[3];
  float* ref_ch[3];
  for (int c = 0; c < 3; c++) {
    ch[c] = (float*)malloc((size_t)total * sizeof(float));
    compute_channel(pixel_matrix, total, header->bands, weights[c], ch[c]);

    if (ref_matrix) {
      ref_ch[c] = (float*)malloc((size_t)total * sizeof(float));
      compute_channel(ref_matrix, total, header->bands, weights[c], ref_ch[c]);
    } else {
      ref_ch[c] = ch[c];
    }
  }

  uint8_t* image = (uint8_t*)malloc((size_t)total * 3);

  for (int c = 0; c < 3; c++) {
    float vmin = ref_ch[c][0], vmax = ref_ch[c][0];
    for (int i = 1; i < total; i++) {
      if (ref_ch[c][i] < vmin) vmin = ref_ch[c][i];
      if (ref_ch[c][i] > vmax) vmax = ref_ch[c][i];
    }

    float lo, hi;
    percentile_range(ref_ch[c], total, vmin, vmax, 2.0f, 98.0f, &lo, &hi);

    float inv_range = (hi > lo + 1e-6f) ? 255.0f / (hi - lo) : 1.0f;
    for (int i = 0; i < total; i++) {
      float v = (ch[c][i] - lo) * inv_range;
      if (v < 0.0f) v = 0.0f;
      if (v > 255.0f) v = 255.0f;
      image[i * 3 + c] = (uint8_t)(v + 0.5f);
    }
  }

  for (int c = 0; c < 3; c++) {
    free(weights[c]);
    free(ch[c]);
    if (ref_matrix) free(ref_ch[c]);
  }

  // Saturation boost: scale each channel away from luminance (Rec.601 weights).
  for (int i = 0; i < total; i++) {
    float r = image[i * 3 + 0];
    float g = image[i * 3 + 1];
    float b = image[i * 3 + 2];
    float lum = 0.299f * r + 0.587f * g + 0.114f * b;
    image[i * 3 + 0] = clamp_u8(lum + RGB_SAT_FACTOR * (r - lum));
    image[i * 3 + 1] = clamp_u8(lum + RGB_SAT_FACTOR * (g - lum));
    image[i * 3 + 2] = clamp_u8(lum + RGB_SAT_FACTOR * (b - lum));
  }

  return image;
}

void save_rgb_image(int16_t** pixel_matrix, const hsi_header* header,
                    const char* filename, int16_t** ref_matrix) {
  uint8_t* image = hsi_to_rgb(pixel_matrix, header, ref_matrix);
  stbi_write_png(filename, header->samples, header->lines, 3, image,
                 header->samples * 3);
  free(image);
}
