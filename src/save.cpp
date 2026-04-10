#include <fstream>
#include <stdexcept>

#include "save.h"
#include "standart_data.h"

void save_standarts(const compressed_image* img, hsi_header* header,
                    const char* filename) {
  std::ofstream fout(filename);

  // Считаем общее число под-эталонов
  int total = 0;
  for (int i = 0; i < img->num_ref; i++) total += img->ref_counts[i];

  fout << total << ' ' << header->bands << '\n';

  // Выводим все под-эталоны в порядке плоской нумерации
  for (int i = 0; i < img->num_ref; i++) {
    for (int j = 0; j < img->ref_counts[i]; j++) {
      for (int w = 0; w < header->bands; w++) {
        fout << img->hsi_standarts[i][j][w] << ' ';
      }
      fout << '\n';
    }
  }
}

void save_compressed_image(const compressed_image* img, const hsi_header* header,
                           const char* filename) {
  std::ofstream fout(filename);
  if (!fout.is_open()) {
    throw std::runtime_error("Cannot open file for writing");
  }

  fout << header->samples << ' ' << header->lines << '\n';

  for (int idx = 0; idx < img->size; ++idx) {
    const standart_data* sd = img->image[idx];
    fout << sd->ref_index << ' '
         << sd->match.epsilon << ' '
         << sd->match.delta_y << ' '
         << sd->match.k_m << '\n';
  }
}
