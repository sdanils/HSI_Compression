#include "kekm.h"

#include <math.h>
#include <stdint.h>

static double mean(const int16_t* p, int n) {
    double s = 0;
    for (int i = 0; i < n; i++) s += p[i];
    return s / n;
}

static double moment2(const int16_t* p, int n) {
    double s = 0;
    for (int i = 0; i < n; i++) s += (double)p[i] * p[i];
    return s / n;
}

static double cov(const int16_t* a, const int16_t* b, int n) {
    double ma = mean(a, n), mb = mean(b, n), s = 0;
    for (int i = 0; i < n; i++) s += ((double)a[i] - ma) * ((double)b[i] - mb);
    return s / n;
}

static double cross(const int16_t* a, const int16_t* b, int n) {
    double s = 0;
    for (int i = 0; i < n; i++) s += (double)a[i] * b[i];
    return s / n;
}

static double disp(const int16_t* p, int n) {
    double m = mean(p, n);
    double s = 0;
    for (int i = 0; i < n; i++) s += ((double)p[i] - m) * ((double)p[i] - m);
    return s / n;
}

// (1) Неинвариантная оценка (НП)
kekm_result kekm_nt(const int16_t* ref, const int16_t* pix, int bands) {
    double m2e = moment2(ref, bands);
    double m2  = moment2(pix, bands);
    double mey = cross(ref, pix, bands);
    kekm_result r;
    r.epsilon = m2e + m2 - 2.0 * mey;
    r.delta_y = 0.0;
    r.k_m = 1.0;
    return r;
}

// (2) Инвариантная к ортогональным преобразованиям (ОП)
kekm_result kekm_ot(const int16_t* ref, const int16_t* pix, int bands) {
    double de = disp(ref, bands);
    double dp = disp(pix, bands);
    double c  = cov(ref, pix, bands);
    kekm_result r;
    r.epsilon = de + dp - 2.0 * c;
    r.delta_y = mean(pix, bands) - mean(ref, bands);
    r.k_m = 1.0;
    return r;
}

// (3) Инвариантная к масштабированию (МП)
kekm_result kekm_st(const int16_t* ref, const int16_t* pix, int bands) {
    double mey = cross(ref, pix, bands);
    double m2e = moment2(ref, bands);
    double myy = moment2(pix, bands);
    kekm_result r;
    if (m2e <= 0.0) {
        // Вырожденный (нулевой) эталон: масштабирование не определено.
        // Откатываемся на НП — k_m = 1, epsilon = M²(y) (= формула НП при e ≡ 0).
        r.k_m = 1.0;
        r.delta_y = 0.0;
        r.epsilon = myy;
        return r;
    }
    r.k_m = mey / m2e;
    r.epsilon = myy - mey * mey / m2e;
    r.delta_y = 0.0;
    return r;
}

// (4) Инвариантная к аффинным преобразованиям (АП)
kekm_result kekm_at(const int16_t* ref, const int16_t* pix, int bands) {
    double de = disp(ref, bands);
    double dp = disp(pix, bands);
    double c  = cov(ref, pix, bands);
    kekm_result r;
    if (de <= 0.0) {
        // Константный эталон: аффинный масштаб не определён (cov тоже = 0).
        // Откатываемся на ОП — k_m = 1, epsilon = D(y).
        r.k_m = 1.0;
        r.delta_y = mean(pix, bands) - mean(ref, bands);
        r.epsilon = dp;
        return r;
    }
    r.epsilon = dp - c * c / de;
    r.k_m = c / de;
    r.delta_y = mean(pix, bands) - r.k_m * mean(ref, bands);
    return r;
}

double pixel_norm(const int16_t* pix, int bands, kekm_method method) {
    if (method == KEKM_OT || method == KEKM_AT)
        return disp(pix, bands);
    return moment2(pix, bands);
}

kekm_result pixel_distance(const int16_t* ref, const int16_t* pix, int bands,
                           kekm_method method) {
  switch (method) {
    case KEKM_OT: return kekm_ot(ref, pix, bands);
    case KEKM_ST: return kekm_st(ref, pix, bands);
    case KEKM_AT: return kekm_at(ref, pix, bands);
    default:      return kekm_nt(ref, pix, bands);
  }
}
