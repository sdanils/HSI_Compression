#include <cstdio>
#include <cstdlib>

#include "compressed_image.h"
#include "compression_settings.h"
#include "functions.h"
#include "hsi_header.h"

int main(int argc, char* argv[]) {
  const char* hdr_path = "/.data/hsi.hdr";
  const char* dat_path = "/.data/hsi.gsd";

  kekm_method method = KEKM_NT;

  if (argc == 3 || argc == 4) {
    hdr_path = argv[1];
    dat_path = argv[2];
    if (argc == 4) method = (kekm_method)atoi(argv[3]);
  } else if (argc == 6 || argc == 7) {
    hdr_path = argv[1];
    dat_path = argv[2];
    if (argc == 7) method = (kekm_method)atoi(argv[6]);
  } else if (argc != 1) {
    fprintf(stderr,
            "Usage: %s [hdr dat [method]] | [hdr dat samples lines byte_order "
            "[method]]\n"
            "  method: 1=NT 2=OT 3=ST 4=AT (default: 1)\n",
            argv[0]);
    return 1;
  }

  hsi_header header = {};
  read_hdr_file(hdr_path, &header);

  if (argc == 6) {
    header.samples    = atoi(argv[3]);
    header.lines      = atoi(argv[4]);
    header.byte_order = atoi(argv[5]);
  }

  int16_t** hsi_data = load_hsi_data(dat_path, &header);
  compressed_image comp_data = {NULL, 0, NULL, NULL, 0};

  compression_settings settings = {5000, 2000, method};
  compression(hsi_data, &header, &comp_data, &settings);

  save_standarts(&comp_data, &header);
  save_compressed_image(&comp_data, &header);

  // Восстановление из памяти
  int16_t** restored = decompress(&comp_data, &header);
  save_rgb_image(restored, &header, "restored_preview.png");
  free_hsi_data(restored, &header);

  // Восстановление из файлов (раскомментировать для автономного использования)
  // int16_t** restored2 = decompress_from_files("standarts.txt", "image.txt");
  // save_rgb_image(restored2, &header, "restored_from_files.png");
  // free_hsi_data(restored2, &header);

  free_hsi_data(hsi_data, &header);
  free_compressed_image(&comp_data);
  free(header.wavelengths);

  return 0;
}