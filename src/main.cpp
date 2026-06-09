#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "compressed_image.h"
#include "compression_settings.h"
#include "functions.h"
#include "hsi_header.h"

static void print_usage(const char* prog) {
  fprintf(stderr,
          "Usage: %s [hdr_path dat_path] [OPTIONS]\n\n"
          "  hdr_path              путь к .hdr файлу (по умолчанию /.data/hsi.hdr)\n"
          "  dat_path              путь к .gsd файлу (по умолчанию /.data/hsi.gsd)\n\n"
          "Options:\n"
          "  -m, --method   <1-4>  метод KEKM: 1=NT 2=OT 3=ST 4=AT (default: 1)\n"
          "  -s, --samples  <N>    переопределить число столбцов\n"
          "  -l, --lines    <N>    переопределить число строк\n"
          "  -b, --byte-order <N>  переопределить порядок байт\n"
          "  --me           <%%>   порог 1-го уровня, %% от нормы пикселя (default: 5.0)\n"
          "  --ae           <%%>   порог 2-го уровня, %% от нормы пикселя (default: 2.0)\n",
          prog);
}

int main(int argc, char* argv[]) {
  const char* hdr_path = "/.data/hsi.hdr";
  const char* dat_path = "/.data/hsi.gsd";

  kekm_method method = KEKM_NT;
  int override_samples = 0, override_lines = 0, override_byte_order = -1;
  double main_error = 5.0, additional_error = 2.0;

  int i = 1;
  // первые два позиционных аргумента — пути (если не начинаются с '-')
  if (i < argc && argv[i][0] != '-') { hdr_path = argv[i++]; }
  if (i < argc && argv[i][0] != '-') { dat_path = argv[i++]; }

  for (; i < argc; i++) {
    const char* flag = argv[i];
    const bool has_next = (i + 1 < argc);

    if ((strcmp(flag, "-m") == 0 || strcmp(flag, "--method") == 0) && has_next)
      method = (kekm_method)atoi(argv[++i]);
    else if ((strcmp(flag, "-s") == 0 || strcmp(flag, "--samples") == 0) && has_next)
      override_samples = atoi(argv[++i]);
    else if ((strcmp(flag, "-l") == 0 || strcmp(flag, "--lines") == 0) && has_next)
      override_lines = atoi(argv[++i]);
    else if ((strcmp(flag, "-b") == 0 || strcmp(flag, "--byte-order") == 0) && has_next)
      override_byte_order = atoi(argv[++i]);
    else if (strcmp(flag, "--me") == 0 && has_next)
      main_error = atof(argv[++i]);
    else if (strcmp(flag, "--ae") == 0 && has_next)
      additional_error = atof(argv[++i]);
    else {
      fprintf(stderr, "Неизвестный флаг: %s\n\n", flag);
      print_usage(argv[0]);
      return 1;
    }
  }
  
  hsi_header header = {};
  read_hdr_file(hdr_path, &header);

  if (override_samples)    header.samples    = override_samples;
  if (override_lines)      header.lines      = override_lines;
  if (override_byte_order >= 0) header.byte_order = override_byte_order;

  std::cout << "[debug] hdr=" << hdr_path << " dat=" << dat_path << "\n"
            << "[debug] samples=" << header.samples
            << " lines=" << header.lines
            << " bands=" << header.bands
            << " byte_order=" << header.byte_order
            << " method=" << (int)method << "\n"
            << "[debug] main_error_pct=" << main_error << "%"
            << " additional_error_pct=" << additional_error << "%\n";

  int16_t** hsi_data = load_hsi_data(dat_path, &header);
  save_rgb_image(hsi_data, &header, "preview.png");
  
  compressed_image comp_data = {NULL, 0, NULL, NULL, 0};
  compression_settings settings = {main_error, additional_error, method};
  // main_error / additional_error — проценты; адаптация к конкретному пикселю
  // выполняется внутри check_pixel() через pixel_norm()
  compression(hsi_data, &header, &comp_data, &settings);

  save_standarts_gsd(&comp_data, &header);
  save_compressed_image_gsd(&comp_data, &header);

  // Восстановление из памяти
  int16_t** restored = decompress(&comp_data, &header);
  save_rgb_image(restored, &header, "restored_preview.png");
  free_hsi_data(restored, &header);

  // Восстановление из GSD-файлов (раскомментировать для автономного использования)
  // int16_t** restored2 = decompress_from_gsd_files("standarts.gsd", "image.gsd");
  // save_rgb_image(restored2, &header, "restored_from_files.png");
  // free_hsi_data(restored2, &header);

  free_hsi_data(hsi_data, &header);
  free_compressed_image(&comp_data);
  free(header.wavelengths);

  return 0;
}