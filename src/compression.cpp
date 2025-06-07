#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "compressed_image.h"
#include "compression_settings.h"
#include "functions.h"
#include "hsi_header.h"
#include "match_resalt.h"

int add_match_result(compressed_image* compressed_data, int* capacity,
                     match_result* new_elem) {
  if (compressed_data->size >= *capacity) {
    int new_capacity = (*capacity == 0) ? 4 : (*capacity * 2);

    compressed_data->image = (match_result**)realloc(
        compressed_data->image, new_capacity * sizeof(match_result*));
    *capacity = new_capacity;
  }

  match_result* elem_copy = (match_result*)malloc(sizeof(match_result));
  if (elem_copy == NULL) {
    return -1;
  }
  memcpy(elem_copy, new_elem, sizeof(match_result));

  compressed_data->image[compressed_data->size] = elem_copy;
  compressed_data->size++;

  return 0;
}

int compression(int16_t** pixel_matrix, hsi_header* header,
                compressed_image* compressed_data,
                compression_settings* settings) {
  int capacity = 0;
  match_result result = {-1, -1, 100000};

  int total_pixel = header->samples * header->lines;
  for (int i = 0; i < total_pixel; i++) {
    int16_t* current_pixel = pixel_matrix[i];
    if (current_pixel[0] == -1 || current_pixel[0] == 0)
      continue;  // Пропускаем пиксели заглушки

    check_pixel(current_pixel, compressed_data, header->bands, settings,
                &result);
    add_match_result(compressed_data, &capacity, &result);
  }
  compressed_data->image = (match_result**)realloc(
      compressed_data->image, compressed_data->size * sizeof(match_result*));

  return 0;
}