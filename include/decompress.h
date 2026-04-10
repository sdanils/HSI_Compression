#pragma once

#include <cstdint>

#include "compressed_image.h"
#include "hsi_header.h"

/**
 * @brief Восстанавливает матрицу пикселей из сжатых данных в памяти.
 * Применяет обратное аффинное преобразование: ŷ[b] = round(k_m * ref[b] + delta_y).
 * NT: k_m=1, delta_y=0 — копия эталона.
 * OT: k_m=1            — эталон + смещение по среднему.
 * ST: delta_y=0        — масштабированный эталон.
 * AT: оба параметра    — полное аффинное восстановление.
 * @param comp_data Сжатые данные с эталонами и результатами сопоставления.
 * @param header    Метаданные изображения.
 * @return Восстановленная матрица пикселей [lines*samples][bands].
 */
int16_t** decompress(const compressed_image* comp_data,
                     const hsi_header* header);

/**
 * @brief Восстанавливает матрицу пикселей из двух файлов без данных в памяти.
 * Формат standarts_file: <total> <bands> / строка на каждый под-эталон.
 * Формат image_file: <samples> <lines> / <ref_index> <epsilon> <delta_y> <k_m>.
 * @param standarts_file Путь к файлу эталонов.
 * @param image_file     Путь к файлу сжатого изображения.
 * @return Восстановленная матрица пикселей [lines*samples][bands].
 */
int16_t** decompress_from_files(const char* standarts_file,
                                const char* image_file);
