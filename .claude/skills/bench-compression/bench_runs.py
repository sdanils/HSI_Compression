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
ELAPSED_RE = re.compile(r"Elapsed \(wall clock\) time[^:]*:\s*([0-9:.]+)")
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


def run_one(method, shift, sc):
    """Один прогон компрессора. Возвращает dict с замерами."""
    m_num, me, ae = METHODS[method]
    binary = ROOT / ".build" / "hsi_compressor"
    hdr = ROOT / ".data" / "hsi.hdr"
    dat = ROOT / ".data" / "all.dat"
    out_dir = ROOT / "runs_bench" / f"{method}_S{shift}"
    out_dir.mkdir(parents=True, exist_ok=True)
    std = out_dir / "standarts.gsd"
    img = out_dir / "image.gsd"

    cmd = [
        "/usr/bin/time", "-v",
        str(binary), "compress", str(hdr), str(dat),
        "-m", str(m_num), "--me", me, "--ae", ae,
        "-S", str(shift), "-c", sc, "-w", "1", "-r", "0",
        "-o", str(std), "-i", str(img),
    ]
    print(f"[{method} S{shift}] sc={sc} ... ", end="", flush=True)
    res = subprocess.run(cmd, capture_output=True, text=True)
    output = res.stdout + res.stderr
    (out_dir / "output.txt").write_text(output, encoding="utf-8")

    internal = INTERNAL_RE.search(output)
    elapsed = ELAPSED_RE.search(output)
    maxrss = MAXRSS_RE.search(output)
    if res.returncode != 0 or not internal or not elapsed or not maxrss:
        raise RuntimeError(
            f"[{method} S{shift}] прогон не удался (код {res.returncode}); "
            f"см. {out_dir/'output.txt'}"
        )

    comp_bytes = (std.stat().st_size if std.exists() else 0) + (
        img.stat().st_size if img.exists() else 0
    )
    sec = elapsed_to_sec(elapsed.group(1))
    print(f"{elapsed.group(1)} ({sec:.1f} c), внутр.={internal.group(1)}")
    return {
        "method": method,
        "shift": shift,
        "time_str": elapsed.group(1),
        "time_sec": sec,
        "mem_bytes": int(maxrss.group(1)) * 1024,
        "internal": int(internal.group(1)),
        "comp_bytes": comp_bytes,
    }


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
    args = ap.parse_args()

    binary = ROOT / ".build" / "hsi_compressor"
    if not binary.exists():
        sys.exit(f"Не найден бинарь {binary} — собери проект")
    dat = ROOT / ".data" / "all.dat"
    if not dat.exists():
        sys.exit(f"Не найден входной файл {dat}")

    sc_map = {m: getattr(args, f"sc_{m.lower()}") for m in METHODS}
    missing = [m for m in args.methods if not sc_map[m]]
    if missing:
        sys.exit("не задан sc для методов: "
                 + ", ".join(f"{m} (--sc-{m.lower()})" for m in missing))

    dat_bytes = dat.stat().st_size
    rows = []
    for method in args.methods:
        for shift in (0, 1):
            rows.append(run_one(method, shift, sc_map[method]))

    out_dir = ROOT / "runs_bench"
    out_dir.mkdir(parents=True, exist_ok=True)
    write_table(rows, dat_bytes, out_dir / "bench_results.txt")


if __name__ == "__main__":
    main()
