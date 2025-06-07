#include <cstdint>  // Для int16_t
#include <cstdlib>  // Для exit, malloc, free
#include <cstring>  // Для strtok, strcmp
#include <fstream>

#include "functions.h"
#include "hsi_header.h"

int16_t** load_hsi_data(const char* dat_path, const hsi_header* header) {
  FILE* dat_file = fopen(dat_path, "rb");
  if (!dat_file) {
    exit(1);
  }

  // Пропускаем смещение (если есть)
  fseek(dat_file, header->header_offset, SEEK_SET);

  // Проверка типа данных (должен быть int16)
  if (header->data_type != 2) {
    exit(1);
  }

  int total_pixels = header->lines * header->samples;
  int16_t** pixel_matrix = (int16_t**)malloc(total_pixels * sizeof(int16_t*));
  for (int i = 0; i < total_pixels; i++) {
    pixel_matrix[i] = (int16_t*)malloc(header->bands * sizeof(int16_t));
  }

  // Чтение данных (BIP)
  for (int i = 0; i < total_pixels; i++) {
    fread(pixel_matrix[i], sizeof(int16_t), header->bands, dat_file);
    if (header->byte_order == 1) {
      for (int b = 0; b < header->bands; b++) {
        pixel_matrix[i][b] =
            (pixel_matrix[i][b] >> 8) | (pixel_matrix[i][b] << 8);
      }
    }
  }

  fclose(dat_file);
  return pixel_matrix;
}