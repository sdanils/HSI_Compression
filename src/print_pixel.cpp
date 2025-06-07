#include <cstdint>  // Для int16_t
#include <iostream>

#include "functions.h"

void print_pixel(const int16_t* pixel, int bands) {
  for (int i = 0; i < bands; i++) {
    std::cout << pixel[i] << " ";
  }
  std::cout << "\n";
}