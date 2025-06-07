#pragma once
#include <cstdint>  // Для int16_t

#include "match_resalt.h"

/**
 * @brief Структура, хранящая сжатое гиперспектральное изображение.
 */
struct compressed_image {
  int16_t*** hsi_standarts; /**< 3D массив эталонов:
                               [num_ref][ref_counts[i]][samples] */
  int num_ref;              /**< Количество эталонов */
  int* ref_counts; /**< Массив высот (число строк) для каждого эталона */

  match_result** image; /**< Массив указателей на результаты сжатия пикселей */
  int size;             /**< Размер массива image (число пикселей) */
};