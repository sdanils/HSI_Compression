#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "decompress.h"
#include "kekm.h"
#include "standart_data.h"

// Применяет обратное преобразование к одному пикселю.
// Универсальная формула: ŷ[b] = round(k_m * ref[b] + delta_y)
// NT: k_m=1, delta_y=0  — копия эталона
// OT: k_m=1             — эталон + смещение по среднему
// ST: delta_y=0         — масштабированный эталон
// AT: оба параметра     — полное аффинное восстановление
static void apply_inverse(const int16_t* ref, const kekm_result* match,
                          int bands, int16_t* out) {
  for (int b = 0; b < bands; b++) {
    double val = match->k_m * ref[b] + match->delta_y;
    if (val > INT16_MAX) val = INT16_MAX;
    if (val < INT16_MIN) val = INT16_MIN;
    out[b] = (int16_t)round(val);
  }
}

// Восстановление из comp_data в памяти.
// image[idx] соответствует пикселю с плоским индексом idx в матрице.
int16_t** decompress(const compressed_image* comp_data,
                     const hsi_header* header) {
  int total_pixels = header->lines * header->samples;

  int16_t** pixel_matrix = (int16_t**)malloc(total_pixels * sizeof(int16_t*));
  for (int i = 0; i < total_pixels; i++)
    pixel_matrix[i] = (int16_t*)calloc(header->bands, sizeof(int16_t));

  // Строим плоский массив указателей на под-эталоны из 3D comp_data
  int total_refs = 0;
  for (int i = 0; i < comp_data->num_ref; i++)
    total_refs += comp_data->ref_counts[i];

  const int16_t** flat = (const int16_t**)malloc(total_refs * sizeof(int16_t*));
  int flat_idx = 0;
  for (int i = 0; i < comp_data->num_ref; i++)
    for (int j = 0; j < comp_data->ref_counts[i]; j++)
      flat[flat_idx++] = comp_data->hsi_standarts[i][j];

  for (int idx = 0; idx < comp_data->size; idx++) {
    const standart_data* sd = comp_data->image[idx];
    if (sd->ref_index < 0 || sd->ref_index >= total_refs) continue;
    apply_inverse(flat[sd->ref_index], &sd->match, header->bands,
                  pixel_matrix[idx]);
  }

  free(flat);
  return pixel_matrix;
}

// Восстановление из двух бинарных GSD-файлов.
// standarts.gsd: [int32 total][int32 bands][int16 × total × bands] (BIP).
// image.gsd:     [int32 samples][int32 lines][per pixel: int32 ref, double×3].
int16_t** decompress_from_gsd_files(const char* standarts_gsd,
                                    const char* image_gsd,
                                    hsi_header* out_header) {
  // --- Эталоны ---
  FILE* sf = fopen(standarts_gsd, "rb");
  if (!sf) throw std::runtime_error("Cannot open standarts GSD for reading");

  int32_t total = 0, bands = 0;
  fread(&total, sizeof(int32_t), 1, sf);
  fread(&bands, sizeof(int32_t), 1, sf);
  if (total <= 0 || bands <= 0) {
    fclose(sf);
    throw std::runtime_error("Invalid standarts GSD header");
  }

  int16_t** refs = (int16_t**)malloc(total * sizeof(int16_t*));
  for (int i = 0; i < total; i++) {
    refs[i] = (int16_t*)malloc(bands * sizeof(int16_t));
    fread(refs[i], sizeof(int16_t), bands, sf);
  }
  fclose(sf);

  // --- Сжатое изображение ---
  FILE* imf = fopen(image_gsd, "rb");
  if (!imf) {
    for (int i = 0; i < total; i++) free(refs[i]);
    free(refs);
    throw std::runtime_error("Cannot open image GSD for reading");
  }

  int32_t samples = 0, lines = 0;
  fread(&samples, sizeof(int32_t), 1, imf);
  fread(&lines, sizeof(int32_t), 1, imf);
  int total_pixels = lines * samples;

  int16_t** pixel_matrix = (int16_t**)malloc(total_pixels * sizeof(int16_t*));
  for (int idx = 0; idx < total_pixels; idx++) {
    pixel_matrix[idx] = (int16_t*)calloc(bands, sizeof(int16_t));

    int32_t ref = 0;
    kekm_result match = {0.0, 0.0, 1.0};
    fread(&ref, sizeof(int32_t), 1, imf);
    fread(&match.epsilon, sizeof(double), 1, imf);
    fread(&match.delta_y, sizeof(double), 1, imf);
    fread(&match.k_m, sizeof(double), 1, imf);

    if (ref >= 0 && ref < total)
      apply_inverse(refs[ref], &match, bands, pixel_matrix[idx]);
  }
  fclose(imf);

  for (int i = 0; i < total; i++) free(refs[i]);
  free(refs);

  if (out_header) {
    out_header->samples = samples;
    out_header->lines = lines;
    out_header->bands = bands;
  }
  return pixel_matrix;
}
