#include <cstdint>  // Для int16_t
#include <cstdlib>  // Для exit, malloc, free
#include <cstring>  // Для strtok, strcmp
#include <fstream>
#include <iostream>

#include "functions.h"
#include "struct_hsi.h"

// Функция для чтения заголовка .hdr
void read_hdr_file(const char* hdr_path, HSI_Header* header) {
  FILE* hdr_file = fopen(hdr_path, "r");
  if (!hdr_file) {
    std::cerr << "Ошибка: Не удалось открыть файл " << hdr_path << std::endl;
    exit(1);
  }

  char line[256];
  while (fgets(line, sizeof(line), hdr_file)) {
    char* key = strtok(line, " =");
    char* value = strtok(nullptr, " =");

    if (!key || !value) continue;

    // Удаление лишних пробелов и символов
    value = strtok(value, " \t\n\r");

    if (strcmp(key, "samples") == 0) {
      header->samples = atoi(value);
    } else if (strcmp(key, "lines") == 0) {
      header->lines = atoi(value);
    } else if (strcmp(key, "bands") == 0) {
      header->bands = atoi(value);
    } else if (strcmp(key, "data") == 0 &&
               strcmp(strtok(nullptr, " ="), "type") == 0) {
      header->data_type = atoi(value);
    } else if (strcmp(key, "interleave") == 0) {
      strncpy(header->interleave, value, 3);
      header->interleave[3] = '\0';
    } else if (strcmp(key, "byte") == 0 &&
               strcmp(strtok(nullptr, " ="), "order") == 0) {
      header->byte_order = atoi(value);
    } else if (strcmp(key, "header") == 0 &&
               strcmp(strtok(nullptr, " ="), "offset") == 0) {
      header->header_offset = atoi(value);
    }
  }

  fclose(hdr_file);
}