---
name: bench-compression
description: Use when benchmarking hsi_compressor on the full all.dat cube — runtime, peak memory, internal etalons, and compressed-file size per method and shift mode. Триггеры — прогнать бенчмарк по all.dat, замерить время и память компрессора, сравнить методы NT/OT/ST/AT при -S 0 и -S 1 с заданными sc.
---

# Бенчмарк сжатия (bench-compression)

## Суть

Прогоняет `hsi_compressor` на полном кубе `.data/all.dat` для каждого метода
KEKM и обоих режимов сдвига эталонов (`-S 0` и `-S 1`) с **заданным
пользователем** `sc` для каждого метода. Замеряет время работы, пиковую память,
число внутренних эталонов и вес файлов сжатия. Итог — текстовая таблица.

## Что меряется

- **Время работы** и **пиковая память** — из обёртки `/usr/bin/time -v`
  (`Elapsed (wall clock) time`, `Maximum resident set size`).
- **Внутренних эталонов** — из stdout компрессора (`Внутренних эталонов: N`).
- **Вес all.dat** — размер входного файла (константа).
- **Вес файлов сжатия** — `standarts.gsd` + `image.gsd` прогона.

## Прогоны

8 прогонов = 4 метода × shift ∈ {0, 1}, **последовательно по одному** (чистые
замеры без конкуренции за CPU/RAM). Каждый прогон с `-w 1` (нужны файлы сжатия)
и `-r 0` (RGB не нужен), пути GSD разводятся по `runs_bench/<METHOD>_S<shift>/`.
Размеры берутся из `hsi.hdr` (1032×8955×224, byte order 1) — без `-s/-l/-b`.

Пороги `me/ae` по методу по умолчанию те же, что в refining-sc:
NT/ST — `--me 0.8 --ae 0.2`, OT/AT — `--me 1.6 --ae 0.4`. Их можно
переопределить для каждого метода флагами `--me-<метод>` / `--ae-<метод>`.

## Как запустить

Из корня репозитория. `sc` задаётся для каждого метода (обязательно для тех, что
прогоняются):

```bash
# все четыре метода
python3 .claude/skills/bench-compression/bench_runs.py \
    --sc-nt 1.128 --sc-ot 1.066 --sc-st 1.0 --sc-at 1.6

# только часть методов
python3 .claude/skills/bench-compression/bench_runs.py \
    --methods OT AT --sc-ot 1.066 --sc-at 1.6

# с переопределением порогов me/ae для метода
python3 .claude/skills/bench-compression/bench_runs.py \
    --methods NT --sc-nt 1.128 --me-nt 1.0 --ae-nt 0.3
```

Перед запуском проект должен быть собран (`.build/hsi_compressor`), причём из
**текущего** исходника — бинарь использует подкоманду `compress` первым
аргументом.

## Выход

- `runs_bench/bench_results.txt` — итоговая таблица (по строке на прогон):
  `Метод | Shift | Время | Память | Внутр.эталонов | all.dat | Файлы сжатия`.
- `runs_bench/<METHOD>_S<shift>/output.txt` — сырой вывод прогона (включая отчёт
  `/usr/bin/time -v`).
- `runs_bench/<METHOD>_S<shift>/{standarts,image}.gsd` — файлы сжатия прогона.

## Замечания

- Полный куб 4.14 ГБ × 8 прогонов последовательно — долго (минуты–десятки минут
  на прогон) и ёмко по диску.
