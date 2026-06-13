#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "save.h"
#include "standart_data.h"

// ---------- GSD binary ----------

void save_standarts_gsd(const compressed_image* img, const hsi_header* header,
                        const char* filename) {
  FILE* f = fopen(filename, "wb");
  if (!f) throw std::runtime_error("Cannot open standarts GSD for writing");

  int32_t total = 0;
  for (int i = 0; i < img->num_ref; i++) total += img->ref_counts[i];
  int32_t bands = header->bands;

  fwrite(&total, sizeof(int32_t), 1, f);
  fwrite(&bands, sizeof(int32_t), 1, f);

  for (int i = 0; i < img->num_ref; i++)
    for (int j = 0; j < img->ref_counts[i]; j++)
      fwrite(img->hsi_standarts[i][j], sizeof(int16_t), bands, f);

  fclose(f);
}

void save_compressed_image_gsd(const compressed_image* img,
                               const hsi_header* header,
                               const char* filename) {
  FILE* f = fopen(filename, "wb");
  if (!f) throw std::runtime_error("Cannot open image GSD for writing");

  int32_t samples = header->samples;
  int32_t lines   = header->lines;
  fwrite(&samples, sizeof(int32_t), 1, f);
  fwrite(&lines,   sizeof(int32_t), 1, f);

  for (int idx = 0; idx < img->size; ++idx) {
    const standart_data* sd = img->image[idx];
    int32_t ref = sd->ref_index;
    fwrite(&ref,             sizeof(int32_t), 1, f);
    fwrite(&sd->match.epsilon, sizeof(double),  1, f);
    fwrite(&sd->match.delta_y, sizeof(double),  1, f);
    fwrite(&sd->match.k_m,     sizeof(double),  1, f);
  }

  fclose(f);
}

void save_hsi_data(int16_t** pixel_matrix, const hsi_header* header,
                   const char* filename) {
  FILE* f = fopen(filename, "wb");
  if (!f) throw std::runtime_error("Cannot open output file for writing");

  int total_pixels = header->lines * header->samples;
  for (int i = 0; i < total_pixels; i++) {
    if (header->byte_order == 1) {
      for (int b = 0; b < header->bands; b++) {
        uint16_t raw = (uint16_t)pixel_matrix[i][b];
        uint16_t swapped = (uint16_t)((raw >> 8) | (raw << 8));
        fwrite(&swapped, sizeof(uint16_t), 1, f);
      }
    } else {
      fwrite(pixel_matrix[i], sizeof(int16_t), header->bands, f);
    }
  }

  fclose(f);
}
