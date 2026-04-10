#pragma once

#include <cstdint>

#include "compressed_image.h"
#include "hsi_header.h"

/**
 * @brief Освобождает матрицу пикселей.
 * @param data   Матрица пикселей [lines*samples][bands].
 * @param header Метаданные (используется lines, samples).
 */
void free_hsi_data(int16_t** data, hsi_header* header);

/**
 * @brief Освобождает структуру сжатого изображения (эталоны и массив image).
 * @param img Структура сжатого изображения.
 */
void free_compressed_image(compressed_image* img);

/**
 * @brief Выводит значения всех полос пикселя в stdout.
 * @param pixel Пиксель [bands].
 * @param bands Количество спектральных каналов.
 */
void print_pixel(const int16_t* pixel, int bands);
