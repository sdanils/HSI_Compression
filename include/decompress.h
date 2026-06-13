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
 * @brief Восстанавливает матрицу пикселей из двух бинарных GSD-файлов.
 * Формат standarts_gsd: [int32 total][int32 bands][int16 × total × bands].
 * Формат image_gsd: [int32 samples][int32 lines][int32 ref + double×3 per pixel].
 * @param standarts_gsd Путь к бинарному файлу эталонов.
 * @param image_gsd     Путь к бинарному файлу сжатого изображения.
 * @param out_header    Если не nullptr, заполняет samples/lines/bands из файлов.
 * @return Восстановленная матрица пикселей [lines*samples][bands].
 */
int16_t** decompress_from_gsd_files(const char* standarts_gsd,
                                    const char* image_gsd,
                                    hsi_header* out_header = nullptr);
