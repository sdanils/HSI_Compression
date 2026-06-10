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

// Histogram-equalisation LUT for float values quantised into 65536 bins.
// out_lut[bin] → uint8, bin = round((val - vmin) / (vmax - vmin) * 65535).
static void build_equalization_lut_float(const float* values, int total,
                                         float vmin, float vmax,
                                         uint8_t* out_lut) {
  int* hist = (int*)calloc(65536, sizeof(int));
  float inv_range = (vmax > vmin + 1e-6f) ? 65535.0f / (vmax - vmin) : 1.0f;

  for (int i = 0; i < total; i++) {
    int bin = (int)((values[i] - vmin) * inv_range + 0.5f);
    if (bin < 0) bin = 0;
    if (bin > 65535) bin = 65535;
    hist[bin]++;
  }

  long long* cdf = (long long*)malloc(65536 * sizeof(long long));
  cdf[0] = hist[0];
  for (int i = 1; i < 65536; i++)
    cdf[i] = cdf[i - 1] + hist[i];

  long long cdf_min = 0;
  for (int i = 0; i < 65536; i++) {
    if (hist[i] > 0) { cdf_min = cdf[i]; break; }
  }

  long long denom = (long long)total - cdf_min;
  for (int i = 0; i < 65536; i++) {
    if (denom <= 0 || cdf[i] <= cdf_min)
      out_lut[i] = 0;
    else {
      double v = (double)(cdf[i] - cdf_min) / (double)denom * 255.0;
      out_lut[i] = (uint8_t)(v > 255.0 ? 255 : v);
    }
  }

  free(hist);
  free(cdf);
}

// Gaussian sigma in nm — controls how many neighbouring bands blend into each channel.
// Larger sigma → smoother, more bands contribute; smaller → closer to single-band.
static const float RGB_SIGMA = 30.0f;
static const float RGB_TARGETS[3] = {660.0f, 550.0f, 470.0f};

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

    uint8_t* lut = (uint8_t*)malloc(65536);
    build_equalization_lut_float(ref_ch[c], total, vmin, vmax, lut);

    float inv_range = (vmax > vmin + 1e-6f) ? 65535.0f / (vmax - vmin) : 1.0f;
    for (int i = 0; i < total; i++) {
      int bin = (int)((ch[c][i] - vmin) * inv_range + 0.5f);
      if (bin < 0) bin = 0;
      if (bin > 65535) bin = 65535;
      image[i * 3 + c] = lut[bin];
    }

    free(lut);
  }

  for (int c = 0; c < 3; c++) {
    free(weights[c]);
    free(ch[c]);
    if (ref_matrix) free(ref_ch[c]);
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
