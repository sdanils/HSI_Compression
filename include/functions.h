#pragma once

#include <cstdint>  // Для int16_t

#include "compressed_image.h"
#include "compression_settings.h"
#include "hsi_header.h"
#include "standart_data.h"

/**
 * @brief Вычисляет среднеквадратичную ошибку (MSE) между двумя пикселями.
 * @param pixel1 Указатель на первый пиксель (массив значений).
 * @param pixel2 Указатель на второй пиксель (массив значений).
 * @param bands Количество спектральных каналов (размерность массивов пикселей).
 * @return Среднеквадратичная ошибка (MSE) между двумя пикселями.
 */
double calculate_mse(const int16_t* pixel1, const int16_t* pixel2, int bands);

/**
 * @brief Добавляет новый стандарт (эталон) в сжатые данные.
 * @param pixel Указатель на пиксель, который будет добавлен в эталоны.
 * @param compressed_data Указатель на структуру сжатых данных.
 * @param bands Количество спектральных каналов.
 * @param result Указатель на структуру для хранения результата добавления.
 */
void add_standart(const int16_t* pixel, compressed_image* compressed_data,
                  int bands, standart_data* result);

/**
 * @brief Добавляет внутренний стандарт (эталон) в сжатые данные.
 * @param pixel Указатель на пиксель, который будет добавлен.
 * @param compressed_data Указатель на структуру сжатых данных.
 * @param bands Количество спектральных каналов.
 * @param best_i Индекс основного эталона, к которому добавляется новый.
 * @param result Указатель на структуру для хранения результата добавления.
 */
void add_internal_standart(const int16_t* pixel,
                           compressed_image* compressed_data, int bands,
                           int best_i, standart_data* result);

/**
 * @brief Проверяет пиксель на соответствие стандартам и обновляет результат.
 * @param pixel Указатель на пиксель для проверки.
 * @param compressed_data Указатель на структуру сжатых данных.
 * @param bands Количество спектральных каналов.
 * @param settings Указатель на настройки сжатия.
 * @param result Указатель на структуру для хранения результата проверки.
 */
void check_pixel(const int16_t* pixel, compressed_image* compressed_data,
                 int bands, compression_settings* settings,
                 standart_data* result);

/**
 * @brief Добавляет результат сопоставления в структуру сжатого изображения.
 * Если текущий размер достиг емкости, емкость увеличивается (увеличение вдвое).
 * Создается копия нового элемента и добавляется в массив результатов.
 * @param compressed_data Указатель на структуру сжатого изображения.
 * @param capacity Указатель на текущую емкость массива результатов.
 * @param new_elem Указатель на новый элемент результата сопоставления.
 * @return int Возвращает 0 при успешном добавлении, -1 при ошибке выделения
 * памяти.
 */
int add_match_result(compressed_image* compressed_data, int* capacity,
                     standart_data* new_elem);

/**
 * @brief Выполняет сжатие изображения, анализируя каждый пиксель.
 * Функция проходит по всем пикселям матрицы, пропуская пиксели с первыми
 * значениями -1 или 0. Для каждого подходящего пикселя вызывается функция
 * проверки и результат добавляется в структуру сжатого изображения.
 * @param pixel_matrix Двумерный массив пикселей изображения.
 * @param header Указатель на структуру заголовка с метаданными изображения.
 * @param compressed_data Указатель на структуру для хранения результатов
 * сжатия.
 * @param settings Указатель на настройки сжатия.
 * @return int Возвращает 0 при успешном завершении.
 */
int compression(int16_t** pixel_matrix, hsi_header* header,
                compressed_image* compressed_data,
                compression_settings* settings);
/**
 * @brief Загружает данные HSI из бинарного файла.
 * Функция открывает файл, пропускает заголовок, проверяет тип данных и
 * загружает пиксели в матрицу.
 * @param dat_path Путь к бинарному файлу с данными.
 * @param header Указатель на структуру заголовка, содержащую метаданные.
 * @return Указатель на матрицу пикселей (двумерный массив).
 */
int16_t** load_hsi_data(const char* dat_path, const hsi_header* header);

/**
 * @brief Читает заголовок из файла .hdr.
 * Функция открывает файл заголовка и считывает его содержимое в структуру
 * hsi_header.
 * @param hdr_path Путь к файлу заголовка.
 * @param header Указатель на структуру заголовка для заполнения.
 */
void read_hdr_file(const char* hdr_path, hsi_header* header);

/**
 * @brief Сохраняет эталоны в текстовый файл.
 * Функция записывает количество эталонов и их значения в файл.
 * @param img Указатель на структуру сжатых данных с эталонами.
 * @param header Указатель на структуру заголовка для получения информации.
 * @param filename Имя файла для сохранения (по умолчанию "standarts.txt").
 */
void save_standarts(const compressed_image* img, hsi_header* header,
                    const char* filename = "standarts.txt");
void save_compressed_image(const compressed_image* img,
                           const char* filename = "image.txt");

void free_hsi_data(int16_t** data, hsi_header* header);
void free_compressed_image(struct compressed_image* img);
void print_pixel(const int16_t* pixel, int bands);