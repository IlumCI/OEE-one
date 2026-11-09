#!/usr/bin/env python3
"""Null-calibrated real-versus-null test for component-normalised activity.

Replaces the fixed per-run trend rule (RESULTS.md, Table I rule) with tests whose
false-positive rate is set by the null controls themselves.

Statistic: Theil-Sen slope of the normalised median activity (predecessor + link-bit
components) over the second half of a horizon [H/2, H], per 1e6 steps.

1. Per-run test, calibrated on nulls. The null distribution is the pooled set of
   slopes of every null-control trajectory over every window of length H/2 (start
   positions every STRIDE steps) within its available length. When a null run is itself
   tested, its own windows are left out (leave-one-out). p = (#null windows with
   slope >= s + 1) / (N + 1). Windows overlap, so p is approximate.
2. Group test: paired sign-flip permutation test over seeds on d = slope_real -
   slope_null (exact; 2^n sign patterns), plus Hedges' g of the paired differences.

Usage: calibrated.py OUT_DIR RUN [RUN ...] [--horizons 2000000 4000000 8000000]
Runs must carry a second-shadow null (presence file with S2). Stagnated runs are
truncated at their last birth and take part only in horizons they reach.
"""
import argparse
import json
import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import geb_activity as ga  # noqa: E402

STRIDE = 250000


def series(run):
    stag = ga.stagnation_time(run)
    out = {}
    for null in (False, True):
        _, s = ga.analyse(run, "resets", "pb", 20, "presence", null, stag)
        out["null" if null else "real"] = (s["time"].astype(float), np.asarray(s["N_Amedian"], float))
    out["end"] = float(out["real"][0][-1])
    return out


def slope(t, y, lo, hi):
    m = (t >= lo) & (t <= hi)
    if m.sum() < 20:
        return None
    return ga.theil_sen(t[m], y[m]) * 1e6


def null_windows(series_by_run, length, exclude=None):
    vals = []
    for name, s in series_by_run.items():
        if name == exclude:
            continue
        t, y = s["null"]
        start = 0.0
        while start + length <= t[-1]:
            v = slope(t, y, start, start + length)
            if v is not None:
                vals.append(v)
            start += STRIDE
    return np.array(vals)


def sign_flip_p(d):
    d = np.asarray(d, float)
    n = len(d)
    obs = d.mean()
    cnt = 0
    for mask in range(2 ** n):
        signs = np.array([1 if mask >> i & 1 else -1 for i in range(n)])
        cnt += (signs * d).mean() >= obs - 1e-12
    return cnt / 2 ** n


def hedges_g(d):
    d = np.asarray(d, float)
    n = len(d)
    if n < 2 or d.std(ddof=1) == 0:
        return float("nan")
    return float(d.mean() / d.std(ddof=1) * (1 - 3 / (4 * (n - 1) - 1)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("runs", nargs="+")
    ap.add_argument("--horizons", type=float, nargs="+", default=[2e6, 4e6, 8e6])
    a = ap.parse_args()
    S = {os.path.basename(r.rstrip("/")): series(r) for r in a.runs}
    result = dict(horizons=[], stride=STRIDE)
    lines = []
    for H in a.horizons:
        L = H / 2
        rows, d = [], []
        for name, s in S.items():
            if s["end"] < H * 0.995:
                continue
            sr = slope(*s["real"], H - L, H)
            sn = slope(*s["null"], H - L, H)
            ref_real = null_windows(S, L)                  # all nulls
            ref_null = null_windows(S, L, exclude=name)    # leave this run's null out
            p_real = (np.sum(ref_real >= sr) + 1) / (ref_real.size + 1)
            p_null = (np.sum(ref_null >= sn) + 1) / (ref_null.size + 1)
            rows.append(dict(run=name, slope_real=sr, slope_null=sn, p_real=float(p_real), p_null=float(p_null),
                             null_windows=int(ref_real.size), null_max=float(ref_real.max())))
            d.append(sr - sn)
        if not rows:
            continue
        g = dict(horizon=H, runs=rows, n=len(d), mean_diff=float(np.mean(d)),
                 p_group=sign_flip_p(d) if len(d) >= 2 else float("nan"), hedges_g=hedges_g(d))
        result["horizons"].append(g)
        lines.append(f"\n### Horizon {H / 1e6:g}M (slope over {L / 1e6:g}M-{H / 1e6:g}M, per 1e6 steps)\n")
        lines.append("| run | real slope | p (vs null windows) | null slope | p (leave-one-out) |")
        lines.append("|---|---|---|---|---|")
        for r in rows:
            lines.append(f"| {r['run']} | {r['slope_real']:.1f} | {r['p_real']:.3f} | {r['slope_null']:.1f} | "
                         f"{r['p_null']:.3f} |")
        lines.append(f"\nGroup (n = {g['n']}): mean paired difference {g['mean_diff']:.1f} per 1e6 steps, "
                     f"sign-flip p = {g['p_group']:.3f} (minimum attainable {1 / 2 ** g['n']:.3f}), "
                     f"Hedges' g = {g['hedges_g']:.2f}. Null reference: {rows[0]['null_windows']} windows, "
                     f"max null slope {rows[0]['null_max']:.1f}.")
    os.makedirs(a.out, exist_ok=True)
    json.dump(result, open(os.path.join(a.out, "calibrated.json"), "w"), indent=1)
    open(os.path.join(a.out, "calibrated.md"), "w").write("\n".join(lines) + "\n")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
