#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "check_pixel.h"
#include "compression.h"

int add_standart_data(compressed_image* compressed_data, int* capacity,
                      standart_data* new_elem) {
  if (compressed_data->size >= *capacity) {
    int new_capacity = (*capacity == 0) ? 4 : (*capacity * 2);

    compressed_data->image = (standart_data**)realloc(
        compressed_data->image, new_capacity * sizeof(standart_data*));
    *capacity = new_capacity;
  }

  standart_data* elem_copy = (standart_data*)malloc(sizeof(standart_data));
  if (elem_copy == NULL) {
    return -1;
  }
  memcpy(elem_copy, new_elem, sizeof(standart_data));

  compressed_data->image[compressed_data->size] = elem_copy;
  compressed_data->size++;

  return 0;
}

int compression(int16_t** pixel_matrix, hsi_header* header,
                compressed_image* compressed_data,
                compression_settings* settings) {
  int capacity = 0;
  standart_data result = {-1, {0.0, 0.0, 1.0}};

  int16_t* zero_pixel = (int16_t*)calloc(header->bands, sizeof(int16_t));

  int total_pixel = header->samples * header->lines;
  for (int i = 0; i < total_pixel; i++) {
    int16_t* current_pixel = pixel_matrix[i];
    int neg_count = 0;
    for (int b = 0; b < header->bands; b++)
      if (current_pixel[b] < 0) neg_count++;
    const int16_t* pixel_to_compress =
        (neg_count > header->bands / 2) ? zero_pixel : current_pixel;

    check_pixel(pixel_to_compress, compressed_data, header->bands, settings,
                &result);
    add_standart_data(compressed_data, &capacity, &result);
  }

  free(zero_pixel);
  compressed_data->image = (standart_data**)realloc(
      compressed_data->image, compressed_data->size * sizeof(standart_data*));

  return 0;
}