#!/usr/bin/env python3
"""Channon (2019) scaling analysis of maximum individual complexity in Geb.

Maximum individual complexity per snapshot is Geb's own `max_used_component_count`
(the number of genes involved in an agent's neural development, maximised over
agents). For each run it is averaged over a time window (default 0.5M-1M steps), then
averaged over runs per (world length L, max neurons N) setting.

Fits:
  - joint scaling (N = 2L, scale = L/10): complexity = a + b log2(scale); compared with
    Channon 2019: 10.167525 + 1.866825 log2(scale);
  - all settings: complexity = a + b softmin(ln L, ln N), softmin(x, y) = -ln(e^-x + e^-y)
    (Channon 2019, which reports the softmin model fitting single-parameter scaling).

Usage: analyse.py OUT_DIR RUN_DIR [RUN_DIR ...] [--window 500000 1000000]
Run directories are matched by name: scale_L<L>_N<N>_s<seed>; any other directory is
treated as (L=20, N=20) unless its log says otherwise.
"""

import argparse
import json
import math
import os
import re
import sys

import numpy as np

CHANNON_A, CHANNON_B = 10.167525, 1.866825
LINE = re.compile(r"Complexities of used components at time=(\d+): max_used_component_count=(\d+)")


def run_setting(run_dir):
    m = re.search(r"L(\d+)_N(\d+)", os.path.basename(run_dir.rstrip("/")))
    if m:
        return int(m.group(1)), int(m.group(2))
    return 20, 20


def run_complexity(run_dir, lo, hi):
    vals = []
    with open(os.path.join(run_dir, "log.txt")) as f:
        for line in f:
            mm = LINE.match(line)
            if mm:
                t = int(mm.group(1))
                if lo <= t <= hi:
                    vals.append(int(mm.group(2)))
    return (float(np.mean(vals)), len(vals)) if vals else (None, 0)


def ci95(x):
    x = np.asarray(x, float)
    if len(x) < 2:
        return float("nan")
    from scipy import stats
    return float(stats.t.ppf(0.975, len(x) - 1) * x.std(ddof=1) / math.sqrt(len(x)))


def linfit(x, y):
    x = np.asarray(x, float)
    y = np.asarray(y, float)
    A = np.vstack([np.ones_like(x), x]).T
    coef, res, _, _ = np.linalg.lstsq(A, y, rcond=None)
    yhat = A @ coef
    ss_res = float(((y - yhat) ** 2).sum())
    ss_tot = float(((y - y.mean()) ** 2).sum())
    n = len(x)
    se_b = math.sqrt(ss_res / (n - 2) / ((x - x.mean()) ** 2).sum()) if n > 2 else float("nan")
    return dict(a=float(coef[0]), b=float(coef[1]), r2=1 - ss_res / ss_tot if ss_tot else float("nan"),
                se_b=se_b, n=n)


def softmin(x, y):
    return -np.log(np.exp(-x) + np.exp(-y))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("runs", nargs="+")
    ap.add_argument("--window", nargs=2, type=int, default=[500000, 1000000])
    a = ap.parse_args()
    lo, hi = a.window
    per_run = []
    for d in a.runs:
        if not os.path.exists(os.path.join(d, "log.txt")):
            continue
        c, n = run_complexity(d, lo, hi)
        if c is None or n < (hi - lo) // 1000 * 0.9:
            continue  # window not (yet) covered
        L, N = run_setting(d)
        per_run.append(dict(run=os.path.basename(d.rstrip("/")), L=L, N=N, complexity=c))
    settings = {}
    for r in per_run:
        settings.setdefault((r["L"], r["N"]), []).append(r["complexity"])
    table = []
    for (L, N), v in sorted(settings.items()):
        table.append(dict(L=L, N=N, n=len(v), mean=float(np.mean(v)), ci95=ci95(v)))

    # Joint scaling: N = 2L, scale = L/10 (per-run points, as in Channon 2019 Fig. 4).
    diag = [r for r in per_run if r["N"] == 2 * r["L"]]
    joint = linfit([math.log2(r["L"] / 10) for r in diag], [r["complexity"] for r in diag]) if len(
        {r["L"] for r in diag}) >= 2 else None
    # softmin model over all runs.
    sm = linfit([float(softmin(math.log(r["L"]), math.log(r["N"]))) for r in per_run],
                [r["complexity"] for r in per_run]) if len(settings) >= 3 else None

    os.makedirs(a.out, exist_ok=True)
    res = dict(window=[lo, hi], settings=table, joint_fit=joint, softmin_fit=sm,
               channon2019_joint=dict(a=CHANNON_A, b=CHANNON_B), runs=per_run)
    with open(os.path.join(a.out, "scaling.json"), "w") as f:
        json.dump(res, f, indent=2)
    lines = ["| L | N | runs | max individual complexity (mean) | 95% CI | Channon 2019 joint fit |",
             "|---|---|---|---|---|---|"]
    for t in table:
        pred = f"{CHANNON_A + CHANNON_B * math.log2(t['L'] / 10):.2f}" if t["N"] == 2 * t["L"] else ""
        lines.append(f"| {t['L']} | {t['N']} | {t['n']} | {t['mean']:.2f} | ±{t['ci95']:.2f} | {pred} |")
    if joint:
        lines.append(f"\nJoint scaling fit (N = 2L): complexity = {joint['a']:.2f} + {joint['b']:.2f} "
                     f"log2(L/10), slope SE {joint['se_b']:.2f}, R² {joint['r2']:.3f}, n {joint['n']} runs. "
                     f"Channon 2019: {CHANNON_A:.2f} + {CHANNON_B:.2f} log2(scale).")
    if sm:
        lines.append(f"softmin model (all runs): complexity = {sm['a']:.2f} + {sm['b']:.2f} "
                     f"softmin(ln L, ln N), R² {sm['r2']:.3f}, n {sm['n']} runs.")
    with open(os.path.join(a.out, "scaling.md"), "w") as f:
        f.write("\n".join(lines) + "\n")
    print("\n".join(lines))

    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        return
    fig, axes = plt.subplots(1, 3, figsize=(13, 4))
    groups = [("joint (N = 2L)", lambda t: t["N"] == 2 * t["L"], "L"),
              ("neurons only (L = 10)", lambda t: t["L"] == 10, "N"),
              ("world only (N = 20)", lambda t: t["N"] == 20, "L")]
    for ax, (title, sel, key) in zip(axes, groups):
        pts = sorted([t for t in table if sel(t)], key=lambda t: t[key])
        if not pts:
            continue
        xs = [t[key] for t in pts]
        ax.errorbar(xs, [t["mean"] for t in pts], yerr=[t["ci95"] for t in pts], marker="o", capsize=3,
                    label="this port")
        if key == "L" and title.startswith("joint"):
            xx = np.array(xs, float)
            ax.plot(xx, CHANNON_A + CHANNON_B * np.log2(xx / 10), "--", label="Channon 2019 fit")
        ax.set_xscale("log", base=2)
        ax.set_xlabel(key)
        ax.set_title(title, fontsize=9)
        ax.set_ylabel("max individual complexity")
        ax.legend(fontsize=7)
    fig.tight_layout()
    fig.savefig(os.path.join(a.out, "scaling.png"), dpi=120)


if __name__ == "__main__":
    main()
