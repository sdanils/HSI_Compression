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

# Базовые пороги (NT/ST). OT/AT берут их удвоенными — множитель ниже.
BASE_ME = "0.8"
BASE_AE = "0.2"

# method -> (method_num, множитель порогов относительно базовых)
METHODS = {
    "NT": (1, 1),
    "OT": (2, 2),
    "ST": (3, 1),
    "AT": (4, 2),
}


def fmt_pct(value):
    """Аккуратная строка из float-порога: 0.8 -> '0.8', 1.6 -> '1.6'."""
    return f"{round(value, 6):g}"

INTERNAL_RE = re.compile(r"Внутренних эталонов:\s*(\d+)")
# строка таблицы: "  1.128          229      1867" (sc может быть и "1." — seed)
ROW_RE = re.compile(r"^\s*(\d\.\d*)\s+\d+\s+(\d+)\s*$")
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


def run_one(binary, hdr, dat, samples, lines, byte_order,
            method_num, me, ae, sc, out_dir):
    """Запустить компрессор для одного sc, сохранить вывод, вернуть internal."""
    cmd = [
        str(binary), "compress", str(hdr), str(dat),
        "-s", str(samples), "-l", str(lines), "-b", str(byte_order),
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


def run_tag(args):
    """Тег прогона: <image>_<me>_<ae> по базовым (NT/ST) порогам."""
    return f"{Path(args.dat).stem}_{args.me}_{args.ae}"


def results_path(args):
    """Единый файл статистики прогона (по умолчанию по тегу)."""
    if args.results:
        return ROOT / args.results
    return ROOT / f"results_top_internal_{run_tag(args)}.txt"


# Заведомо огромное число эталонов в seed-строке: любой реальный прогон меньше.
SEED_INTERNAL = 999999999


def create_template(path, args):
    """Создать файл-шаблон статистики: по одной seed-строке (sc=1.0,
    огромное число эталонов) на каждый метод. Любой реальный прогон её побьёт,
    поэтому уточнение стартует от sc=1.0."""
    lines = [
        "Результаты компрессии HSI по методам и коэффициенту sc (шаблон)",
        f"Параметры: -s {args.samples} -l {args.lines} -b {args.byte_order}, "
        f"базовые NT/ST: --me {args.me} --ae {args.ae}, OT/AT: ×2",
        "Сортировка: по возрастанию внутренних эталонов",
        "",
    ]
    for method in METHODS:
        lines += [
            f"{method} (прогонов: 1):",
            "  sc         Главных    Внутр.",
            "  ------------------------------",
            f"  1.       {SEED_INTERNAL:>10} {SEED_INTERNAL:>10}",
            "",
        ]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Создан шаблон статистики {path}", flush=True)


def refine_method(method, args):
    method_num, mult = METHODS[method]
    # OT/AT работают на удвоенных базовых порогах (mult == 2).
    me = fmt_pct(float(args.me) * mult)
    ae = fmt_pct(float(args.ae) * mult)
    binary = ROOT / ".build" / "hsi_compressor"
    hdr = Path(args.hdr) if os.path.isabs(args.hdr) else ROOT / args.hdr
    dat = Path(args.dat) if os.path.isabs(args.dat) else ROOT / args.dat
    # Папка результатов привязана к тегу прогона (изображение + базовые me/ae),
    # чтобы прогоны разных изображений/порогов не затирали друг друга:
    #   runs/<image>_<me>_<ae>/<METHOD>/output_<sc>.txt  и  .../refined.txt
    out_dir = ROOT / "runs" / run_tag(args) / method
    out_dir.mkdir(parents=True, exist_ok=True)

    res_file = results_path(args)
    if not res_file.exists():
        print(f"[{method}] нет файла статистики {res_file} — пропуск")
        return
    scores = parse_results(res_file, method)
    if not scores:
        print(f"[{method}] секция не найдена в {res_file} — пропуск")
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
            ex.submit(run_one, binary, hdr, dat, args.samples, args.lines,
                      args.byte_order, method_num, me, ae, sc, out_dir)
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
    ap.add_argument("--results", default=None,
                    help="файл статистики (относительно корня репо); по умолчанию "
                         "results_top_internal_<image>_<me>_<ae>.txt по тегу прогона")
    ap.add_argument("--workers", type=int, default=os.cpu_count(),
                    help="число параллельных процессов компрессора")
    ap.add_argument("--hdr", default=".data/hsi.hdr",
                    help="путь к .hdr (абсолютный или относительно корня репо)")
    ap.add_argument("--dat", default=".data/hsi759_479.gsd",
                    help="путь к .gsd/.dat (абсолютный или относительно корня репо)")
    ap.add_argument("-s", "--samples", type=int, default=760,
                    help="число столбцов изображения (флаг -s компрессора)")
    ap.add_argument("-l", "--lines", type=int, default=480,
                    help="число строк изображения (флаг -l компрессора)")
    ap.add_argument("-b", "--byte-order", type=int, default=0, choices=(0, 1),
                    help="порядок байт: 0=LE, 1=BE (флаг -b компрессора)")
    ap.add_argument("-e", "--me", default=BASE_ME,
                    help=f"базовый порог 1 уровня %% (NT/ST; OT/AT берут ×2). "
                         f"По умолчанию {BASE_ME}")
    ap.add_argument("-a", "--ae", default=BASE_AE,
                    help=f"базовый порог 2 уровня %% (NT/ST; OT/AT берут ×2). "
                         f"По умолчанию {BASE_AE}")
    args = ap.parse_args()

    if not (ROOT / ".build" / "hsi_compressor").exists():
        sys.exit(f"Не найден бинарь {ROOT/'.build'/'hsi_compressor'} — собери проект")

    dat_path = Path(args.dat) if os.path.isabs(args.dat) else ROOT / args.dat
    if not dat_path.exists():
        sys.exit(f"Не найден файл изображения {dat_path}")
    hdr_path = Path(args.hdr) if os.path.isabs(args.hdr) else ROOT / args.hdr
    if not hdr_path.exists():
        sys.exit(f"Не найден заголовок {hdr_path}")

    res_file = results_path(args)
    if not res_file.exists():
        create_template(res_file, args)

    for method in args.methods:
        refine_method(method, args)


if __name__ == "__main__":
    main()
