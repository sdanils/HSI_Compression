#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "compressed_image.h"
#include "compression_settings.h"
#include "functions.h"
#include "hsi_header.h"
#include "standart_data.h"

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
  standart_data result = {-1, -1, 100000};

  int total_pixel = header->samples * header->lines;
  for (int i = 0; i < total_pixel; i++) {
    int16_t* current_pixel = pixel_matrix[i];
    if (current_pixel[0] == -1 || current_pixel[0] == 0)
      continue;  // Пропускаем пиксели заглушки

    check_pixel(current_pixel, compressed_data, header->bands, settings,
                &result);
    add_standart_data(compressed_data, &capacity, &result);
  }
  compressed_data->image = (standart_data**)realloc(
      compressed_data->image, compressed_data->size * sizeof(standart_data*));

  return 0;
}