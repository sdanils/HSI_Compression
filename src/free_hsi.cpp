#include <cstdint>  // Для int16_t
#include <cstdlib>  // Для exit, malloc, free

#include "functions.h"

void free_hsi_data(int16_t** data, int total_pixels) {
  for (int i = 0; i < total_pixels; i++) {
    free(data[i]);
  }
  free(data);
}