#pragma once

#include <cstdint>

#include "compressed_image.h"
#include "hsi_header.h"

/**
 * @brief Сохраняет все под-эталоны в бинарный GSD-файл.
 * Формат: [int32 total][int32 bands][int16 × total × bands] — BIP.
 * @param img      Сжатые данные с эталонами.
 * @param header   Метаданные (используется bands).
 * @param filename Имя выходного файла (по умолчанию "standarts.gsd").
 */
void save_standarts_gsd(const compressed_image* img, const hsi_header* header,
                        const char* filename = "standarts.gsd");

/**
 * @brief Сохраняет сжатое изображение в бинарный GSD-файл.
 * Формат: [int32 samples][int32 lines][per pixel: int32 ref_index, double epsilon, double delta_y, double k_m].
 * @param img      Сжатые данные.
 * @param header   Метаданные (используется samples, lines).
 * @param filename Имя выходного файла (по умолчанию "image.gsd").
 */
void save_compressed_image_gsd(const compressed_image* img, const hsi_header* header,
                               const char* filename = "image.gsd");

/**
 * @brief Сохраняет матрицу пикселей в бинарный файл (int16, BIP).
 * Формат совпадает с входным .gsd: row-major, для каждого пикселя — все bands подряд.
 * При byte_order=1 выполняется перестановка байт (big-endian).
 * @param pixel_matrix Матрица [lines*samples][bands].
 * @param header       Метаданные (samples, lines, bands, byte_order).
 * @param filename     Имя выходного файла.
 */
void save_hsi_data(int16_t** pixel_matrix, const hsi_header* header,
                   const char* filename);

/**
 * @brief Сохраняет RGB-превью в PNG.
 * R/G/B — гауссов-взвешенный блендинг (σ=30 нм) всех каналов вокруг 660/550/470 нм,
 * затем CDF-эквализация гистограммы на канал. Если задан ref_matrix, LUT
 * калибруется по нему (для сопоставимых preview/restored превью).
 * @param pixel_matrix Матрица пикселей [lines*samples][bands].
 * @param header       Метаданные (samples, lines, bands, wavelengths).
 * @param filename     Имя выходного PNG-файла (по умолчанию "rgb_preview.png").
 * @param ref_matrix   Матрица для калибровки LUT или nullptr.
 */
void save_rgb_image(int16_t** pixel_matrix, const hsi_header* header,
                    const char* filename = "rgb_preview.png",
                    int16_t** ref_matrix = nullptr);
