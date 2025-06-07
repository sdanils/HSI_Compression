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

  char line[256];
  while (fgets(line, sizeof(line), hdr_file)) {
    char* key = strtok(line, "=");
    char* value = strtok(nullptr, "=");

    if (!key || !value) continue;

    value = strtok(value, " \t\n\r");

    if (strcmp(key, "samples ") == 0) {
      header->samples = atoi(value);
    } else if (strcmp(key, "lines ") == 0) {
      header->lines = atoi(value);
    } else if (strcmp(key, "bands ") == 0) {
      header->bands = atoi(value);
    } else if (strcmp(key, "data type ") == 0) {
      header->data_type = atoi(value);
    } else if (strcmp(key, "interleave ") == 0) {
      strncpy(header->interleave, value, 3);
      header->interleave[3] = '\0';
    } else if (strcmp(key, "byte order ") == 0) {
      header->byte_order = atoi(value);
    } else if (strcmp(key, "header offset ") == 0) {
      header->header_offset = atoi(value);
    }
  }

  fclose(hdr_file);
}