#include <iostream>
#include <string>

#include "functions.h"
#include "struct_hsi.h"

using namespace std;

int main() {
  const char* hdr_path = "../data/hsi.hdr";
  const char* dat_path = "../data/hsi.gsd";

  HSI_Header header;
  read_hdr_file(hdr_path, &header);

  int16_t** hsi_data = load_hsi_data(dat_path, &header);

  int x = 42, y = 0;
  int pixel_index = y * header.samples + x;
  std::cout << "Пиксель (" << x << "," << y << "): ";
  for (int b = 0; b < header.bands; b++) {
    std::cout << hsi_data[pixel_index][b] << " ";
  }
  std::cout << std::endl;

  int total_pixels = header.lines * header.samples;
  free_hsi_data(hsi_data, total_pixels);
  return 0;
}