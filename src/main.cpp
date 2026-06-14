#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "app.h"
#include "kekm.h"

static void print_usage(const char* prog) {
  fprintf(
      stderr,
      "Usage: %s compress   <hdr_path> <dat_path> [OPTIONS]\n"
      "       %s decompress [hdr_path] [OPTIONS]\n\n"
      "Режимы:\n"
      "  compress    сжать изображение (пути к .hdr и .gsd обязательны)\n"
      "  decompress  восстановить .dat из GSD-файлов\n"
      "              (hdr_path нужен для byte_order, по умолчанию "
      "/.data/hsi.hdr)\n\n"
      "Options:\n"
      "  -m, --method     <1-4>  метод KEKM: 1=NT 2=OT 3=ST 4=AT (default: 1)\n"
      "  -s, --samples    <N>    переопределить число столбцов\n"
      "  -l, --lines      <N>    переопределить число строк\n"
      "  -b, --byte-order <N>    переопределить порядок байт\n"
      "  -e, --me         <%%>   порог 1-го уровня, %% от нормы пикселя "
      "(default: 5.0)\n"
      "  -a, --ae         <%%>   порог 2-го уровня, %% от нормы пикселя "
      "(default: 2.0)\n"
      "  -S, --shift      <0|1>  смещённое размещение нового эталона (default: "
      "1)\n"
      "  -c, --sc         <x>    коэффициент шага решётки sc: d = sc*delta,\n"
      "                          1 <= sc <= 2 (default: 1.0)\n"
      "  -r, --rgb        <0|1>  рисовать RGB-превью (default: 1)\n"
      "  -w, --write      <0|1>  сохранять GSD-файлы (default: 1)\n"
      "  -d, --dat-out    <file> выходной .dat файл (только decompress)\n"
      "  -p, --preview    <file> имя файла для превью оригинала (default: "
      "preview.png)\n"
      "  -P, --restored   <file> имя файла для превью восстановленного "
      "(default: restored_preview.png)\n"
      "  -o, --std        <file> файл эталонов (default: "
      "standarts.gsd)\n"
      "  -i, --img        <file> файл сжатого изображения (default: "
      "image.gsd)\n",
      prog, prog);
}

int main(int argc, char* argv[]) {
  const char* hdr_path = "/.data/hsi.hdr";
  const char* dat_path = "/.data/hsi.gsd";

  kekm_method method = KEKM_NT;
  int override_samples = 0, override_lines = 0, override_byte_order = -1;
  double main_error = 5.0, additional_error = 2.0;
  int shift_enabled = 1;
  double step_coef = 1.0;  // коэффициент шага решётки sc
  int decompress_only = 0;
  int draw_rgb = 1;
  int write_gsd = 1;
  const char* dat_out_path = "restored.dat";
  const char* preview_path = "preview.png";
  const char* restored_path = "restored_preview.png";
  const char* std_path = "standarts.gsd";
  const char* img_path = "image.gsd";

  // первый аргумент — подкоманда (режим)
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }
  const char* cmd = argv[1];
  if (strcmp(cmd, "compress") == 0) {
    decompress_only = 0;
  } else if (strcmp(cmd, "decompress") == 0) {
    decompress_only = 1;
  } else {
    fprintf(stderr, "Неизвестная команда: %s\n\n", cmd);
    print_usage(argv[0]);
    return 1;
  }

  int i = 2;
  // позиционные пути после подкоманды (если не начинаются с '-'):
  // compress   — <hdr> <dat> (оба обязательны)
  // decompress — [hdr] (для byte_order)
  bool hdr_given = false, dat_given = false;
  if (i < argc && argv[i][0] != '-') {
    hdr_path = argv[i++];
    hdr_given = true;
  }
  if (!decompress_only && i < argc && argv[i][0] != '-') {
    dat_path = argv[i++];
    dat_given = true;
  }

  for (; i < argc; i++) {
    const char* flag = argv[i];
    const bool has_next = (i + 1 < argc);

    if ((strcmp(flag, "-m") == 0 || strcmp(flag, "--method") == 0) && has_next)
      method = (kekm_method)atoi(argv[++i]);
    else if ((strcmp(flag, "-s") == 0 || strcmp(flag, "--samples") == 0) &&
             has_next)
      override_samples = atoi(argv[++i]);
    else if ((strcmp(flag, "-l") == 0 || strcmp(flag, "--lines") == 0) &&
             has_next)
      override_lines = atoi(argv[++i]);
    else if ((strcmp(flag, "-b") == 0 || strcmp(flag, "--byte-order") == 0) &&
             has_next)
      override_byte_order = atoi(argv[++i]);
    else if ((strcmp(flag, "-e") == 0 || strcmp(flag, "--me") == 0) && has_next)
      main_error = atof(argv[++i]);
    else if ((strcmp(flag, "-a") == 0 || strcmp(flag, "--ae") == 0) && has_next)
      additional_error = atof(argv[++i]);
    else if ((strcmp(flag, "-S") == 0 || strcmp(flag, "--shift") == 0) &&
             has_next)
      shift_enabled = atoi(argv[++i]);
    else if ((strcmp(flag, "-c") == 0 || strcmp(flag, "--sc") == 0) && has_next)
      step_coef = atof(argv[++i]);
    else if ((strcmp(flag, "-r") == 0 || strcmp(flag, "--rgb") == 0) &&
             has_next)
      draw_rgb = atoi(argv[++i]);
    else if ((strcmp(flag, "-w") == 0 || strcmp(flag, "--write") == 0) &&
             has_next)
      write_gsd = atoi(argv[++i]);
    else if ((strcmp(flag, "-d") == 0 || strcmp(flag, "--dat-out") == 0) &&
             has_next)
      dat_out_path = argv[++i];
    else if ((strcmp(flag, "-p") == 0 || strcmp(flag, "--preview") == 0) &&
             has_next)
      preview_path = argv[++i];
    else if ((strcmp(flag, "-P") == 0 || strcmp(flag, "--restored") == 0) &&
             has_next)
      restored_path = argv[++i];
    else if ((strcmp(flag, "-o") == 0 || strcmp(flag, "--std") == 0) &&
             has_next)
      std_path = argv[++i];
    else if ((strcmp(flag, "-i") == 0 || strcmp(flag, "--img") == 0) &&
             has_next)
      img_path = argv[++i];
    else {
      fprintf(stderr, "Неизвестный флаг: %s\n\n", flag);
      print_usage(argv[0]);
      return 1;
    }
  }

  // в режиме compress пути к .hdr и .gsd обязательны
  if (!decompress_only && (!hdr_given || !dat_given)) {
    fprintf(stderr, "Режим compress требует пути к .hdr и .gsd\n\n");
    print_usage(argv[0]);
    return 1;
  }

  cli_options opts = {hdr_path,         dat_path,
                      method,           override_samples,
                      override_lines,   override_byte_order,
                      main_error,       additional_error,
                      shift_enabled,    step_coef,
                      draw_rgb,         write_gsd,
                      dat_out_path,     preview_path,
                      restored_path,    std_path,
                      img_path};

  return decompress_only ? run_decompress(opts) : run_compress(opts);
}