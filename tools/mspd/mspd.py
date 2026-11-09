"""Multi-Scale Path Divergence (MSPD), discrete-state estimator.

Implements the windowed finite-resolution target of Akhtyrchenko, Katsnelson and
Ustyuzhanin, "Directing Open-Ended Evolution in Artificial Life via Multi-Scale Path
Divergence", arXiv:2606.17091 (v2), sections 3.5-3.6:

  carriers i       : local degrees of freedom (here: lattice sites)
  window law       : L_{i,k,tau} = law of the transition pair (Z_i(u), Z_i(u+tau)),
                     u uniform on the admissible start times of window k     (Eq. 10)
  heterogeneity    : H_k = mean pairwise D(L_i, L_j) over sampled carriers   (Eq. 11, 17)
                     D = Jensen-Shannon distance (the paper's choice for discrete CA)
  pooled null      : H0_k = the same statistic when every carrier's samples are drawn
                     from the window's pooled law (no site-local structure)
  trace            : h_k = phi(H_k - H0_k)                                    (Eq. 13)
  functional       : M(h) = sum_j w_j ||C_j h - U_j C_{j+1} h||^2 / (||C_j h||^2 + eta^2)
                            / sum_j w_j                                       (Eq. 15)
                     C_j = block averaging at dyadic scale r_j, U_j = repetition upsampling
                     (the implementation the paper names), w_j = 1.

Choices the paper leaves open, fixed here:
  phi(x) = max(x - z * sd(H0), 0): a significance floor from the null replicates, so a
  system with no site-local heterogeneity has h = 0 rather than clipped noise.
  eta = z * median_k sd(H0_k): the regulariser is set to the estimator's own noise scale.
  With a fixed small eta (1e-3) the functional, being a ratio, inflates residual noise
  in a near-zero trace to O(1); tying eta to the null spread prevents that. eta is applied
  per trace entry (denominator ||C_j h||^2 + K_j eta^2), so the floor does not shrink
  relative to the signal as the number of windows grows.
  Carriers are sampled among sites that are active (non-zero) at some time in the window.

Properties (tested in tests/test_mspd.py): static dynamics -> 0; homogeneous stationary
noise -> ~0; constant heterogeneity -> ~0 (h constant); heterogeneity organised over
several time scales -> large; a globally synchronous switch -> 0 (no site heterogeneity,
as the paper notes for its S5 family).
"""

import numpy as np


def window_starts(T, s, step):
    return list(range(0, T - s + 1, step))


def _js_pairwise(P):
    """Mean pairwise Jensen-Shannon distance (base 2, in [0, 1]) between rows of P."""
    n = P.shape[0]
    if n < 2:
        return 0.0
    with np.errstate(divide="ignore", invalid="ignore"):
        logP = np.where(P > 0, np.log2(P), 0.0)
        ent = (P * logP).sum(1)
        tot = 0.0
        for a in range(n - 1):
            M = 0.5 * (P[a] + P[a + 1:])
            logM = np.where(M > 0, np.log2(M), 0.0)
            # JS = H(M) - (H(P)+H(Q))/2 = 0.5*sum p log p + 0.5*sum q log q - sum m log m
            js = 0.5 * ent[a] + 0.5 * ent[a + 1:] - (M * logM).sum(1)
            tot += np.sqrt(np.clip(js, 0, None)).sum()
    return tot / (n * (n - 1) / 2)


def window_heterogeneity(Z, a, s, tau, C, n_carriers, null_reps, rng):
    """H_k and pooled-null replicates for window [a, a+s) at lag tau.

    Z: (T, S) integer states in [0, C)."""
    seg = Z[a:a + s]
    active = np.flatnonzero((seg != 0).any(0))
    if active.size < 2 or s <= tau:
        return 0.0, np.zeros(null_reps)
    sites = rng.choice(active, size=min(n_carriers, active.size), replace=False)
    codes = seg[:-tau, sites].astype(np.int64) * C + seg[tau:, sites]   # (m, n)
    m, n = codes.shape
    B = C * C
    counts = np.zeros((n, B))
    for j in range(n):
        counts[j] = np.bincount(codes[:, j], minlength=B)
    H = _js_pairwise(counts / m)
    pooled = counts.sum(0) / counts.sum()
    H0 = np.empty(null_reps)
    for r in range(null_reps):
        sim = rng.multinomial(m, pooled, size=n).astype(float) / m
        H0[r] = _js_pairwise(sim)
    return H, H0


def trace(Z, C, s, step, tau, n_carriers=64, null_reps=6, z=2.0, seed=0):
    """Windowed processed trace h_k and raw H_k, H0_k."""
    rng = np.random.default_rng(seed)
    H, H0m, H0s = [], [], []
    for a in window_starts(Z.shape[0], s, step):
        h, h0 = window_heterogeneity(Z, a, s, tau, C, n_carriers, null_reps, rng)
        H.append(h); H0m.append(h0.mean()); H0s.append(h0.std(ddof=1) if h0.size > 1 else 0.0)
    H, H0m, H0s = map(np.asarray, (H, H0m, H0s))
    h = np.maximum(H - H0m - z * H0s, 0.0)
    eta = z * float(np.median(H0s)) if H0s.size else 1e-3
    return h, H, H0m, max(eta, 1e-6)


def functional(h, n_scales=4, eta=1e-3):
    """Eq. 15 on dyadic scales r = 1, 2, ..., 2^(n_scales-1)."""
    h = np.asarray(h, float)
    top = 2 ** (n_scales - 1)
    K = (len(h) // top) * top
    if K < top:
        raise ValueError(f"trace too short: {len(h)} windows, need >= {top}")
    h = h[:K]
    terms = []
    for j in range(n_scales - 1):
        r = 2 ** j
        Cj = h.reshape(-1, r).mean(1)
        Cn = h.reshape(-1, 2 * r).mean(1)
        Uj = np.repeat(Cn, 2)
        terms.append(((Cj - Uj) ** 2).sum() / ((Cj ** 2).sum() + Cj.size * eta ** 2))
    return float(np.mean(terms)), terms


def mspd(Z, C, s, step, taus, n_scales=5, **kw):
    """MSPD per lag and mean over lags. Returns dict."""
    out = {}
    for tau in taus:
        h, H, H0, eta = trace(Z, C, s, step, tau, **kw)
        val, terms = functional(h, n_scales, eta)
        out[tau] = dict(mspd=val, terms=terms, mean_h=float(h.mean()), mean_H=float(H.mean()),
                        mean_H0=float(H0.mean()), eta=eta, K=int(len(h)), h=h.tolist())
    out["mean"] = float(np.mean([out[t]["mspd"] for t in taus]))
    out["mean_h"] = float(np.mean([out[t]["mean_h"] for t in taus]))
    return out
