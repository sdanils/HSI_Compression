#pragma once

/**
 * @brief Заголовок HSI-файла, описывающий параметры гиперспектрального
 * изображения.
 */
struct hsi_header {
  int samples;        /**< Ширина изображения (количество столбцов) */
  int lines;          /**< Высота изображения (количество строк) */
  int bands;          /**< Количество спектральных каналов */
  int data_type;      /**< Тип данных (например, 2 = int16) */
  char interleave[4]; /**< Порядок хранения данных ("bip", "bil", "bsq") */
  int byte_order;     /**< Порядок байтов (0 = little-endian, 1 = big-endian) */
  int header_offset;  /**< Смещение данных в файле (в байтах) */
};