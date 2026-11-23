#!/usr/bin/env python3
"""MSPD of L0 dynamics. Carriers: grid cells. State: oee_l0 -qdump byte (0 empty, else
1 + 3*dominant kernel + density level), so C = 1 + 3*K states.

Usage: l0_mspd.py OUT.json K N QDUMP [QDUMP ...] [--window 64] [--step 16] [--taus 1 4] [--scales 5]
"""
import argparse
import json
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mspd  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out"); ap.add_argument("K", type=int); ap.add_argument("N", type=int); ap.add_argument("runs", nargs="+")
    ap.add_argument("--window", type=int, default=64); ap.add_argument("--step", type=int, default=16)
    ap.add_argument("--taus", type=int, nargs="+", default=[1, 4]); ap.add_argument("--scales", type=int, default=5)
    a = ap.parse_args()
    rows = []
    for run in a.runs:
        Z = np.fromfile(run, np.uint8).reshape(-1, a.N * a.N)
        vals, hs = [], []
        for tau in a.taus:
            h, H, H0, eta = mspd.trace(Z, 1 + 3 * a.K, a.window, a.step, tau)
            vals.append(mspd.functional(h, a.scales, eta)[0]); hs.append(float(h.mean()))
        r = dict(run=run, steps=int(Z.shape[0]), mspd=float(np.mean(vals)), per_tau=dict(zip(map(str, a.taus), vals)),
                 mean_h=float(np.mean(hs)), occupancy=float((Z > 0).mean()))
        rows.append(r)
        print(f"{run}: MSPD {r['mspd']:.4f}  mean h {r['mean_h']:.3f}  occ {r['occupancy']:.3f}", flush=True)
    json.dump(dict(params=vars(a), rows=rows), open(a.out, "w"), indent=1)


if __name__ == "__main__":
    main()
