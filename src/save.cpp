#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include "compressed_image.h"
#include "hsi_header.h"

void save_standarts(const compressed_image* img, hsi_header* header,
                    const char* filename = "standarts.txt") {
  std::ofstream fout(filename);
  fout << img->num_ref << ' ' << header->bands << '\n';

  for (int ref = 0; ref < img->num_ref; ++ref) {
    int height = img->ref_counts[ref];
    fout << height << '\n';

    for (int h = 0; h < height; ++h) {
      for (int w = 0; w < header->bands; ++w) {
        fout << img->hsi_standarts[ref][h][w] << ' ';
      }
      fout << '\n';
    }
  }
}

void save_compressed_image(const compressed_image* img,
                           const char* filename = "image.txt") {
  std::ofstream fout(filename);
  if (!fout.is_open()) {
    throw std::runtime_error("Cannot open file for writing");
  }

  fout << img->size << '\n';

  for (int idx = 0; idx < img->size; ++idx) {
    const match_result* mr = img->image[idx];
    fout << mr->main << ' ' << mr->additional << ' ' << mr->mse << '\n';
  }
}