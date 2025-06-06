#pragma once

// Структура для хранения параметров из .hdr файла
struct HSI_Header {
  int samples;         // ширина (количество столбцов)
  int lines;           // высота (количество строк)
  int bands;           // количество спектральных каналов
  int data_type;       // тип данных (2 = int16)
  char interleave[4];  // порядок хранения ("bip", "bil", "bsq")
  int byte_order;      // порядок байтов (0 = little-endian, 1 = big-endian)
  int header_offset;   // смещение данных в файле
};
