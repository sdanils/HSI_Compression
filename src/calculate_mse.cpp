#include <cmath>

#include "functions.h"

double calculate_mse(const int16_t* pixel1, const int16_t* pixel2, int bands) {
  double sum = 0.0;
  for (int b = 0; b < bands; b++) {
    double diff = pixel1[b] - pixel2[b];
    sum += diff * diff;
  }
  return sqrt(sum / bands);
}
