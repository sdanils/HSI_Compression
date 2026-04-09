#include <cstdio>
#include <cstdlib>

#include "compressed_image.h"
#include "compression_settings.h"
#include "functions.h"
#include "hsi_header.h"

int main(int argc, char* argv[]) {
  const char* hdr_path = "/.data/hsi.hdr";
  const char* dat_path = "/.data/hsi.gsd";

  if (argc == 3) {
    hdr_path = argv[1];
    dat_path = argv[2];
  } else if (argc == 6) {
    hdr_path = argv[1];
    dat_path = argv[2];
  } else if (argc != 1) {
    fprintf(stderr, "Usage: %s [hdr_path dat_path [samples lines byte_order]]\n", argv[0]);
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
  // compressed_image comp_data = {NULL, 0, NULL, NULL, 0};

  // compression_settings settings = {26000, 21000};
  // compression(hsi_data, &header, &comp_data, &settings);

  // save_standarts(&comp_data, &header);
  // save_compressed_image(&comp_data);
  save_rgb_image(hsi_data, &header);

  free_hsi_data(hsi_data, &header);
  // free_compressed_image(&comp_data);
  free(header.wavelengths);

  return 0;
}