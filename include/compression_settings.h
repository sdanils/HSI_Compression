#pragma once

#include "kekm.h"

/**
 * @brief Параметры сжатия.
 *
 * Пороги задаются в процентах (0.0 – 100.0) от нормировочной базы пикселя:
 *   NT/ST → M²(y),  OT/AT → D(y)
 * Пример: main_error_pct=5.0 означает, что расстояние до ближайшего
 * основного эталона не должно превышать 5 % энергии/дисперсии пикселя.
 */
struct compression_settings {
  double main_error_pct;       /**< Порог 1-го уровня, % */
  double additional_error_pct; /**< Порог 2-го уровня, % */
  kekm_method method;          /**< Метод сравнения пикселей (KEKM_NT/OT/ST/AT) */
};
