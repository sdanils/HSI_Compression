#pragma once

#include <stdint.h>

typedef enum {
    KEKM_NT = 1,  // неинвариантная (НП)
    KEKM_OT = 2,  // инвариантная к ортогональным преобразованиям (ОП)
    KEKM_ST = 3,  // инвариантная к масштабированию (МП)
    KEKM_AT = 4   // инвариантная к аффинным преобразованиям (АП)
} kekm_method;

typedef struct {
    double epsilon;  // оценка близости
    double delta_y;  // смещение
    double k_m;      // коэффициент масштабирования
} kekm_result;

// (1) Неинвариантная оценка (НП)
kekm_result kekm_nt(const int16_t* ref, const int16_t* pix, int bands);

// (2) Инвариантная к ортогональным преобразованиям (ОП)
kekm_result kekm_ot(const int16_t* ref, const int16_t* pix, int bands);

// (3) Инвариантная к масштабированию (МП)
kekm_result kekm_st(const int16_t* ref, const int16_t* pix, int bands);

// (4) Инвариантная к аффинным преобразованиям (АП)
kekm_result kekm_at(const int16_t* ref, const int16_t* pix, int bands);

// Диспетчер: вызывает нужный метод по method
kekm_result pixel_distance(const int16_t* ref, const int16_t* pix, int bands,
                           kekm_method method);
