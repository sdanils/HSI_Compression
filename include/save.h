#pragma once

#include <cstdint>

#include "compressed_image.h"
#include "hsi_header.h"

/**
 * @brief Сохраняет все под-эталоны в текстовый файл.
 * Формат: <total> <bands> / строка на каждый под-эталон в порядке плоской нумерации.
 * @param img      Сжатые данные с эталонами.
 * @param header   Метаданные (используется bands).
 * @param filename Имя выходного файла (по умолчанию "standarts.txt").
 */
void save_standarts(const compressed_image* img, hsi_header* header,
                    const char* filename = "standarts.txt");

/**
 * @brief Сохраняет сжатое изображение в текстовый файл.
 * Формат: <samples> <lines> / <ref_index> <epsilon> <delta_y> <k_m>.
 * @param img      Сжатые данные.
 * @param header   Метаданные (используется samples, lines).
 * @param filename Имя выходного файла (по умолчанию "image.txt").
 */
void save_compressed_image(const compressed_image* img, const hsi_header* header,
                           const char* filename = "image.txt");

/**
 * @brief Сохраняет RGB-превью в PNG.
 * Выбирает каналы, ближайшие к 660/550/470 нм, применяет растяжку 2–98% и гамма-коррекцию γ=2.2.
 * @param pixel_matrix Матрица пикселей [lines*samples][bands].
 * @param header       Метаданные (samples, lines, bands, wavelengths).
 * @param filename     Имя выходного PNG-файла (по умолчанию "rgb_preview.png").
 */
void save_rgb_image(int16_t** pixel_matrix, const hsi_header* header,
                    const char* filename = "rgb_preview.png");
