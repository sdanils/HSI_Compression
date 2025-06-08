#include <cstdint>  // Для int16_t
#include <cstdlib>  // Для exit, malloc, free

#include "functions.h"

void free_hsi_data(int16_t** data, hsi_header* header) {
  int total_pixels = header->lines * header->samples;
  for (int i = 0; i < total_pixels; i++) {
    free(data[i]);
  }
  free(data);
}

void free_compressed_image(struct compressed_image* img) {
  if (img == NULL) {
    return;
  }

  if (img->hsi_standarts != NULL) {
    for (int i = 0; i < img->num_ref; i++) {
      if (img->hsi_standarts[i] != NULL) {
        for (int j = 0; j < img->ref_counts[i]; j++) {
          free(img->hsi_standarts[i][j]);
        }
        free(img->hsi_standarts[i]);
      }
    }
    free(img->hsi_standarts);
  }

  if (img->ref_counts != NULL) {
    free(img->ref_counts);
  }

  if (img->image != NULL) {
    for (int i = 0; i < img->size; i++) {
      if (img->image[i] != NULL) {
        free(img->image[i]);
      }
    }
    free(img->image);
  }
}