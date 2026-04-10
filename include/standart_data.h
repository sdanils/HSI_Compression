#pragma once

#include "kekm.h"

/**
 * @brief Результат сжатия одного пикселя.
 */
struct standart_data {
  int ref_index;      /**< Индекс эталона */
  kekm_result match;  /**< Результат сравнения: epsilon, delta_y, k_m */
};
