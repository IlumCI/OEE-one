#!/usr/bin/env python3
"""Summarise a BFF seed sweep (tools/bff/sweep.sh).

Per seed: transition epoch (first logged epoch with high-order entropy >= 1, the paper's
criterion), final entropy, and from the final checkpoint (bff_inspect) the number of
self-replicators (cubff CheckSelfRep >= 5) and distinct programs. A transition "held" if
at least 10% of programs are self-replicators at the end of the run.

Usage: analyse.py OUT_DIR SWEEP_DIR INSPECT_BIN
"""
import glob
import json
import math
import os
import subprocess
import sys


def wilson(k, n, z=1.96):
    if n == 0:
        return (float("nan"), float("nan"))
    p = k / n
    d = 1 + z * z / n
    c = (p + z * z / (2 * n)) / d
    h = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / d
    return (c - h, c + h)


def main():
    out, sweep, inspect = sys.argv[1:4]
    rows = []
    for d in sorted(glob.glob(os.path.join(sweep, "seed_*")), key=lambda p: int(p.rsplit("_", 1)[1])):
        if not os.path.exists(os.path.join(d, "done")):
            continue
        seed = int(d.rsplit("_", 1)[1])
        lines = open(os.path.join(d, "log.csv")).read().splitlines()[1:]
        log = [(int(l.split(",")[0]), float(l.split(",")[3]), float(l.split(",")[6])) for l in lines]
        t = next((e for e, h, _ in log if h >= 1), None)
        peak = max(h for _, h, _ in log)
        insp = json.loads(subprocess.run([inspect, os.path.join(d, "ck"), "3", "1"], capture_output=True,
                                         check=True, text=True).stdout)
        frac = insp["selfreplicators"] / insp["programs"]
        rows.append(dict(seed=seed, transition=t, final_epoch=log[-1][0], final_entropy=log[-1][1], peak_entropy=peak,
                         selfrep_fraction=frac, distinct=insp["distinct"], held=bool(t is not None and frac >= 0.1),
                         top_program=insp["top"][0]["program"] if t is not None and frac >= 0.1 else None,
                         final_ops_per_run=log[-1][2]))
        print(rows[-1], flush=True)
    n = len(rows)
    k = sum(r["transition"] is not None for r in rows)
    h = sum(r["held"] for r in rows)
    lo, hi = wilson(k, n)
    os.makedirs(out, exist_ok=True)
    json.dump(dict(runs=rows, n=n, transitions=k, held=h, ci95=[lo, hi]), open(os.path.join(out, "sweep.json"), "w"),
              indent=1)
    md = ["| seed | transition epoch | peak H | final H | self-replicators at end | distinct programs | outcome |",
          "|---|---|---|---|---|---|---|"]
    for r in rows:
        outcome = "held" if r["held"] else ("collapsed" if r["transition"] is not None else "no transition")
        md.append(f"| {r['seed']} | {r['transition'] or '-'} | {r['peak_entropy']:.2f} | {r['final_entropy']:.2f} | "
                  f"{100 * r['selfrep_fraction']:.1f}% | {r['distinct']} | {outcome} |")
    md.append(f"\nTransitions within 16,384 epochs: {k}/{n} = {100 * k / n:.0f}% "
              f"(95% Wilson CI {100 * lo:.0f}-{100 * hi:.0f}%); paper: about 40% of 1000 runs. "
              f"Held to the end of the run: {h}/{k}.")
    open(os.path.join(out, "sweep.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))


if __name__ == "__main__":
    main()
