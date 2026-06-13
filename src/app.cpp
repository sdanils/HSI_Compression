#include "app.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#include "compressed_image.h"
#include "compression_settings.h"
#include "functions.h"
#include "hsi_header.h"

namespace {
// Аналог finally: лямбда из деструктора вызывается при любом выходе из области
// видимости — обычном return или исключении (RAII). Освобождение ресурсов
// пишется один раз и гарантированно выполняется на всех путях.
template <typename F>
struct scope_guard {
  F fn;
  ~scope_guard() { fn(); }
};
template <typename F>
scope_guard<F> make_guard(F fn) {
  return scope_guard<F>{fn};
}
}  // namespace

int run_decompress(const cli_options& opts) {
  // Режим декомпрессии: читаем эталоны + сжатое изображение из GSD-файлов,
  // восстанавливаем матрицу пикселей и пишем её в .dat (формат load_hsi_data).
  hsi_header dh = {};
  read_hdr_file(opts.hdr_path, &dh);  // нужен byte_order (в GSD не хранится)
  auto cleanup = make_guard([&] { free(dh.wavelengths); });

  if (opts.override_byte_order >= 0) dh.byte_order = opts.override_byte_order;

  try {
    // samples/lines/bands берутся из GSD-файлов и перезаписывают значения hdr
    int16_t** restored =
        decompress_from_gsd_files(opts.std_path, opts.img_path, &dh);
    save_hsi_data(restored, &dh, opts.dat_out_path);

    printf("[decompress] %s + %s -> %s\n"
           "[decompress] samples=%d lines=%d bands=%d byte_order=%d\n",
           opts.std_path, opts.img_path, opts.dat_out_path, dh.samples,
           dh.lines, dh.bands, dh.byte_order);

    free_hsi_data(restored, &dh);
    return 0;
  } catch (const std::exception& e) {
    fprintf(stderr, "Ошибка: %s\n", e.what());
    return 1;
  }
}

int run_compress(const cli_options& opts) {
  hsi_header header = {};
  read_hdr_file(opts.hdr_path, &header);

  if (opts.override_samples) header.samples = opts.override_samples;
  if (opts.override_lines) header.lines = opts.override_lines;
  if (opts.override_byte_order >= 0) header.byte_order = opts.override_byte_order;

  printf("[debug] hdr=%s dat=%s\n"
         "[debug] samples=%d lines=%d bands=%d byte_order=%d method=%d\n"
         "[debug] main_error_pct=%g%% additional_error_pct=%g%% shift=%d "
         "step_coef=%g rgb=%d write_gsd=%d\n",
         opts.hdr_path, opts.dat_path, header.samples, header.lines,
         header.bands, header.byte_order, (int)opts.method, opts.main_error,
         opts.additional_error, opts.shift_enabled, opts.step_coef,
         opts.draw_rgb, opts.write_gsd);

  int16_t** hsi_data = load_hsi_data(opts.dat_path, &header);
  compressed_image comp_data = {NULL, 0, NULL, NULL, 0};

  auto cleanup = make_guard([&] {
    free_hsi_data(hsi_data, &header);
    free_compressed_image(&comp_data);
    free(header.wavelengths);
  });

  try {
    compression_settings settings = {opts.main_error, opts.additional_error,
                                     opts.method, opts.shift_enabled,
                                     opts.step_coef};
    compression(hsi_data, &header, &comp_data, &settings);

    int total_standards = 0;
    for (int k = 0; k < comp_data.num_ref; k++)
      total_standards += comp_data.ref_counts[k];
    printf("Главных эталонов: %d  Внутренних эталонов: %d", comp_data.num_ref,
           total_standards);
    if (settings.shift_enabled) printf("  (sc: %g)", settings.step_coef);
    printf("\n");

    if (opts.write_gsd) {
      save_standarts_gsd(&comp_data, &header, opts.std_path);
      save_compressed_image_gsd(&comp_data, &header, opts.img_path);
    }

    if (opts.draw_rgb) {
      save_rgb_image(hsi_data, &header, opts.preview_path);

      int16_t** restored = decompress(&comp_data, &header);
      save_rgb_image(restored, &header, opts.restored_path, hsi_data);
      free_hsi_data(restored, &header);
    }

    return 0;
  } catch (const std::exception& e) {
    fprintf(stderr, "Ошибка: %s\n", e.what());
    return 1;
  }
}
