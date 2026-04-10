#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "check_pixel.h"
#include "kekm.h"

// Возвращает плоский глобальный индекс под-эталона best_i[sub_j].
// ref_counts должны быть уже актуальны (до добавления нового элемента).
static int flat_index(const compressed_image* cd, int main_i, int sub_j) {
  int flat = 0;
  for (int k = 0; k < main_i; k++) flat += cd->ref_counts[k];
  return flat + sub_j;
}

void add_standart(const int16_t* pixel, compressed_image* cd, int bands,
                  standart_data* result) {
  cd->hsi_standarts = (int16_t***)realloc(
      cd->hsi_standarts, (cd->num_ref + 1) * sizeof(int16_t**));
  cd->hsi_standarts[cd->num_ref] = (int16_t**)malloc(sizeof(int16_t*));
  cd->hsi_standarts[cd->num_ref][0] = (int16_t*)malloc(bands * sizeof(int16_t));
  memcpy(cd->hsi_standarts[cd->num_ref][0], pixel, bands * sizeof(int16_t));

  cd->ref_counts = (int*)realloc(cd->ref_counts,
                                 (cd->num_ref + 1) * sizeof(int));
  cd->ref_counts[cd->num_ref] = 1;

  // Плоский индекс нового эталона — сумма всех предыдущих под-эталонов
  result->ref_index = flat_index(cd, cd->num_ref, 0);
  kekm_result zero = {0.0, 0.0, 1.0};
  result->match = zero;
  cd->num_ref++;
}

void add_internal_standart(const int16_t* pixel, compressed_image* cd,
                           int bands, int best_i, standart_data* result) {
  int new_j = cd->ref_counts[best_i];
  cd->hsi_standarts[best_i] = (int16_t**)realloc(
      cd->hsi_standarts[best_i], (new_j + 1) * sizeof(int16_t*));
  cd->hsi_standarts[best_i][new_j] = (int16_t*)malloc(bands * sizeof(int16_t));
  memcpy(cd->hsi_standarts[best_i][new_j], pixel, bands * sizeof(int16_t));

  // Вычисляем плоский индекс до инкремента ref_counts
  result->ref_index = flat_index(cd, best_i, new_j);
  cd->ref_counts[best_i]++;
  kekm_result zero = {0.0, 0.0, 1.0};
  result->match = zero;
}

void check_pixel(const int16_t* pixel, compressed_image* cd, int bands,
                 compression_settings* settings, standart_data* result) {
  // Уровень 1: ищем ближайший основной эталон
  double min_eps = 1e15;
  int best_i = -1;

  for (int i = 0; i < cd->num_ref; i++) {
    kekm_result r =
        pixel_distance(pixel, cd->hsi_standarts[i][0], bands, settings->method);
    if (r.epsilon < min_eps) {
      min_eps = r.epsilon;
      best_i = i;
    }
  }

  if (best_i == -1 || min_eps > settings->main_error) {
    add_standart(pixel, cd, bands, result);
    return;
  }

  // Уровень 2: ищем ближайший под-эталон внутри best_i
  double min_sub = 1e15;
  int best_j = -1;
  kekm_result best_r = {0.0, 0.0, 1.0};

  for (int j = 0; j < cd->ref_counts[best_i]; j++) {
    kekm_result r = pixel_distance(pixel, cd->hsi_standarts[best_i][j], bands,
                                   settings->method);
    if (r.epsilon < min_sub) {
      min_sub = r.epsilon;
      best_j = j;
      best_r = r;
    }
  }

  if (min_sub > settings->additional_error) {
    add_internal_standart(pixel, cd, bands, best_i, result);
  } else {
    result->ref_index = flat_index(cd, best_i, best_j);
    result->match = best_r;
  }
}
