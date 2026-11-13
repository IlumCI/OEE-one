#!/usr/bin/env python3
"""Diversity D over time in fixed windows, real and second-shadow null.

Usage: d_series.py RUN [RUN ...] [--grouping rule|pb] [--window 500000]
Prints per run and window: mean D (real), mean D (null), mean population N (from log "N" lines
when present). Intended for growing-world runs (L3 v7), where windows align with growth steps.
"""
import argparse
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import geb_activity as ga  # noqa: E402


def series(run, grouping, null):
    _, s = ga.analyse(run, "resets", grouping, 20, "presence", null, ga.stagnation_time(run))
    return np.asarray(s["time"], float), np.asarray(s["N_D"], float)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runs", nargs="+")
    ap.add_argument("--grouping", default="rule")
    ap.add_argument("--window", type=int, default=500000)
    a = ap.parse_args()
    for run in a.runs:
        t, d = series(run, a.grouping, False)
        tn, dn = series(run, a.grouping, True)
        print(f"## {os.path.basename(run.rstrip('/'))} ({a.grouping})")
        print("| window start | D real | D null |")
        print("|---|---|---|")
        for lo in range(0, int(t.max()) + 1, a.window):
            sel, seln = (t >= lo) & (t < lo + a.window), (tn >= lo) & (tn < lo + a.window)
            if sel.any():
                print(f"| {lo} | {d[sel].mean():.1f} | {dn[seln].mean() if seln.any() else float('nan'):.1f} |")
        print()


if __name__ == "__main__":
    main()
