#include <string>

#include "compressed_image.h"
#include "compression_settings.h"
#include "functions.h"
#include "hsi_header.h"

using namespace std;

int main() {
  const char* hdr_path = "../data/hsi.hdr";
  const char* dat_path = "../data/hsi.gsd";

  hsi_header header;
  read_hdr_file(hdr_path, &header);

  int16_t** hsi_data = load_hsi_data(dat_path, &header);
  compressed_image comp_data = {NULL, 0, NULL, NULL, 0};

  compression_settings settings = {26000, 21000};
  compression(hsi_data, &header, &comp_data, &settings);

  save_standarts(&comp_data, &header);
  save_compressed_image(&comp_data);

  free_hsi_data(hsi_data, &header);
  free_compressed_image(&comp_data);

  return 0;
}