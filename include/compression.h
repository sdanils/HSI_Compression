#pragma once

#include <cstdint>

#include "compressed_image.h"
#include "compression_settings.h"
#include "hsi_header.h"
#include "standart_data.h"

/**
 * @brief Добавляет результат сопоставления пикселя в массив image.
 * Удваивает ёмкость массива при переполнении.
 * @param compressed_data Структура сжатого изображения.
 * @param capacity        Текущая ёмкость массива.
 * @param new_elem        Добавляемый элемент.
 * @return 0 при успехе, -1 при ошибке выделения памяти.
 */
int add_standart_data(compressed_image* compressed_data, int* capacity,
                      standart_data* new_elem);

/**
 * @brief Выполняет сжатие изображения двухкритериальным алгоритмом.
 * Пиксели с более чем половиной отрицательных значений заменяются нулевым вектором.
 * @param pixel_matrix    Матрица пикселей [lines*samples][bands].
 * @param header          Метаданные изображения.
 * @param compressed_data Структура для результатов сжатия.
 * @param settings        Параметры сжатия.
 * @return 0 при успешном завершении.
 */
int compression(int16_t** pixel_matrix, hsi_header* header,
                compressed_image* compressed_data,
                compression_settings* settings);
