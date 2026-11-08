#!/usr/bin/env python3
"""Run the activity analysis over a set of Geb runs and write a markdown summary.

Usage: report.py OUT_DIR RUN_DIR [RUN_DIR ...]
Each RUN_DIR is analysed according to what it contains:
  - presence file with S2: reset-shadow normalised test, real and null, both groupings
  - otherwise (independent shadow): classic Bedau-Packard comparison from rulestat
"""

import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import geb_activity as ga  # noqa: E402


def row(name, r):
    tr = r["normalised_median_activity"]["trend"]
    return (f"| {name} | {r['grouping']} | {'null' if r.get('null_control') else 'real'} | "
            f"{r['last_time']} | {r['diversity']['mean_second_half']:.1f} | "
            f"{r['normalised_median_activity']['final']:.0f} | "
            f"{tr['mk_z_second_half']:.2f} | {tr['slope_last_quarter'] * 1e6:.1f} | "
            f"{r['normalised_total_activity']['final']:.0f} | "
            f"{r['normalised_new_activity']['fraction_snapshots_positive']:.2f} | "
            f"{r['classification']} |")


def main():
    out = sys.argv[1]
    runs = sys.argv[2:]
    os.makedirs(out, exist_ok=True)
    lines = [
        "| run | grouping | population | last step | D (2nd half mean) | normalised median "
        "| MK z (median, 2nd half) | median slope, last quarter (per 1e6 steps) "
        "| normalised total | new activity >0 (fraction) | class |",
        "|---|---|---|---|---|---|---|---|---|---|---|",
    ]
    for run in runs:
        name = os.path.basename(run.rstrip("/"))
        pres = os.path.join(run, "presence")
        has_presence = False
        if os.path.exists(pres):
            with open(pres) as f:
                head = [next(f, "") for _ in range(3)]
            has_presence = any(l.split()[1:2] == ["S2"] for l in head if l.strip())
        configs = []
        if has_presence:
            for g in ("pb", "rule"):
                for null in (False, True):
                    configs.append(dict(variant="resets", grouping=g, source="presence", null=null))
        else:
            for g in ("pb", "rule"):
                configs.append(dict(variant="no_resets", grouping=g, source="rulestat", null=False))
        stag = ga.stagnation_time(run)
        if stag is not None:
            name = f"{name} (stagnated; truncated at {stag})"
        for c in configs:
            try:
                r, s = ga.analyse(run, c["variant"], c["grouping"], 20, c["source"], c["null"], stag)
            except SystemExit as e:
                lines.append(f"| {name} | {c['grouping']} | - | - | - | - | - | - | - | - | {e} |")
                continue
            ga.write_outputs(r, s, os.path.join(out, os.path.basename(run.rstrip("/"))), plot=True)
            lines.append(row(name, r))
    with open(os.path.join(out, "summary.md"), "w") as f:
        f.write("\n".join(lines) + "\n")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
