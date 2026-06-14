#include "etalon_shift.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "kekm.h"

static void quantize_to_i16(const double* src, int bands, int16_t* dst) {
  for (int b = 0; b < bands; b++) {
    double v = round(src[b]);
    if (v < 0) v = 0;
    if (v > INT16_MAX) v = INT16_MAX;
    dst[b] = (int16_t)v;
  }
}

// Вычисляет позицию, отталкиваемую от выровненного эталона êᵢ = k_m·ref + Δy
// на величину shift в линейной шкале ρ = √(n·ε).
static void repulse_from_ref(const double* y, const kekm_result* kr,
                              const int16_t* ref, double rho, double shift,
                              int bands, double* out) {
  for (int b = 0; b < bands; b++)
    out[b] = y[b] + (y[b] - (kr->k_m * ref[b] + kr->delta_y)) / rho * shift;
}

void make_shifted_etalon(const int16_t* pixel, int16_t* const* refs,
                         int count, int bands,
                         const compression_settings* settings,
                         double threshold, double step_coef,
                         const int16_t* anchor, double max_anchor_dist,
                         int16_t* out) {
  memcpy(out, pixel, bands * sizeof(int16_t));

  if (!settings->shift_enabled || count <= 0 || threshold <= 0.0) return;

  double target_epsilon = threshold * step_coef;
  double target_rho = sqrt((double)bands * target_epsilon);

  double*  y         = (double*)  malloc(bands * sizeof(double));
  double*  y_tmp     = (double*)  malloc(bands * sizeof(double));
  int16_t* y_i16     = (int16_t*) malloc(bands * sizeof(int16_t));
  int16_t* y_tmp_i16 = (int16_t*) malloc(bands * sizeof(int16_t));
  if (!y || !y_tmp || !y_i16 || !y_tmp_i16) {
    free(y); free(y_tmp); free(y_i16); free(y_tmp_i16);
    return;
  }

  for (int b = 0; b < bands; b++) {
    y[b]     = pixel[b];
    y_i16[b] = pixel[b];
  }

  bool check_anchor = anchor && max_anchor_dist > 0.0;

  for (int i = 0; i < count; i++) {
    kekm_result kr = pixel_distance(refs[i], y_i16, bands, settings->method);
    if (!std::isfinite(kr.epsilon) || kr.epsilon <= 0.0 || kr.epsilon >= target_epsilon)
      continue;

    double rho = sqrt((double)bands * kr.epsilon);
    double shift = target_rho - rho;
    if (rho <= 1e-9 || shift <= 1e-9) continue;

    repulse_from_ref(y, &kr, refs[i], rho, shift, bands, y_tmp);
    quantize_to_i16(y_tmp, bands, y_tmp_i16);

    if (check_anchor) {
      kekm_result ar = pixel_distance(anchor, y_tmp_i16, bands, settings->method);
      if (!std::isfinite(ar.epsilon) || ar.epsilon > max_anchor_dist) continue;
    }

    memcpy(y,     y_tmp,     bands * sizeof(double));
    memcpy(y_i16, y_tmp_i16, bands * sizeof(int16_t));
  }

  memcpy(out, y_i16, bands * sizeof(int16_t));

  free(y); free(y_tmp);
  free(y_i16); free(y_tmp_i16);
}
