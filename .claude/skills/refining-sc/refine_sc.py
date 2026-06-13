#!/usr/bin/env python3
"""Пошаговое уточнение коэффициента sc по разрядам для минимизации
числа внутренних эталонов (по каждому методу KEKM независимо).

Алгоритм (точно по постановке задачи):
  1. Читаем results_top_internal.txt -> для метода берём наименьшее
     число внутренних эталонов и ВСЕ значения sc (3 знака), на которых
     оно достигается.
  2. К каждому такому sc дописываем один разряд: для "1.052" это
     10 значений 1.0520 .. 1.0529 (цифры 0..9).
  3. Запускаем компрессор для всех кандидатов параллельно, парсим
     "Внутренних эталонов: N". Каждый прогон — без RGB и без записи
     GSD (флаги -r 0 -w 0).
  4. Среди новых прогонов снова берём минимум и ВСЕ sc на нём -> это
     новые префиксы. Повторяем, пока в sc не станет 8 знаков после точки.

Каждый прогон сохраняется в runs/<METHOD>/output_<sc>.txt (тот же формат
именования, что и раньше). Итог по каждому методу — runs/<METHOD>/refined.txt.
"""
import argparse
import concurrent.futures as cf
import os
import re
import subprocess
import sys
from pathlib import Path

# ROOT/.claude/skills/refining-sc/refine_sc.py  ->  parents[3] == ROOT
ROOT = Path(__file__).resolve().parents[3]

METHODS = {
    "NT": (1, "0.8", "0.2"),
    "OT": (2, "1.6", "0.4"),
    "ST": (3, "0.8", "0.2"),
    "AT": (4, "1.6", "0.4"),
}

INTERNAL_RE = re.compile(r"Внутренних эталонов:\s*(\d+)")
# строка таблицы: "  1.128          229      1867"
ROW_RE = re.compile(r"^\s*(\d\.\d+)\s+\d+\s+(\d+)\s*$")
# заголовок секции: "NT (прогонов: 182, +S0):"
SEC_RE = re.compile(r"^(NT|OT|ST|AT)\s*\(")


def parse_results(path, method):
    """Вернуть {sc_str: internal} для секции метода из results-файла."""
    out = {}
    in_section = False
    for line in path.read_text(encoding="utf-8").splitlines():
        m = SEC_RE.match(line)
        if m:
            in_section = m.group(1) == method
            continue
        if not in_section:
            continue
        r = ROW_RE.match(line)
        if r:
            out[r.group(1)] = int(r.group(2))
    return out


def best_prefixes(scores):
    """sc-строки с минимальным числом внутренних эталонов."""
    mn = min(scores.values())
    return mn, sorted(sc for sc, v in scores.items() if v == mn)


def run_one(binary, hdr, dat, method_num, me, ae, sc, out_dir):
    """Запустить компрессор для одного sc, сохранить вывод, вернуть internal."""
    cmd = [
        str(binary), "compress", str(hdr), str(dat),
        "-s", "760", "-l", "480", "-b", "0",
        "-m", str(method_num), "--me", me, "--ae", ae,
        "-w", "0", "-r", "0", "-c", sc,
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    output = res.stdout + res.stderr
    (out_dir / f"output_{sc}.txt").write_text(output, encoding="utf-8")
    m = INTERNAL_RE.search(output)
    if not m:
        raise RuntimeError(f"sc={sc}: не найдено 'Внутренних эталонов' в выводе")
    return sc, int(m.group(1))


def children(prefix):
    """Дописать один разряд (0..9) к sc-строке."""
    return [prefix + d for d in "0123456789"]


def refine_method(method, args):
    method_num, me, ae = METHODS[method]
    binary = ROOT / ".build" / "hsi_compressor"
    hdr = ROOT / ".data" / "hsi.hdr"
    dat = ROOT / ".data" / "hsi759_479.gsd"
    out_dir = ROOT / "runs" / method
    out_dir.mkdir(parents=True, exist_ok=True)

    scores = parse_results(ROOT / args.results, method)
    if not scores:
        print(f"[{method}] секция не найдена в {args.results} — пропуск")
        return
    mn, prefixes = best_prefixes(scores)
    decimals = len(prefixes[0].split(".")[1])
    log = [f"[{method}] старт: внутр.={mn}, sc({decimals} зн.)={prefixes}"]
    print(log[-1], flush=True)

    # Один уровень уточнения за запуск: к каждому лучшему sc дописываем
    # по одному разряду (10 кандидатов на префикс) и запускаем только их.
    cands = [c for p in prefixes for c in children(p)]
    results = {}
    with cf.ThreadPoolExecutor(max_workers=args.workers) as ex:
        futs = [
            ex.submit(run_one, binary, hdr, dat, method_num, me, ae, sc, out_dir)
            for sc in cands
        ]
        for f in cf.as_completed(futs):
            sc, internal = f.result()
            results[sc] = internal
    mn, prefixes = best_prefixes(results)
    decimals += 1
    log.append(f"[{method}] уровень {decimals} зн.: внутр.={mn}, sc={prefixes}")
    print(log[-1], flush=True)

    log.append(f"[{method}] ИТОГ: внутр.={mn}, sc({decimals} зн.)={prefixes}")
    print(log[-1], flush=True)
    (out_dir / "refined.txt").write_text("\n".join(log) + "\n", encoding="utf-8")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--methods", nargs="+", default=list(METHODS),
                    choices=list(METHODS), help="какие методы уточнять")
    ap.add_argument("--results", default="results_top_internal.txt",
                    help="файл с исходными результатами (относительно корня репо)")
    ap.add_argument("--workers", type=int, default=os.cpu_count(),
                    help="число параллельных процессов компрессора")
    args = ap.parse_args()

    if not (ROOT / ".build" / "hsi_compressor").exists():
        sys.exit(f"Не найден бинарь {ROOT/'.build'/'hsi_compressor'} — собери проект")

    for method in args.methods:
        refine_method(method, args)


if __name__ == "__main__":
    main()
