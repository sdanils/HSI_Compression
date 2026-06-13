#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "check_pixel.h"
#include "etalon_shift.h"
#include "kekm.h"

// Возвращает плоский глобальный индекс под-эталона best_i[sub_j].
static int flat_index(const compressed_image* cd, int main_i, int sub_j) {
  int flat = 0;
  for (int k = 0; k < main_i; k++) flat += cd->ref_counts[k];
  return flat + sub_j;
}

void add_standart(const int16_t* pixel, compressed_image* cd, int bands,
                  standart_data* result) {
  int16_t*** tmp_s = (int16_t***)realloc(cd->hsi_standarts,
                                         (cd->num_ref + 1) * sizeof(int16_t**));
  if (!tmp_s) throw std::runtime_error("realloc failed in add_standart");
  cd->hsi_standarts = tmp_s;

  cd->hsi_standarts[cd->num_ref] = (int16_t**)malloc(sizeof(int16_t*));
  cd->hsi_standarts[cd->num_ref][0] = (int16_t*)malloc(bands * sizeof(int16_t));
  memcpy(cd->hsi_standarts[cd->num_ref][0], pixel, bands * sizeof(int16_t));

  int* tmp_rc = (int*)realloc(cd->ref_counts, (cd->num_ref + 1) * sizeof(int));
  if (!tmp_rc) throw std::runtime_error("realloc failed in add_standart");
  cd->ref_counts = tmp_rc;
  cd->ref_counts[cd->num_ref] = 1;

  result->ref_index = flat_index(cd, cd->num_ref, 0);
  kekm_result zero = {0.0, 0.0, 1.0};
  result->match = zero;
  cd->num_ref++;
}

void add_internal_standart(const int16_t* pixel, compressed_image* cd,
                           int bands, int best_i, standart_data* result) {
  int new_j = cd->ref_counts[best_i];
  int16_t** tmp = (int16_t**)realloc(cd->hsi_standarts[best_i],
                                     (new_j + 1) * sizeof(int16_t*));
  if (!tmp) throw std::runtime_error("realloc failed in add_internal_standart");
  cd->hsi_standarts[best_i] = tmp;
  cd->hsi_standarts[best_i][new_j] = (int16_t*)malloc(bands * sizeof(int16_t));
  memcpy(cd->hsi_standarts[best_i][new_j], pixel, bands * sizeof(int16_t));

  int new_flat = flat_index(cd, best_i, new_j);
  for (int i = 0; i < cd->size; i++) {
    if (cd->image[i]->ref_index >= new_flat)
      cd->image[i]->ref_index++;
  }

  result->ref_index = new_flat;
  cd->ref_counts[best_i]++;
  kekm_result zero = {0.0, 0.0, 1.0};
  result->match = zero;
}

// Записывает в result фактическое соответствие пиксель↔эталон. При вырожденном
// результате (NaN/inf) сохраняется тождественное соответствие {0, 0, 1}.
static void set_match(const int16_t* etalon, const int16_t* pixel, int bands,
                      kekm_method method, standart_data* result) {
  kekm_result r = pixel_distance(etalon, pixel, bands, method);
  if (std::isfinite(r.epsilon) && std::isfinite(r.delta_y) &&
      std::isfinite(r.k_m))
    result->match = r;
}

// Ищет ближайший вектор среди refs[0..count-1].
static int find_nearest(const int16_t* pixel, int16_t** refs, int count,
                        int bands, kekm_method method, kekm_result* best_r) {
  double min_eps = 1e15;
  int best = -1;
  for (int i = 0; i < count; i++) {
    kekm_result r = pixel_distance(refs[i], pixel, bands, method);
    if (r.epsilon < min_eps) {
      min_eps = r.epsilon;
      best = i;
      *best_r = r;
    }
  }
  return best;
}

static void create_main_standart(const int16_t* pixel,
                                  int16_t** main_refs, int num_refs,
                                  compressed_image* cd, int bands,
                                  const compression_settings* settings,
                                  double main_threshold, standart_data* result) {
  int16_t* etalon = (int16_t*)malloc(bands * sizeof(int16_t));
  if (!etalon) throw std::runtime_error("malloc failed in check_pixel");
  make_shifted_etalon(pixel, main_refs, num_refs, bands, settings,
                      main_threshold, settings->step_coef, nullptr, 0.0, etalon);
  add_standart(etalon, cd, bands, result);
  set_match(etalon, pixel, bands, settings->method, result);
  free(etalon);
}

static void create_internal_standart(const int16_t* pixel,
                                      compressed_image* cd, int bands, int best_i,
                                      const compression_settings* settings,
                                      double additional_threshold, double main_threshold,
                                      standart_data* result) {
  int16_t* etalon = (int16_t*)malloc(bands * sizeof(int16_t));
  if (!etalon) throw std::runtime_error("malloc failed in check_pixel");
  make_shifted_etalon(pixel, cd->hsi_standarts[best_i],
                      cd->ref_counts[best_i], bands, settings,
                      additional_threshold, settings->step_coef,
                      cd->hsi_standarts[best_i][0], main_threshold, etalon);
  add_internal_standart(etalon, cd, bands, best_i, result);
  set_match(etalon, pixel, bands, settings->method, result);
  free(etalon);
}

void check_pixel(const int16_t* pixel, compressed_image* cd, int bands,
                 const compression_settings* settings, standart_data* result) {
  double norm = pixel_norm(pixel, bands, settings->method);
  double main_threshold       = settings->main_error_pct       / 100.0 * norm;
  double additional_threshold = settings->additional_error_pct / 100.0 * norm;

  // Уровень 1: ближайший основной эталон (по первому под-эталону каждого)
  int16_t** main_refs = (int16_t**)malloc(cd->num_ref * sizeof(int16_t*));
  if (cd->num_ref > 0 && !main_refs)
    throw std::runtime_error("malloc failed in check_pixel");
  for (int i = 0; i < cd->num_ref; i++)
    main_refs[i] = cd->hsi_standarts[i][0];

  kekm_result best_r1 = {0.0, 0.0, 1.0};
  int best_i = find_nearest(pixel, main_refs, cd->num_ref, bands,
                            settings->method, &best_r1);

  if (best_i == -1 || best_r1.epsilon > main_threshold) {
    create_main_standart(pixel, main_refs, cd->num_ref, cd, bands, settings,
                         main_threshold, result);
    free(main_refs);
    return;
  }
  free(main_refs);

  // Уровень 2: ближайший под-эталон внутри best_i
  kekm_result best_r2 = {0.0, 0.0, 1.0};
  // best_j == -1 если все под-эталоны дали NaN/inf
  int best_j = find_nearest(pixel, cd->hsi_standarts[best_i],
                            cd->ref_counts[best_i], bands,
                            settings->method, &best_r2);

  if (best_j == -1 || best_r2.epsilon > additional_threshold) {
    create_internal_standart(pixel, cd, bands, best_i, settings,
                             additional_threshold, main_threshold, result);
  } else {
    result->ref_index = flat_index(cd, best_i, best_j);
    result->match = best_r2;
  }
}
