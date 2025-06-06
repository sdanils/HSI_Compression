#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "functions.h"
#include "match_resalt.h"

match_result check_pixel(const int16_t* pixel, int16_t**** hsi_standarts,
                         int* num_ref, int** ref_counts, int bands,
                         double main_error, double additional_error) {
  double min_mse = 10000;
  int best_i = -1;

  for (int i = 0; i < *num_ref; i++) {
    double mse = calculate_mse(pixel, (*hsi_standarts)[i][0], bands);
    if (mse < min_mse) {
      min_mse = mse;
      best_i = i;
    }
  }

  if (min_mse > main_error) {
    (*hsi_standarts) =
        (int16_t***)realloc(*hsi_standarts, (*num_ref + 1) * sizeof(int16_t**));
    (*hsi_standarts)[*num_ref] = (int16_t**)malloc(sizeof(int16_t*));
    (*hsi_standarts)[*num_ref][0] = (int16_t*)malloc(bands * sizeof(int16_t));
    memcpy((*hsi_standarts)[*num_ref][0], pixel, bands * sizeof(int16_t));

    (*ref_counts) = (int*)realloc(*ref_counts, (*num_ref + 1) * sizeof(int));
    (*ref_counts)[*num_ref] = 1;
    (*num_ref)++;

    return match_result{-1, -1, 10000};
  }

  for (int j = 1; j < (*ref_counts)[best_i]; j++) {
    double mse = calculate_mse(pixel, (*hsi_standarts)[best_i][j], bands);
    if (mse <= additional_error) {
      return match_result{best_i, j, mse};
    }
  }

  int new_j = (*ref_counts)[best_i];
  (*hsi_standarts)[best_i] = (int16_t**)realloc((*hsi_standarts)[best_i],
                                                (new_j + 1) * sizeof(int16_t*));
  (*hsi_standarts)[best_i][new_j] = (int16_t*)malloc(bands * sizeof(int16_t));
  memcpy((*hsi_standarts)[best_i][new_j], pixel, bands * sizeof(int16_t));
  (*ref_counts)[best_i]++;

  return match_result{best_i, new_j, 10000};
}