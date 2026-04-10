#pragma once

#include "kekm.h"

/**
 * @brief Параметры сжатия.
 */
struct compression_settings {
  int main_error;       /**< Максимально допустимая ошибка для основного эталона */
  int additional_error; /**< Максимально допустимая ошибка для дополнительного эталона */
  kekm_method method;   /**< Метод сравнения пикселей (KEKM_NT/OT/ST/AT) */
};
