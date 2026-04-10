#pragma once

#include <cstdint>

#include "hsi_header.h"

/**
 * @brief Загружает пиксельную матрицу из бинарного файла (.gsd, BIP, int16).
 * @param dat_path Путь к бинарному файлу с данными.
 * @param header   Метаданные изображения (samples, lines, bands, byte_order).
 * @return Матрица пикселей [lines*samples][bands].
 */
int16_t** load_hsi_data(const char* dat_path, const hsi_header* header);

/**
 * @brief Читает метаданные из файла заголовка ENVI (.hdr).
 * @param hdr_path Путь к файлу заголовка.
 * @param header   Структура для заполнения.
 */
void read_hdr_file(const char* hdr_path, hsi_header* header);
