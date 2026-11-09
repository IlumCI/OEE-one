#!/usr/bin/env python3
"""MSPD of BFF soup dynamics.

Carriers: byte positions in the first N programs (program slots persist across epochs;
partners change every epoch). State: the byte's instruction class (13 classes: zero,
two no-op shades, ten commands). Input: per-epoch partial dumps from
`oee_bff -dump-every 1 -dump-programs N`.

Usage: bff_mspd.py OUT.json RUN_DIR [RUN_DIR ...] [--window 32] [--step 8] [--taus 1 4]
"""
import argparse
import glob
import json
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mspd  # noqa: E402

CMDS = {ord(c): i + 3 for i, c in enumerate("[]+-.,<>{}")}
LUT = np.array([0 if b == 0 else CMDS.get(b, 1 + (b & 1)) for b in range(256)], np.uint8)


def load(run):
    files = sorted(glob.glob(os.path.join(run, "soup_*.bin")))
    return LUT[np.stack([np.fromfile(f, np.uint8) for f in files])]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("runs", nargs="+")
    ap.add_argument("--window", type=int, default=32)
    ap.add_argument("--step", type=int, default=8)
    ap.add_argument("--taus", type=int, nargs="+", default=[1, 4])
    ap.add_argument("--scales", type=int, default=5)
    a = ap.parse_args()
    rows = []
    for run in a.runs:
        Z = load(run)
        vals, hs = [], []
        for tau in a.taus:
            h, H, H0, eta = mspd.trace(Z, 13, a.window, a.step, tau)
            vals.append(mspd.functional(h, a.scales, eta)[0])
            hs.append(float(h.mean()))
        r = dict(run=os.path.basename(run.rstrip("/")), epochs=int(Z.shape[0]), mspd=float(np.mean(vals)),
                 per_tau=dict(zip(map(str, a.taus), vals)), mean_h=float(np.mean(hs)))
        rows.append(r)
        print(f"{r['run']}: MSPD {r['mspd']:.4f}  mean h {r['mean_h']:.3f}  ({r['epochs']} epochs)", flush=True)
    json.dump(dict(params=vars(a), rows=rows), open(a.out, "w"), indent=1)


if __name__ == "__main__":
    main()
