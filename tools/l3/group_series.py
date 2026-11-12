#!/usr/bin/env python3
"""Summarise L3 group-structure lines ("time: G<t>G ...") from Geb logs.

Usage: group_series.py RUN_DIR [RUN_DIR ...] [--bins 8]
Prints, per run, the mean of every logged group field in equal time bins.
"""
import argparse
import os
import re

import numpy as np


def parse(run):
    rows = []
    with open(os.path.join(run, "log.txt")) as f:
        for line in f:
            if not line.startswith("time: G"):
                continue
            t = int(re.search(r"G(\d+)G", line).group(1))
            kv = dict(zip(*[iter(line.split("\t", 1)[1].split())] * 2))
            rows.append((t, {k: float(v) for k, v in kv.items()}))
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runs", nargs="+")
    ap.add_argument("--bins", type=int, default=8)
    a = ap.parse_args()
    for run in a.runs:
        rows = parse(run)
        if not rows:
            print(f"{run}: no group lines")
            continue
        keys = [k for k in rows[-1][1] if k != "propagules"]
        t = np.array([r[0] for r in rows])
        edges = np.linspace(t.min(), t.max() + 1, a.bins + 1)
        print(f"## {os.path.basename(run.rstrip('/'))}")
        print("| t (start) | " + " | ".join(keys) + " |")
        print("|---" * (len(keys) + 1) + "|")
        for lo, hi in zip(edges[:-1], edges[1:]):
            sel = [r[1] for r in rows if lo <= r[0] < hi]
            if not sel:
                continue
            vals = [np.mean([s.get(k, np.nan) for s in sel]) for k in keys]
            print(f"| {int(lo)} | " + " | ".join(f"{v:.3g}" for v in vals) + " |")
        print()


if __name__ == "__main__":
    main()
