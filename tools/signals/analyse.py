#!/usr/bin/env python3
"""Signalling analysis from `signals` output. Usage: analyse.py NORMAL.tsv NORMAL.err DEAF.tsv DEAF.err
1. Listening: I(heard; did) per agent-step, minus a null with heard shuffled among agents of the same step.
2. Action rates conditional on each heard signal class.
3. Intervention: deaf vs normal (births, population, action rates)."""
import sys
import numpy as np
ACT = ['reproduce', 'attack', 'move', 'turn', 'bond']
CLS = ['reproduce signal', 'attack/move signal', 'bond signal']

def mi(x, y):
    xv, xi = np.unique(x, return_inverse=True); yv, yi = np.unique(y, return_inverse=True)
    j = np.zeros((len(xv), len(yv))); np.add.at(j, (xi, yi), 1); j /= j.sum()
    px = j.sum(1, keepdims=True); py = j.sum(0, keepdims=True); nz = j > 0
    return float((j[nz] * np.log2(j[nz] / (px @ py)[nz])).sum())

def load(f):
    a = np.loadtxt(f, dtype=np.int64); return a[:, 0], a[:, 1], a[:, 2]

t, h, d = load(sys.argv[1]); rng = np.random.default_rng(0)
real = mi(h, d); hs = h.copy()
nulls = []
for _ in range(20):
    for tt in np.unique(t):
        idx = np.where(t == tt)[0]; hs[idx] = rng.permutation(h[idx])
    nulls.append(mi(hs, d))
nulls = np.array(nulls)
print(f'agent-steps {len(t)}; H(did) = {mi(d, d):.3f} bits')
print(f'I(heard; did) = {real:.4f} bits; shuffled null {nulls.mean():.4f} +- {nulls.std():.4f}; excess {real - nulls.mean():.4f} bits ({(real - nulls.mean()) / mi(d, d) * 100:.1f}% of action entropy)')
print('\naction rates by what was heard (rows: heard class present / absent)')
for k, c in enumerate(CLS):
    on = (h >> k) & 1 == 1
    if on.sum() == 0: continue
    print(f'  {c:20s} heard in {on.mean()*100:5.1f}% |', '  '.join(f'{a} {((d[on] >> j) & 1).mean():.3f}/{((d[~on] >> j) & 1).mean():.3f}' for j, a in enumerate(ACT)))
if len(sys.argv) > 3:
    tn, hn, dn = t, h, d; td, hd, dd = load(sys.argv[3])
    en = np.loadtxt(sys.argv[2]); ed = np.loadtxt(sys.argv[4])
    print('\nintervention (normal vs deaf, same checkpoint, same random streams)')
    print(f'  births {int(en[-1, 2])} vs {int(ed[-1, 2])}; final population {int(en[-1, 1])} vs {int(ed[-1, 1])}; warriors {int(en[-1, 3])} vs {int(ed[-1, 3])}')
    for j, a in enumerate(ACT):
        print(f'  {a:9s} rate {((dn >> j) & 1).mean():.3f} vs {((dd >> j) & 1).mean():.3f}')
