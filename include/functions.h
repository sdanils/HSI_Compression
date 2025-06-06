#pragma once

#include <cstdint>  // Для int16_t

#include "struct_hsi.h"

int16_t** load_hsi_data(const char* dat_path, const HSI_Header* header);
void read_hdr_file(const char* hdr_path, HSI_Header* header);
void free_hsi_data(int16_t** data, int lines);