#pragma once

#include "kekm.h"

/**
 * @brief Разобранные параметры командной строки.
 *
 * Заполняется в main() при разборе argv и передаётся в режимы запуска
 * (run_compress / run_decompress) — вся логика режимов вынесена из main.
 */
struct cli_options {
  const char* hdr_path;      /**< путь к .hdr */
  const char* dat_path;      /**< путь к входному .gsd */
  kekm_method method;        /**< метод KEKM (NT/OT/ST/AT) */
  int override_samples;      /**< переопределение числа столбцов (0 — нет) */
  int override_lines;        /**< переопределение числа строк (0 — нет) */
  int override_byte_order;   /**< переопределение порядка байт (-1 — нет) */
  double main_error;         /**< порог 1-го уровня, % */
  double additional_error;   /**< порог 2-го уровня, % */
  int shift_enabled;         /**< 1 — смещённое размещение нового эталона */
  double step_coef;          /**< коэффициент шага решётки sc */
  int draw_rgb;              /**< 1 — рисовать RGB-превью */
  int write_gsd;             /**< 1 — сохранять GSD-файлы */
  const char* dat_out_path;  /**< выходной .dat (режим декомпрессии) */
  const char* preview_path;  /**< имя файла превью оригинала */
  const char* restored_path; /**< имя файла превью восстановленного */
  const char* std_path;      /**< файл эталонов */
  const char* img_path;      /**< файл сжатого изображения */
};

/**
 * @brief Режим декомпрессии: восстанавливает матрицу пикселей из GSD-файлов
 * и пишет её в .dat.
 * @return Код возврата процесса (0 — успех, 1 — ошибка).
 */
int run_decompress(const cli_options& opts);

/**
 * @brief Режим сжатия: сжимает изображение, сохраняет GSD-файлы и RGB-превью.
 * @return Код возврата процесса (0 — успех, 1 — ошибка).
 */
int run_compress(const cli_options& opts);
