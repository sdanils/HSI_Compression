#pragma once

#include <stdint.h>

#include "compression_settings.h"

/**
 * @brief Строит смещённую копию пикселя для нового эталона.
 *
 * Один проход по эталонам-кандидатам: для каждого соседа, чей epsilon в ε-метрике
 * метода меньше целевого (target_epsilon = threshold·step_coef), последний снимок
 * пикселя сдвигается ОТ выровненного эталона êᵢ = k_m·eᵢ + Δy в линейной шкале
 * ρ = √(n·ε) на величину (target_ρ − ρ). Выравнивание êᵢ делает направление
 * корректным для всех методов KEKM (НП — сам eᵢ, ОП — центрирование,
 * МП — масштабирование, АП — оба). Аккумулятор сдвига ведётся в double (полная
 * точность между итерациями), для сравнений берётся его округлённый int16-снимок.
 *
 * Если сдвиг выключен (settings->shift_enabled == 0), соседей нет или threshold
 * ≤ 0, out — копия пикселя.
 *
 * @param pixel     Нераспознанный пиксель p.
 * @param refs      Эталоны-кандидаты в соседи.
 * @param count     Количество эталонов в refs.
 * @param bands     Количество спектральных каналов n.
 * @param settings  Параметры сжатия (метод, shift_enabled).
 * @param threshold        Порог уровня в ε-шкале (доля от нормы пикселя).
 * @param step_coef        Коэффициент шага решётки sc для этого уровня.
 * @param anchor           Якорный эталон (например, базовый вектор главного эталона);
 *                         NULL — ограничение не применяется.
 * @param max_anchor_dist  Максимально допустимый epsilon от anchor в ε-метрике;
 *                         ≤ 0 — ограничение не применяется.
 * @param out              Буфер на bands элементов под e_new.
 */
void make_shifted_etalon(const int16_t* pixel, int16_t* const* refs,
                         int count, int bands,
                         const compression_settings* settings,
                         double threshold, double step_coef,
                         const int16_t* anchor, double max_anchor_dist,
                         int16_t* out);
