#!/usr/bin/env python3
"""Бенчмарк hsi_compressor на полном кубе all.dat.

Для каждого метода KEKM и обоих режимов сдвига (-S 0 и -S 1) запускает
компрессор с заданным пользователем sc (--sc-<method>), последовательно по
одному прогону. Каждый прогон обёрнут в `/usr/bin/time -v` — отсюда берутся
время работы (wall) и пиковая память (Max RSS). Число внутренних эталонов
парсится из stdout, вес файлов сжатия — из размеров standarts.gsd + image.gsd.

Итог — текстовая таблица runs_bench/bench_results.txt: по строке на прогон
(метод × shift) с временем, памятью, числом внутренних эталонов, весом all.dat
и весом файлов сжатия.
"""
import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

# ROOT/.claude/skills/bench-compression/bench_runs.py -> parents[3] == ROOT
ROOT = Path(__file__).resolve().parents[3]

# method -> (m_num, me, ae); пороги те же, что в refining-sc
METHODS = {
    "NT": (1, "0.8", "0.2"),
    "OT": (2, "1.6", "0.4"),
    "ST": (3, "0.8", "0.2"),
    "AT": (4, "1.6", "0.4"),
}

INTERNAL_RE = re.compile(r"Внутренних эталонов:\s*(\d+)")
ELAPSED_RE = re.compile(
    r"Elapsed \(wall clock\) time \(h:mm:ss or m:ss\):\s*([0-9:.]+)")
MAXRSS_RE = re.compile(r"Maximum resident set size \(kbytes\):\s*(\d+)")


def human_size(nbytes):
    """Человекочитаемый размер из байт."""
    v = float(nbytes)
    for unit in ("Б", "КБ", "МБ", "ГБ", "ТБ"):
        if v < 1024 or unit == "ТБ":
            return f"{int(v)} {unit}" if unit == "Б" else f"{v:.1f} {unit}"
        v /= 1024
    return f"{v:.1f} ТБ"


def elapsed_to_sec(s):
    """'h:mm:ss' или 'm:ss' (с долями) -> секунды (float)."""
    sec = 0.0
    for part in s.split(":"):
        sec = sec * 60 + float(part)
    return sec


def parse_run(method, shift, output, std, img):
    """Распарсить вывод одного прогона в dict замеров или None, если
    каких-то значений нет (внутр. эталоны / время / память)."""
    internal = INTERNAL_RE.search(output)
    elapsed = ELAPSED_RE.search(output)
    maxrss = MAXRSS_RE.search(output)
    if not internal or not elapsed or not maxrss:
        return None
    comp_bytes = (std.stat().st_size if std.exists() else 0) + (
        img.stat().st_size if img.exists() else 0
    )
    return {
        "method": method,
        "shift": shift,
        "time_str": elapsed.group(1),
        "time_sec": elapsed_to_sec(elapsed.group(1)),
        "mem_bytes": int(maxrss.group(1)) * 1024,
        "internal": int(internal.group(1)),
        "comp_bytes": comp_bytes,
    }


def run_one(method, shift, sc, me, ae):
    """Один прогон компрессора. Возвращает dict с замерами.

    Resume: если у прогона уже есть валидный output.txt и оба .gsd —
    переиспользует их без повторного (многочасового) запуска."""
    m_num = METHODS[method][0]
    binary = ROOT / ".build" / "hsi_compressor"
    hdr = ROOT / ".data" / "hsi.hdr"
    dat = ROOT / ".data" / "all.dat"
    out_dir = ROOT / "runs_bench" / f"{method}_S{shift}"
    out_dir.mkdir(parents=True, exist_ok=True)
    std = out_dir / "standarts.gsd"
    img = out_dir / "image.gsd"
    out_txt = out_dir / "output.txt"

    if out_txt.exists() and std.exists() and img.exists():
        cached = parse_run(method, shift, out_txt.read_text(encoding="utf-8"),
                           std, img)
        if cached:
            print(f"[{method} S{shift}] уже посчитан — переиспользую "
                  f"({cached['time_str']}, внутр.={cached['internal']})")
            return cached

    cmd = [
        "/usr/bin/time", "-v",
        str(binary), "compress", str(hdr), str(dat),
        "-m", str(m_num), "--me", me, "--ae", ae,
        "-S", str(shift), "-c", sc, "-w", "1", "-r", "0",
        "-o", str(std), "-i", str(img),
    ]
    print(f"[{method} S{shift}] sc={sc} me={me} ae={ae} ... ",
          end="", flush=True)
    res = subprocess.run(cmd, capture_output=True, text=True)
    output = res.stdout + res.stderr
    out_txt.write_text(output, encoding="utf-8")

    row = parse_run(method, shift, output, std, img)
    if res.returncode != 0 or row is None:
        raise RuntimeError(
            f"[{method} S{shift}] прогон не удался (код {res.returncode}); "
            f"см. {out_txt}"
        )
    print(f"{row['time_str']} ({row['time_sec']:.1f} c), "
          f"внутр.={row['internal']}")
    return row


def write_table(rows, dat_bytes, path):
    """Записать выровненную текстовую таблицу результатов."""
    headers = ["Метод", "Shift", "Время", "Память",
               "Внутр.эталонов", "all.dat", "Файлы сжатия"]
    table = []
    for r in rows:
        table.append([
            r["method"],
            str(r["shift"]),
            f"{r['time_str']} ({r['time_sec']:.1f}c)",
            human_size(r["mem_bytes"]),
            str(r["internal"]),
            human_size(dat_bytes),
            human_size(r["comp_bytes"]),
        ])
    widths = [max(len(h), *(len(row[i]) for row in table)) if table else len(h)
              for i, h in enumerate(headers)]
    fmt = "  ".join(f"{{:<{w}}}" for w in widths)
    lines = [fmt.format(*headers), fmt.format(*("-" * w for w in widths))]
    lines += [fmt.format(*row) for row in table]
    text = "\n".join(lines) + "\n"
    path.write_text(text, encoding="utf-8")
    print("\n" + text)
    print(f"Таблица сохранена в {path}")


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("--methods", nargs="+", default=list(METHODS),
                    choices=list(METHODS), help="какие методы прогонять")
    for m in METHODS:
        ap.add_argument(f"--sc-{m.lower()}", metavar="X",
                        help=f"коэффициент sc для метода {m}")
        ap.add_argument(f"--me-{m.lower()}", metavar="P",
                        help=f"порог me для метода {m} "
                             f"(по умолчанию {METHODS[m][1]})")
        ap.add_argument(f"--ae-{m.lower()}", metavar="P",
                        help=f"порог ae для метода {m} "
                             f"(по умолчанию {METHODS[m][2]})")
    args = ap.parse_args()

    binary = ROOT / ".build" / "hsi_compressor"
    if not binary.exists():
        sys.exit(f"Не найден бинарь {binary} — собери проект")
    dat = ROOT / ".data" / "all.dat"
    if not dat.exists():
        sys.exit(f"Не найден входной файл {dat}")

    sc_map = {m: getattr(args, f"sc_{m.lower()}") for m in METHODS}
    me_map = {m: getattr(args, f"me_{m.lower()}") or METHODS[m][1]
              for m in METHODS}
    ae_map = {m: getattr(args, f"ae_{m.lower()}") or METHODS[m][2]
              for m in METHODS}
    missing = [m for m in args.methods if not sc_map[m]]
    if missing:
        sys.exit("не задан sc для методов: "
                 + ", ".join(f"{m} (--sc-{m.lower()})" for m in missing))

    dat_bytes = dat.stat().st_size
    rows = []
    for method in args.methods:
        for shift in (0, 1):
            rows.append(run_one(method, shift, sc_map[method],
                                me_map[method], ae_map[method]))

    out_dir = ROOT / "runs_bench"
    out_dir.mkdir(parents=True, exist_ok=True)
    write_table(rows, dat_bytes, out_dir / "bench_results.txt")


if __name__ == "__main__":
    main()
