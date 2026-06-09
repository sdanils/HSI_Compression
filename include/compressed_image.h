#pragma once
#include <cstdint>

#include "standart_data.h"

/**
 * @brief Структура, хранящая сжатое гиперспектральное изображение.
 */
struct compressed_image {
  int16_t*** hsi_standarts; /**< 3D массив эталонов: [num_ref][ref_counts[i]][bands] */
  int num_ref;              /**< Количество основных эталонов */
  int* ref_counts;          /**< Число под-эталонов для каждого основного */

  standart_data** image; /**< Результаты сжатия пикселей */
  int size;              /**< Число пикселей в image */
};
