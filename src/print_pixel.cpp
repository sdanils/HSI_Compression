#include <cstdint>  // Для int16_t
#include <iostream>

#include "memory.h"

void print_pixel(const int16_t* pixel, int bands) {
  for (int b = 0; b < bands; b++) {
    printf(" band[%d]=%d", b, pixel[b]);
  }
  printf("\n");
}