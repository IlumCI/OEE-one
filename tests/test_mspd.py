"""Sanity families for the MSPD estimator (cf. the paper's synthetic N0 suite)."""
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "mspd"))
import mspd  # noqa: E402

T, S, C = 2048, 256, 3
S_, STEP, TAUS = 64, 16, (1, 4)


def chain(rng, n, stay, T):
    """Two-state (1, 2) Markov chains with self-transition probability `stay`, n sites."""
    z = np.empty((T, n), np.uint8)
    z[0] = rng.integers(1, 3, n)
    for t in range(1, T):
        flip = rng.random(n) > stay[t] if np.ndim(stay) else rng.random(n) > stay
        z[t] = np.where(flip, 3 - z[t - 1], z[t - 1])
    return z


def families(seed=0):
    rng = np.random.default_rng(seed)
    f = {}
    f["static"] = np.ones((T, S), np.uint8)
    f["homogeneous"] = chain(rng, S, 0.5, T)
    # Constant heterogeneity: half the sites fast, half slow, forever.
    f["constant_het"] = np.hstack([chain(rng, S // 2, 0.5, T), chain(rng, S // 2, 0.97, T)])
    # Heterogeneity switched on and off on several nested time scales.
    t = np.arange(T)
    on = ((t // 128) % 2) * ((t // 512) % 2) + ((t // 64) % 2) * (1 - (t // 1024) % 2)
    stay_b = np.where(on > 0, 0.97, 0.5)
    f["multiscale_het"] = np.hstack([chain(rng, S // 2, 0.5, T), chain(rng, S // 2, stay_b, T)])
    # Every site switches together: no site-to-site heterogeneity at any time.
    stay_all = np.where((t // 256) % 2, 0.97, 0.5)
    f["global_switch"] = chain(rng, S, stay_all, T)
    return f


def main():
    res = {k: mspd.mspd(Z, C, S_, STEP, TAUS)["mean"] for k, Z in families().items()}
    for k, v in res.items():
        print(f"{k:16s} MSPD {v:.4f}")
    assert res["static"] == 0.0
    assert res["global_switch"] < 0.05
    assert res["homogeneous"] < 0.05
    assert res["constant_het"] < 0.1
    floor = max(res["homogeneous"], res["constant_het"])
    assert res["multiscale_het"] > 10 * floor
    assert res["multiscale_het"] > 2 * res["global_switch"]
    # A shuffled-window surrogate is not a valid null: it scores above a true multi-scale
    # system because shuffling maximises fine-scale variation.
    Z = families()["multiscale_het"]
    h, _, _, eta = mspd.trace(Z, C, S_, STEP, 1)
    rng = np.random.default_rng(1)
    sh = np.mean([mspd.functional(rng.permutation(h), 5, eta)[0] for _ in range(20)])
    assert sh > mspd.functional(h, 5, eta)[0]
    print("ok")


if __name__ == "__main__":
    main()
