#!/usr/bin/env python3
"""Evolutionary activity statistics for Geb rulestat records.

Implements:
  - Bedau & Packard activity by presence with shadow comparison (GPEM 2006 sec. 2, 6)
    for the independent-shadow variant ("no_resets").
  - Channon's component activity normalisation with a reset shadow (GPEM 2006 sec. 8-9)
    for the "resets" variant.
  - Isolated-occurrence screening (GPEM sec. 5.1).
  - Component grouping: "rule" (whole rule, as recorded) or "pb" (predecessor plus
    link bits, GPEM sec. 5), the latter using the component dictionary written by
    oee_geb -components.
  - Classification evidence per Table I with median activity (GPEM sec. 7, 10).

Activity unit: one per snapshot of presence (snapshots every 1000 steps).

Usage: geb_activity.py RUN_DIR --variant resets|no_resets [--grouping rule|pb]
                       [--source rulestat|presence] [--null] [--out OUT_DIR] [--plot]
"""

from __future__ import annotations

import argparse
import json
import math
import os
import sys
from collections import defaultdict

import numpy as np


# ---------------------------------------------------------------------------
# Parsing

def read_rulestat(path):
    """Yield (time, {name: abundance}) per snapshot line."""
    with open(path, "r", encoding="ascii") as f:
        for line in f:
            if not line[:1].isdigit():
                continue
            head, _, rest = line.partition(":")
            t = int(head)
            comps = {}
            for item in rest.split(";"):
                item = item.strip()
                if not item:
                    continue
                name, ab = item.split()
                comps[name] = int(ab)
            yield t, comps


def read_dictionary(path):
    d = {}
    with open(path, "r", encoding="ascii") as f:
        for line in f:
            parts = line.split()
            if len(parts) != 4:
                continue
            _, pop, name, genes = parts
            d[(pop, name)] = genes
    return d


def pb_key(genes: str) -> str:
    """Predecessor (paired form) plus the six link bits."""
    cur = 0
    n = len(genes)
    while cur < n and genes[cur] == "0":
        cur += 2
    return genes[:cur] + "|" + genes[-6:]


class Grouper:
    def __init__(self, grouping, dictionary, variant):
        self.grouping = grouping
        self.dictionary = dictionary
        self.variant = variant

    def _genes(self, *keys):
        for k in keys:
            if k in self.dictionary:
                return self.dictionary[k]
        raise KeyError(keys)

    def real(self, name):
        if self.grouping == "rule":
            return name
        return pb_key(self._genes(("R", name), ("S", name)))

    def shadow(self, name):
        if self.variant == "no_resets":
            # Independent shadow: its own component namespace.
            if self.grouping == "rule":
                return "S" + name
            return pb_key(self._genes(("S", "S" + name)))
        # resets: names without 's' refer to the shared (real) list.
        if self.grouping == "rule":
            return name
        return pb_key(self._genes(("S", name), ("R", name)))


def load_presence(run_dir, variant, grouping):
    dictionary = None
    if grouping == "pb":
        dictionary = read_dictionary(os.path.join(run_dir, "components"))
    g = Grouper(grouping, dictionary, variant)
    real = list(read_rulestat(os.path.join(run_dir, "rulestat")))
    shadow = list(read_rulestat(os.path.join(run_dir, "SHADOWrulestat")))
    n = min(len(real), len(shadow))
    real, shadow = real[:n], shadow[:n]
    real_sets, shadow_sets = [], []
    for (_, rc), (_, sc) in zip(real, shadow):
        try:
            rs = set(g.real(k) for k in rc)
            ss = set(g.shadow(k) for k in sc)
        except KeyError:
            # Dictionary not yet written for this snapshot (run in progress).
            break
        real_sets.append(rs)
        shadow_sets.append(ss)
    times = np.array([t for t, _ in real[: len(real_sets)]], dtype=np.int64)
    return times, real_sets, shadow_sets


def load_presence_file(run_dir, grouping):
    """Read the hash-based presence record written by oee_geb -presence."""
    part = 0 if grouping == "rule" else 1
    by_time = defaultdict(dict)
    with open(os.path.join(run_dir, "presence"), "r", encoding="ascii") as f:
        for line in f:
            fields = line.split()
            if len(fields) < 3 or len(fields) != 3 + int(fields[2]):
                continue  # incomplete trailing line
            t, pop = int(fields[0]), fields[1]
            by_time[t][pop] = set(x.split(":")[part] for x in fields[3:])
    times = sorted(t for t, d in by_time.items() if "R" in d and "S" in d)
    pops = {p: [by_time[t].get(p, set()) for t in times] for p in ("R", "S", "S2")}
    has_s2 = all("S2" in by_time[t] for t in times)
    return np.array(times, dtype=np.int64), pops, has_s2


# ---------------------------------------------------------------------------
# Statistics

def screened(sets, previous_sets):
    """Present at k and in previous_sets[k-1] (isolated occurrences removed)."""
    out = [set()]
    for k in range(1, len(sets)):
        out.append(sets[k] & previous_sets[k - 1])
    return out


def bedau_packard(present):
    """Activity by presence. Returns per-snapshot arrays and the activity samples."""
    cum = defaultdict(int)
    D, Acum, Amean, Amed = [], [], [], []
    samples = []  # (k, activity) for present components
    for k, s in enumerate(present):
        acts = []
        for c in s:
            cum[c] += 1
            acts.append(cum[c])
        samples.append(np.array(acts, dtype=np.int64))
        D.append(len(s))
        tot = int(sum(acts))
        Acum.append(tot)
        Amean.append(tot / len(s) if s else 0.0)
        Amed.append(float(np.median(acts)) if acts else 0.0)
    return dict(D=np.array(D), Acum=np.array(Acum, float), Amean=np.array(Amean),
                Amedian=np.array(Amed)), samples


def new_activity(samples, D, a0, a1):
    out = np.zeros(len(samples))
    for k, acts in enumerate(samples):
        if D[k]:
            sel = acts[(acts >= a0) & (acts <= a1)]
            out[k] = sel.sum() / D[k]
    return out


def normalised(real_present, shadow_present):
    """Component activity normalisation (GPEM sec. 9)."""
    cum = defaultdict(int)
    reached = set()
    D, A, Amean, Amed, raw_new = [], [], [], [], []
    min_seen = 0
    per_snapshot = []
    for k in range(len(real_present)):
        R, S = real_present[k], shadow_present[k]
        for c in R | S:
            cum[c] += (1 if c in R else 0) - (1 if c in S else 0)
        acts = np.array([cum[c] for c in R], dtype=np.int64)
        per_snapshot.append([(c, cum[c]) for c in R])
        if acts.size:
            min_seen = min(min_seen, int(acts.min()))
        D.append(len(R))
        tot = float(acts.sum()) if acts.size else 0.0
        A.append(tot)
        Amean.append(tot / len(R) if R else 0.0)
        Amed.append(float(np.median(acts)) if acts.size else 0.0)
    a0 = max(1, -min_seen)
    for k, comps in enumerate(per_snapshot):
        s = 0.0
        for c, a in comps:
            if a >= a0 and c not in reached:
                reached.add(c)
                s += a
        raw_new.append(s / D[k] if D[k] else 0.0)
    return dict(D=np.array(D), Acum=np.array(A), Amean=np.array(Amean),
                Amedian=np.array(Amed), Anew=np.array(raw_new)), a0, min_seen


# ---------------------------------------------------------------------------
# Trend evidence

def theil_sen(x, y):
    """Theil-Sen slope with a subsample for large n."""
    n = len(x)
    if n < 3:
        return 0.0
    idx = np.arange(n)
    if n > 2000:
        rng = np.random.default_rng(0)
        i = rng.integers(0, n, 400000)
        j = rng.integers(0, n, 400000)
    else:
        i, j = np.triu_indices(n, 1)
    m = i != j
    i, j = i[m], j[m]
    dx = x[j] - x[i]
    ok = dx != 0
    return float(np.median((y[j][ok] - y[i][ok]) / dx[ok]))


def mann_kendall_z(y, block=1):
    """Mann-Kendall Z on block means (block>1 reduces autocorrelation)."""
    if block > 1:
        m = len(y) // block
        y = y[: m * block].reshape(m, block).mean(axis=1)
    n = len(y)
    if n < 4:
        return 0.0
    s = 0.0
    for k in range(n - 1):
        s += np.sign(y[k + 1:] - y[k]).sum()
    var = n * (n - 1) * (2 * n + 5) / 18.0
    if s > 0:
        return (s - 1) / math.sqrt(var)
    if s < 0:
        return (s + 1) / math.sqrt(var)
    return 0.0


def trend(times, y, block):
    half = len(y) // 2
    quarter = (3 * len(y)) // 4
    return dict(
        slope_second_half=theil_sen(times[half:].astype(float), y[half:]),
        slope_last_quarter=theil_sen(times[quarter:].astype(float), y[quarter:]),
        mk_z_second_half=mann_kendall_z(y[half:], block),
    )


def unbounded_evidence(tr, z=2.58):
    return tr["mk_z_second_half"] > z and tr["slope_second_half"] > 0 and tr["slope_last_quarter"] > 0


def classify(D_tr, med_tr, new_mean, new_frac_positive):
    D_unb = unbounded_evidence(D_tr)
    med_unb = unbounded_evidence(med_tr)
    new_pos = new_mean > 0 and new_frac_positive > 0
    if not new_pos:
        return "1 (no significant new activity)"
    if D_unb and med_unb:
        return "3c (unbounded D and median activity)"
    if med_unb:
        return "3b (unbounded median activity, bounded D)"
    if D_unb:
        return "3a (unbounded D)"
    return "2 (bounded)"


# ---------------------------------------------------------------------------

def analyse(run_dir, variant, grouping, block, source="rulestat", null=False):
    if source == "presence":
        times, pops, has_s2 = load_presence_file(run_dir, grouping)
        real_sets, shadow_sets = pops["R"], pops["S"]
        if null and not has_s2:
            raise SystemExit("--null requires a run with -second-shadow")
    else:
        if null:
            raise SystemExit("--null requires --source presence")
        times, real_sets, shadow_sets = load_presence(run_dir, variant, grouping)
    if len(times) < 8:
        raise SystemExit(f"too few snapshots ({len(times)})")
    real_present = screened(real_sets, real_sets)
    result = dict(run_dir=run_dir, variant=variant, grouping=grouping, source=source,
                  null_control=null, snapshots=int(len(times)), last_time=int(times[-1]))
    series = dict(time=times)
    if null:
        # Null control: an independent reset shadow takes the place of the real run.
        # Both shadows start each interval from the real state at k-1.
        real_present = screened(pops["S2"], real_sets)
    if variant == "resets":
        # Shadow at k was reset to the real state at k-1.
        shadow_present = screened(shadow_sets, real_sets)
        stats, a0, min_seen = normalised(real_present, shadow_present)
        result.update(a0=a0, most_negative_normalised_activity=min_seen)
        for k, v in stats.items():
            series["N_" + k] = v
        D, med, new = stats["D"], stats["Amedian"], stats["Anew"]
    else:
        shadow_present = screened(shadow_sets, shadow_sets)
        rs, rsamp = bedau_packard(real_present)
        ss, ssamp = bedau_packard(shadow_present)
        tail = len(times) - len(times) // 6
        tail_shadow = np.concatenate([a for a in ssamp[tail:] if a.size] or [np.zeros(1)])
        a0 = float(np.percentile(tail_shadow, 96.5))
        a1 = a0 * (3.2 / 2.8)
        rs["Anew"] = new_activity(rsamp, rs["D"], a0, a1)
        ss["Anew"] = new_activity(ssamp, ss["D"], a0, a1)
        result.update(a0=a0, a1=a1)
        for k, v in rs.items():
            series["R_" + k] = v
        for k, v in ss.items():
            series["S_" + k] = v
        for k in ("Acum", "Amean", "Amedian", "Anew"):
            series["N_" + k] = rs[k] - ss[k]
        D, med, new = rs["D"], series["N_Amedian"], series["N_Anew"]
    D_tr = trend(times, D.astype(float), block)
    med_tr = trend(times, med, block)
    tot_tr = trend(times, series["N_Acum"], block)
    half = len(times) // 2
    new_mean = float(np.mean(new[half:]))
    new_frac = float(np.mean(new[half:] > 0))
    result.update(
        diversity=dict(mean_second_half=float(np.mean(D[half:])), trend=D_tr),
        normalised_median_activity=dict(final=float(med[-1]), trend=med_tr),
        normalised_total_activity=dict(final=float(series["N_Acum"][-1]), trend=tot_tr),
        normalised_new_activity=dict(mean_second_half=new_mean,
                                     fraction_snapshots_positive=new_frac),
        classification=classify(D_tr, med_tr, new_mean, new_frac),
    )
    return result, series


def write_outputs(result, series, out_dir, plot):
    os.makedirs(out_dir, exist_ok=True)
    tag = f"{result['variant']}_{result['grouping']}_{result['source']}" + ("_null" if result["null_control"] else "")
    with open(os.path.join(out_dir, f"summary_{tag}.json"), "w") as f:
        json.dump(result, f, indent=2)
    keys = list(series)
    with open(os.path.join(out_dir, f"series_{tag}.csv"), "w") as f:
        f.write(",".join(keys) + "\n")
        for row in zip(*(series[k] for k in keys)):
            f.write(",".join(f"{v:g}" if isinstance(v, float) else str(v) for v in row) + "\n")
    if not plot:
        return
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    t = series["time"]
    names = ["N_Acum", "N_Amean", "N_Amedian", "N_Anew", "N_D" if "N_D" in series else "R_D"]
    fig, axes = plt.subplots(len(names), 1, figsize=(8, 10), sharex=True)
    for ax, k in zip(axes, names):
        ax.plot(t, series[k], lw=0.6)
        if k.startswith("N_A") and "R_" + k[2:] in series:
            ax.plot(t, series["R_" + k[2:]], lw=0.4, alpha=0.6, label="real")
            ax.plot(t, series["S_" + k[2:]], lw=0.4, alpha=0.6, label="shadow")
            ax.legend(fontsize=7)
        ax.set_ylabel(k, fontsize=8)
    axes[-1].set_xlabel("time step")
    fig.suptitle(f"{os.path.basename(result['run_dir'].rstrip('/'))}: {result['classification']}",
                 fontsize=9)
    fig.tight_layout()
    fig.savefig(os.path.join(out_dir, f"activity_{tag}.png"), dpi=120)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("run_dir")
    ap.add_argument("--variant", required=True, choices=["resets", "no_resets"])
    ap.add_argument("--grouping", default="rule", choices=["rule", "pb"])
    ap.add_argument("--source", default="rulestat", choices=["rulestat", "presence"])
    ap.add_argument("--null", action="store_true",
                    help="null control: second shadow in place of the real run")
    ap.add_argument("--block", type=int, default=20, help="block size for Mann-Kendall")
    ap.add_argument("--out", default=None)
    ap.add_argument("--plot", action="store_true")
    a = ap.parse_args(argv)
    result, series = analyse(a.run_dir, a.variant, a.grouping, a.block, a.source, a.null)
    write_outputs(result, series, a.out or os.path.join(a.run_dir, "analysis"), a.plot)
    json.dump(result, sys.stdout, indent=2)
    print()


if __name__ == "__main__":
    main()
