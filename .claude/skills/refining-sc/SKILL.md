---
name: refining-sc
description: Use when refining the lattice step coefficient sc to minimize internal etalons (внутренние эталоны) in HSI compression. Триггеры — уточнить sc по разрядам, добить sc до 8 знаков, найти лучший sc по results_top_internal.txt, прогнать hsi_compressor по сетке sc для метода NT/OT/ST/AT.
---

# Уточнение коэффициента sc (refining-sc)

## Суть

Из `results_top_internal.txt` для каждого метода берётся sc с наименьшим
числом **внутренних эталонов** и уточняется по одному разряду за шаг
(дописываем цифру 0..9), пока в sc не наберётся 8 знаков после точки.
На каждом шаге запускается множество прогонов `hsi_compressor` параллельно.

## Алгоритм

1. Запустить скрипт, который найдет оптимальный sc в `results_top_internal.txt` и запустит компрессор для всех кандидатов sc **параллельно** (без RGB и без
   записи GSD — флаги `-r 0 -w 0`), распарсит `Внутренних эталонов: N` и сохранит файлы с результатами в `/runs/{method}/output_{sc}.txt`.
2. Прочитать информацию по новым запускам из файлов `/runs/{method}/output_{sc}.txt` и добавить в `results_top_internal.txt` информацию о новых прогоных. 


## Как запустить

```bash
# из корня репозитория; все четыре метода; уточняет на 1 знак
python3 .claude/skills/refining-sc/refine_sc.py
```

Скрипт сам ждёт завершения всех процессов уровня. Перед запуском проект должен быть собран
(`.build/hsi_compressor`).
