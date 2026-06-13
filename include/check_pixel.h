#pragma once

#include <cstdint>

#include "compressed_image.h"
#include "compression_settings.h"
#include "standart_data.h"

/**
 * @brief Добавляет новый основной эталон в сжатые данные.
 * @param pixel           Пиксель, который станет эталоном.
 * @param compressed_data Структура сжатых данных.
 * @param bands           Количество спектральных каналов.
 * @param result          Результат добавления.
 */
void add_standart(const int16_t* pixel, compressed_image* compressed_data,
                  int bands, standart_data* result);

/**
 * @brief Добавляет под-эталон к существующему основному эталону.
 * @param pixel   Пиксель, который станет под-эталоном.
 * @param cd      Структура сжатых данных.
 * @param bands   Количество спектральных каналов.
 * @param best_i  Индекс основного эталона.
 * @param result  Результат добавления.
 */
void add_internal_standart(const int16_t* pixel, compressed_image* cd,
                           int bands, int best_i, standart_data* result);

/**
 * @brief Проверяет пиксель по двухуровневому алгоритму и записывает результат.
 * Уровень 1: сравнение с базовыми векторами основных эталонов.
 * Уровень 2: сравнение с под-эталонами ближайшего основного.
 * @param pixel           Пиксель для проверки.
 * @param compressed_data Структура сжатых данных.
 * @param bands           Количество спектральных каналов.
 * @param settings        Параметры сжатия (пороги и метод).
 * @param result          Результат сопоставления.
 */
void check_pixel(const int16_t* pixel, compressed_image* compressed_data,
                 int bands, const compression_settings* settings,
                 standart_data* result);
