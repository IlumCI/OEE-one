#!/usr/bin/env python3
"""Synthetic checks for tools/activity/geb_activity.py (component normalisation)."""

import os
import sys
import tempfile

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "tools", "activity"))
import geb_activity as ga  # noqa: E402


def write_run(d, real, shadow):
    for name, snaps in (("rulestat", real), ("SHADOWrulestat", shadow)):
        with open(os.path.join(d, name), "w") as f:
            f.write("header\n= DATA =\n")
            for k, comps in enumerate(snaps):
                f.write(f"{(k + 1) * 1000}:" + "".join(f" {c} 1;" for c in sorted(comps)) + "\n")


def test_screening_and_normalisation():
    # Component "9.1" persists in real; the shadow loses it every snapshot.
    real = [{"9.1"} for _ in range(10)]
    shadow = [set() for _ in range(10)]
    rp = ga.screened(real, real)
    sp = ga.screened(shadow, real)
    stats, a0, mn = ga.normalised(rp, sp)
    # First snapshot screened (no previous); then +1 per snapshot.
    assert stats["Acum"][-1] == 9, stats["Acum"]
    assert mn == 0 and a0 == 1
    # Neutral: shadow keeps it too -> normalised activity stays 0.
    stats, _, _ = ga.normalised(rp, ga.screened(real, real))
    assert stats["Acum"][-1] == 0


def test_isolated_occurrence_screened():
    real = [{"a"}, set(), {"a"}, {"a"}]
    rp = ga.screened(real, real)
    assert rp == [set(), set(), set(), {"a"}]


def test_selection_vs_neutral_classification():
    rng = np.random.default_rng(1)
    n = 600
    with tempfile.TemporaryDirectory() as sel, tempfile.TemporaryDirectory() as neu:
        # Selected (3b pattern): 10 persistent adaptive components plus 5 turnover
        # slots; the shadow loses each component half the time. Diversity is bounded,
        # median normalised activity grows without bound, new activity is positive.
        real, shadow = [], []
        core = [f"9.{i}" for i in range(1, 11)]
        for k in range(n):
            turnover = [f"{11 + (k // 20) % 50}.{j}" for j in range(1, 6)]
            active = core + turnover
            real.append(set(active))
            shadow.append({c for c in active if rng.random() < 0.5})
        write_run(sel, real, shadow)
        r, _ = ga.analyse(sel, "resets", "rule", 20)
        assert r["classification"].startswith("3"), r["classification"]
        # Neutral: shadow behaves exactly like real (no selection) -> not class 3.
        write_run(neu, real, real)
        r, _ = ga.analyse(neu, "resets", "rule", 20)
        assert not r["classification"].startswith("3"), r["classification"]


if __name__ == "__main__":
    for name, fn in list(globals().items()):
        if name.startswith("test_"):
            fn()
            print("ok", name)
