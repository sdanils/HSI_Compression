#include <cstdint>  // Для int16_t
#include <cstdlib>  // Для exit, malloc, free
#include <cstring>  // Для strtok, strcmp
#include <fstream>

#include "functions.h"
#include "hsi_header.h"

// Функция для чтения заголовка .hdr
void read_hdr_file(const char* hdr_path, hsi_header* header) {
  FILE* hdr_file = fopen(hdr_path, "r");
  if (!hdr_file) {
    exit(1);
  }

  header->wavelengths = nullptr;

  char line[256];
  int wl_idx = 0;
  int in_wavelength_block = 0;

  while (fgets(line, sizeof(line), hdr_file)) {
    // Если мы внутри блока wavelength = { ... }
    if (in_wavelength_block) {
      char* p = line;
      while (*p) {
        // Пропускаем разделители
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' ||
               *p == '{' || *p == ',') {
          p++;
        }
        if (*p == '}') {
          in_wavelength_block = 0;
          break;
        }
        if (*p == '\0') break;

        char* end;
        float val = strtof(p, &end);
        if (end != p && wl_idx < header->bands) {
          header->wavelengths[wl_idx++] = val;
        }
        p = end;
      }
      continue;
    }

    char* key = strtok(line, "=");
    char* value = strtok(nullptr, "=");

    if (!key || !value) continue;

    // Trim trailing spaces from key
    int klen = (int)strlen(key);
    while (klen > 0 && (key[klen - 1] == ' ' || key[klen - 1] == '\t')) {
      key[--klen] = '\0';
    }

    value = strtok(value, " \t\n\r");

    if (strcmp(key, "samples") == 0) {
      header->samples = atoi(value);
    } else if (strcmp(key, "lines") == 0) {
      header->lines = atoi(value);
    } else if (strcmp(key, "bands") == 0) {
      header->bands = atoi(value);
    } else if (strcmp(key, "data type") == 0) {
      header->data_type = atoi(value);
    } else if (strcmp(key, "interleave") == 0) {
      strncpy(header->interleave, value, 3);
      header->interleave[3] = '\0';
    } else if (strcmp(key, "byte order") == 0) {
      header->byte_order = atoi(value);
    } else if (strcmp(key, "header offset") == 0) {
      header->header_offset = atoi(value);
    } else if (strcmp(key, "wavelength") == 0 && header->bands > 0) {
      header->wavelengths = (float*)malloc(header->bands * sizeof(float));
      wl_idx = 0;
      in_wavelength_block = 1;
      // Значения могут начинаться на той же строке после "{"
      char* p = value;
      while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' ||
               *p == '{') {
          p++;
        }
        if (*p == '}') { in_wavelength_block = 0; break; }
        if (*p == '\0') break;
        char* end;
        float val = strtof(p, &end);
        if (end != p && wl_idx < header->bands) {
          header->wavelengths[wl_idx++] = val;
        }
        p = end;
        if (*p == ',') p++;
      }
    }
  }

  fclose(hdr_file);
}
