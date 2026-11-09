#!/usr/bin/env python3
"""MSPD of Geb dynamics from checkpoints.

For each checkpoint: replay CYCLES cycles (tools/replay, grid mode), take world cells as
carriers with 17 discrete states (empty; heading quadrant x action: none/reproduce/kill),
and compute MSPD (tools/mspd/mspd.py).

Controls must be matched systems (the paper compares against matched random parameters),
e.g. unevolved worlds with random genomes. A shuffled-window surrogate is NOT a valid
null: the functional rewards fine-scale variation, which shuffling maximises, so even a
known multi-scale system scores below its own shuffle (tests/test_mspd.py).

Usage: geb_mspd.py OUT.json REPLAY_BIN CHECKPOINT [CHECKPOINT ...]
         [--cycles 2048] [--window 64] [--step 16] [--taus 1 4] [--states full|action|occupancy]
"""
import argparse
import json
import os
import re
import subprocess
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mspd  # noqa: E402


def grid(replay, ck, cycles):
    out = subprocess.run([replay, ck, str(cycles), "1", "grid"], capture_output=True, check=True).stdout
    Z = np.frombuffer(out, np.uint8)
    S = int(round(np.sqrt(Z.size / cycles)))
    return Z.reshape(cycles, S * S)


def coarsen(Z, mode):
    """full: 17 states; action: empty/idle/reproduce/kill (4); occupancy: empty/occupied (2)."""
    if mode == "full":
        return Z, 17
    if mode == "action":
        return np.where(Z == 0, 0, 1 + (Z.astype(np.int16) - 1) // 4 - ((Z.astype(np.int16) - 1) // 4 >= 2)).astype(np.uint8), 4
    return (Z != 0).astype(np.uint8), 2


def analyse(Z, a):
    Z, C = coarsen(Z, a.states)
    res = dict(per_tau={}, occupancy=float((Z != 0).mean()))
    vals = []
    for tau in a.taus:
        h, H, H0, eta = mspd.trace(Z, C, a.window, a.step, tau)
        v, _ = mspd.functional(h, a.scales, eta)
        res["per_tau"][tau] = dict(mspd=v, mean_h=float(h.mean()), mean_H=float(H.mean()),
                                   mean_H0=float(H0.mean()), eta=eta)
        vals.append(v)
    res["mspd"] = float(np.mean(vals))
    res["mean_h"] = float(np.mean([r["mean_h"] for r in res["per_tau"].values()]))
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("replay")
    ap.add_argument("checkpoints", nargs="+")
    ap.add_argument("--cycles", type=int, default=2048)
    ap.add_argument("--window", type=int, default=64)
    ap.add_argument("--step", type=int, default=16)
    ap.add_argument("--taus", type=int, nargs="+", default=[1, 4])
    ap.add_argument("--scales", type=int, default=5)
    ap.add_argument("--states", choices=["full", "action", "occupancy"], default="full")
    a = ap.parse_args()
    rows = []
    for ck in a.checkpoints:
        m = re.search(r"archive\.(\d+)$", ck)
        Z = grid(a.replay, ck, a.cycles)
        r = analyse(Z, a)
        r.update(checkpoint=ck, time=int(m.group(1)) if m else None, sites=int(Z.shape[1]))
        rows.append(r)
        print(f"{ck}: MSPD {r['mspd']:.4f}  mean h {r['mean_h']:.3f}  "
              f"occupancy {r['occupancy']:.2f}", flush=True)
    with open(a.out, "w") as f:
        json.dump(dict(params=vars(a), rows=rows), f, indent=1)


if __name__ == "__main__":
    main()
