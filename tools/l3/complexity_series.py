#!/usr/bin/env python3
"""Maximum individual complexity (Geb max_used_component_count, Channon 2019) per time window.

Usage: complexity_series.py RUN [RUN ...] [--window 500000]
"""
import argparse
import os
import re

import numpy as np

PAT = re.compile(r"Complexities of used components at time=(\d+): max_used_component_count=(\d+)")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runs", nargs="+")
    ap.add_argument("--window", type=int, default=500000)
    a = ap.parse_args()
    rows = {}
    for run in a.runs:
        t, c = [], []
        with open(os.path.join(run, "log.txt")) as f:
            for line in f:
                m = PAT.search(line)
                if m:
                    t.append(int(m.group(1))); c.append(int(m.group(2)))
        t, c = np.array(t), np.array(c, float)
        rows[os.path.basename(run.rstrip("/"))] = {lo: c[(t >= lo) & (t < lo + a.window)].mean()
                                                   for lo in range(0, int(t.max()) + 1, a.window)
                                                   if ((t >= lo) & (t < lo + a.window)).any()}
    wins = sorted({w for r in rows.values() for w in r})
    print("| window start | " + " | ".join(rows) + " |")
    print("|---" * (len(rows) + 1) + "|")
    for w in wins:
        print(f"| {w} | " + " | ".join(f"{r[w]:.2f}" if w in r else "" for r in rows.values()) + " |")


if __name__ == "__main__":
    main()
