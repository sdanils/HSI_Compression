#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "check_pixel.h"
#include "compression.h"

static const int16_t* select_pixel(const int16_t* pixel, const int16_t* zero_pixel,
                                   int bands) {
  int neg_count = 0;
  for (int b = 0; b < bands; b++)
    if (pixel[b] < 0) neg_count++;
  return (neg_count > bands / 2) ? zero_pixel : pixel;
}

int add_standart_data(compressed_image* compressed_data, int* capacity,
                      standart_data* new_elem) {
  if (compressed_data->size >= *capacity) {
    int new_capacity = (*capacity == 0) ? 4 : (*capacity * 2);

    standart_data** tmp = (standart_data**)realloc(
        compressed_data->image, new_capacity * sizeof(standart_data*));
    if (!tmp) { fprintf(stderr, "realloc failed in add_standart_data\n"); return -1; }
    compressed_data->image = tmp;
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
    const int16_t* pixel_to_compress =
        select_pixel(pixel_matrix[i], zero_pixel, header->bands);

    check_pixel(pixel_to_compress, compressed_data, header->bands, settings,
                &result);

    if (add_standart_data(compressed_data, &capacity, &result) != 0) {
      free(zero_pixel);
      throw std::runtime_error("add_standart_data: нехватка памяти");
    }
  }

  free(zero_pixel);
  if (compressed_data->size > 0) {
    standart_data** tmp = (standart_data**)realloc(
        compressed_data->image, compressed_data->size * sizeof(standart_data*));
    if (tmp) compressed_data->image = tmp;
  }

  return 0;
}