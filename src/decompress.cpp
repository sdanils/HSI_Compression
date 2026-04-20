#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
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
    if (val > 32767.0)  val = 32767.0;
    if (val < -32768.0) val = -32768.0;
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

// Восстановление из файлов standarts_file и image_file.
// Формат standarts_file:
//   <total> <bands>
//   <b0> <b1> ... <bN>   (строка на каждый под-эталон, плоская нумерация)
// Формат image_file:
//   <samples> <lines>
//   <ref_index> <epsilon> <delta_y> <k_m>   (строка на каждый пиксель)
int16_t** decompress_from_files(const char* standarts_file,
                                const char* image_file) {
  // --- Загрузка эталонов ---
  std::ifstream fs(standarts_file);
  if (!fs.is_open())
    throw std::runtime_error("Cannot open standarts file");

  int total_refs, bands;
  fs >> total_refs >> bands;

  int16_t** flat = (int16_t**)malloc(total_refs * sizeof(int16_t*));
  for (int i = 0; i < total_refs; i++) {
    flat[i] = (int16_t*)malloc(bands * sizeof(int16_t));
    for (int b = 0; b < bands; b++) {
      int val;
      fs >> val;
      flat[i][b] = (int16_t)val;
    }
  }
  fs.close();

  // --- Загрузка сжатых пикселей ---
  std::ifstream fi(image_file);
  if (!fi.is_open()) {
    for (int i = 0; i < total_refs; i++) free(flat[i]);
    free(flat);
    throw std::runtime_error("Cannot open image file");
  }

  int samples, lines;
  fi >> samples >> lines;
  int total_pixels = samples * lines;

  int16_t** pixel_matrix = (int16_t**)malloc(total_pixels * sizeof(int16_t*));
  for (int i = 0; i < total_pixels; i++)
    pixel_matrix[i] = (int16_t*)calloc(bands, sizeof(int16_t));

  for (int idx = 0; idx < total_pixels; idx++) {
    int ref_index;
    kekm_result match;
    if (!(fi >> ref_index >> match.epsilon >> match.delta_y >> match.k_m))
      break;
    if (ref_index < 0 || ref_index >= total_refs) continue;
    apply_inverse(flat[ref_index], &match, bands, pixel_matrix[idx]);
  }
  fi.close();

  for (int i = 0; i < total_refs; i++) free(flat[i]);
  free(flat);

  return pixel_matrix;
}

// Восстановление из бинарных GSD-файлов.
int16_t** decompress_from_gsd_files(const char* standarts_gsd,
                                    const char* image_gsd) {
  // --- Загрузка эталонов ---
  FILE* fs = fopen(standarts_gsd, "rb");
  if (!fs) throw std::runtime_error("Cannot open standarts GSD");

  int32_t total_refs, bands;
  fread(&total_refs, sizeof(int32_t), 1, fs);
  fread(&bands,      sizeof(int32_t), 1, fs);

  int16_t** flat = (int16_t**)malloc(total_refs * sizeof(int16_t*));
  for (int i = 0; i < total_refs; i++) {
    flat[i] = (int16_t*)malloc(bands * sizeof(int16_t));
    fread(flat[i], sizeof(int16_t), bands, fs);
  }
  fclose(fs);

  // --- Загрузка сжатых пикселей ---
  FILE* fi = fopen(image_gsd, "rb");
  if (!fi) {
    for (int i = 0; i < total_refs; i++) free(flat[i]);
    free(flat);
    throw std::runtime_error("Cannot open image GSD");
  }

  int32_t samples, lines;
  fread(&samples, sizeof(int32_t), 1, fi);
  fread(&lines,   sizeof(int32_t), 1, fi);
  int total_pixels = samples * lines;

  int16_t** pixel_matrix = (int16_t**)malloc(total_pixels * sizeof(int16_t*));
  for (int i = 0; i < total_pixels; i++)
    pixel_matrix[i] = (int16_t*)calloc(bands, sizeof(int16_t));

  for (int idx = 0; idx < total_pixels; idx++) {
    int32_t ref_index;
    kekm_result match;
    if (fread(&ref_index,      sizeof(int32_t), 1, fi) != 1) break;
    if (fread(&match.epsilon,  sizeof(double),  1, fi) != 1) break;
    if (fread(&match.delta_y,  sizeof(double),  1, fi) != 1) break;
    if (fread(&match.k_m,      sizeof(double),  1, fi) != 1) break;
    if (ref_index < 0 || ref_index >= total_refs) continue;
    apply_inverse(flat[ref_index], &match, bands, pixel_matrix[idx]);
  }
  fclose(fi);

  for (int i = 0; i < total_refs; i++) free(flat[i]);
  free(flat);

  return pixel_matrix;
}
