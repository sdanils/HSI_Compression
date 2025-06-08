#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "compression_settings.h"
#include "functions.h"
#include "standart_data.h"

void add_standart(const int16_t* pixel, compressed_image* compressed_data,
                  int bands, standart_data* result) {
  (compressed_data->hsi_standarts) =
      (int16_t***)realloc(compressed_data->hsi_standarts,
                          (compressed_data->num_ref + 1) * sizeof(int16_t**));
  (compressed_data->hsi_standarts)[compressed_data->num_ref] =
      (int16_t**)malloc(sizeof(int16_t*));
  (compressed_data->hsi_standarts)[compressed_data->num_ref][0] =
      (int16_t*)malloc(bands * sizeof(int16_t));
  memcpy((compressed_data->hsi_standarts)[compressed_data->num_ref][0], pixel,
         bands * sizeof(int16_t));

  (compressed_data->ref_counts) =
      (int*)realloc(compressed_data->ref_counts,
                    (compressed_data->num_ref + 1) * sizeof(int));
  (compressed_data->ref_counts)[compressed_data->num_ref] = 1;
  (compressed_data->num_ref)++;

  result->main = compressed_data->num_ref;
  result->additional = 0;
  result->mse = -1;
}

void add_internal_standart(const int16_t* pixel,
                           compressed_image* compressed_data, int bands,
                           int best_i, standart_data* result) {
  int new_j = (compressed_data->ref_counts)[best_i];
  (compressed_data->hsi_standarts)[best_i] = (int16_t**)realloc(
      (compressed_data->hsi_standarts)[best_i], (new_j + 1) * sizeof(int16_t*));
  (compressed_data->hsi_standarts)[best_i][new_j] =
      (int16_t*)malloc(bands * sizeof(int16_t));
  memcpy((compressed_data->hsi_standarts)[best_i][new_j], pixel,
         bands * sizeof(int16_t));
  (compressed_data->ref_counts)[best_i]++;

  result->main = best_i;
  result->additional = new_j;
  result->mse = -1;
}

void check_pixel(const int16_t* pixel, compressed_image* compressed_data,
                 int bands, compression_settings* settings,
                 standart_data* result) {
  // Проверка по основным эталонам
  double min_mse = 1000000;
  int best_i = -1;

  for (int i = 0; i < compressed_data->num_ref; i++) {
    double mse =
        calculate_mse(pixel, (compressed_data->hsi_standarts)[i][0], bands);
    if (mse < min_mse) {
      min_mse = mse;
      best_i = i;
    }
  }

  if (min_mse > settings->main_error) {
    add_standart(pixel, compressed_data, bands, result);
  } else {
    // Проверка по внутренним эталонам
    double min_additional_mse = 1000000;
    int best_j = -1;

    for (int j = 0; j < (compressed_data->ref_counts)[best_i]; j++) {
      double mse = calculate_mse(
          pixel, (compressed_data->hsi_standarts)[best_i][j], bands);
      if (mse < min_additional_mse) {
        min_additional_mse = mse;
        best_j = j;
      }
    }

    if (min_additional_mse <= settings->additional_error) {
      result->main = best_i;
      result->additional = best_j;
      result->mse = min_additional_mse;
    } else {
      add_internal_standart(pixel, compressed_data, bands, best_i, result);
    }
  }
}