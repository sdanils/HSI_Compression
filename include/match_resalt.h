#pragma once

typedef struct {
  int i;       // Индекс основного эталона
  int j;       // Индекс дополнительного пикселя (-1 если нет)
  double mse;  // Минимальная MSE
} match_result;