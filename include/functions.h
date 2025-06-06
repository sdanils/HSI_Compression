#pragma once

#include <cstdint>  // Для int16_t

#include "match_resalt.h"
#include "struct_hsi.h"

/**
 * @brief Вычисляет среднеквадратичную ошибку (MSE) между двумя спектрами.
 * @param pixel1 Спектр пикселя 1 (массив int16_t)
 * @param pixel2 Спектр пикселя 2 (массив int16_t)
 * @param bands Количество спектральных каналов
 * @return MSE (double)
 */
double calculate_mse(const int16_t* pixel1, const int16_t* pixel2, int bands);

/**
 * @brief Добавляет уникальные пиксели в эталонный набор.
 * @param hsi_data Входные данные [total_pixels][bands]
 * @param hsi_t Эталонный набор (изначально пустой)
 * @param total_pixels Число пикселей в hsi_data
 * @param bands Число спектральных каналов
 * @param threshold Порог MSE для добавления
 * @param[out] num_ref_pixels Количество эталонных пикселей (обновляется)
 * @return Обновлённый hsi_t (int16_t**)
 */
int16_t** get_standarts(int16_t** hsi_data, int16_t** hsi_t, int total_pixels,
                        int bands, double threshold, int* num_ref_pixels);

/**
 * @brief Проверяет пиксель и добавляет в hsi_standarts при необходимости.
 * @param pixel Входной пиксель [bands]
 * @param hsi_standarts Ссылка на Эталонный массив [i][j][bands]
 * @param num_ref Текущее число основных эталонов (i)
 * @param ref_counts Массив с количеством пикселей в каждом кластере (j)
 * @param bands Число каналов
 * @param main_error Порог для основных эталонов
 * @param additional_error Порог для дополнительных пикселей
 * @return match_result (i, j, mse) или (-1, -1, INF) если добавлен новый эталон
 */
match_result check_pixel(const int16_t* pixel, int16_t**** hsi_standarts,
                         int* num_ref, int** ref_counts, int bands,
                         double main_error, double additional_error);

int16_t** load_hsi_data(const char* dat_path, const HSI_Header* header);
void read_hdr_file(const char* hdr_path, HSI_Header* header);
void free_hsi_data(int16_t** data, int lines);