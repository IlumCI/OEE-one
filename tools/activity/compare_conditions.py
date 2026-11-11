#!/usr/bin/env python3
"""Compare evolutionary-diversity growth between conditions (L3 confirmation test).

For each run: diversity D (number of components present, predecessor + link-bit grouping,
isolated-occurrence screened, as in report.py) over snapshots; statistic = Theil-Sen slope
of D over the second half of the run [T/2, T] (per 1e6 steps), plus the mean D over the
second half. Groups are compared with an exact two-sample permutation test on the slope
(one-sided: group A > group B) and Hedges' g.

Usage: compare_conditions.py OUT_DIR --a RUN [RUN ...] --b RUN [RUN ...] [--label-a A --label-b B]
"""
import argparse
import itertools
import json
import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import geb_activity as ga  # noqa: E402


def run_stats(run):
    stag = ga.stagnation_time(run)
    _, s = ga.analyse(run, "resets", "pb", 20, "presence", False, stag)
    t = s["time"].astype(float)
    D = np.asarray(s["N_D"], float)
    half = len(t) // 2
    return dict(run=os.path.basename(run.rstrip("/")), end=float(t[-1]),
                slope=ga.theil_sen(t[half:], D[half:]) * 1e6, mean_D=float(D[half:].mean()))


def perm_p(a, b):
    """Exact one-sided p for mean(a) - mean(b) >= observed over all relabelings."""
    pooled = list(a) + list(b)
    na = len(a)
    obs = np.mean(a) - np.mean(b)
    cnt = tot = 0
    for idx in itertools.combinations(range(len(pooled)), na):
        sa = [pooled[i] for i in idx]
        sb = [pooled[i] for i in range(len(pooled)) if i not in idx]
        cnt += (np.mean(sa) - np.mean(sb)) >= obs - 1e-12
        tot += 1
    return cnt / tot, 1 / tot


def hedges_g(a, b):
    a, b = np.asarray(a, float), np.asarray(b, float)
    na, nb = len(a), len(b)
    if na < 2 or nb < 2:
        return float("nan")
    sp = math.sqrt(((na - 1) * a.var(ddof=1) + (nb - 1) * b.var(ddof=1)) / (na + nb - 2))
    if sp == 0:
        return float("nan")
    return float((a.mean() - b.mean()) / sp * (1 - 3 / (4 * (na + nb) - 9)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--a", nargs="+", required=True)
    ap.add_argument("--b", nargs="+", required=True)
    ap.add_argument("--label-a", default="A")
    ap.add_argument("--label-b", default="B")
    a = ap.parse_args()
    A = [run_stats(r) for r in a.a]
    B = [run_stats(r) for r in a.b]
    res = {}
    for key in ("slope", "mean_D"):
        p, pmin = perm_p([r[key] for r in A], [r[key] for r in B])
        res[key] = dict(p=p, p_min=pmin, hedges_g=hedges_g([r[key] for r in A], [r[key] for r in B]))
    lines = [f"| run | condition | end step | D slope, 2nd half (per 1e6) | mean D, 2nd half |", "|---|---|---|---|---|"]
    for lab, rows in ((a.label_a, A), (a.label_b, B)):
        for r in rows:
            lines.append(f"| {r['run']} | {lab} | {r['end']:.0f} | {r['slope']:.2f} | {r['mean_D']:.1f} |")
    for key in ("slope", "mean_D"):
        lines.append(f"\n{key}: {a.label_a} > {a.label_b}, exact permutation p = {res[key]['p']:.4f} "
                     f"(minimum attainable {res[key]['p_min']:.4f}), Hedges' g = {res[key]['hedges_g']:.2f}")
    os.makedirs(a.out, exist_ok=True)
    json.dump(dict(a=A, b=B, tests=res, labels=[a.label_a, a.label_b]), open(os.path.join(a.out, "compare.json"), "w"), indent=1)
    open(os.path.join(a.out, "compare.md"), "w").write("\n".join(lines) + "\n")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
