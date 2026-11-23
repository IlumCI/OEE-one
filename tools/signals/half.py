#!/usr/bin/env python3
"""Per-agent causal test from `signals CK STEPS 2` output (random half deaf by id, same world).
Compares deaf vs hearing agents among those that have input neurons; tracks the hearing share over time."""
import sys
import numpy as np
ACT = ['reproduce', 'attack', 'move', 'turn', 'bond']
a = np.loadtxt(sys.argv[1], dtype=np.int64); t, h, d, deaf, hasin = a.T
m = hasin == 1
print(f'agent-steps {len(t)}; with inputs {m.mean()*100:.1f}%')
for j, n in enumerate(ACT):
    x0 = ((d[m & (deaf == 0)] >> j) & 1); x1 = ((d[m & (deaf == 1)] >> j) & 1)
    se = np.sqrt(x0.var() / len(x0) + x1.var() / len(x1))
    print(f'  {n:9s} hearing {x0.mean():.3f}  deaf {x1.mean():.3f}  diff {x0.mean()-x1.mean():+.3f} ({(x0.mean()-x1.mean())/se:+.1f} se)')
ts = np.unique(t)
def share(tt, sel): s = sel & (t == tt); return (deaf[s] == 0).mean()
first = ts[:50]; last = ts[-50:]
f0 = np.mean([share(x, m) for x in first]); f1 = np.mean([share(x, m) for x in last])
g0 = np.mean([share(x, ~m) for x in first]); g1 = np.mean([share(x, ~m) for x in last])
print(f'hearing share among agents with inputs: first 50 steps {f0:.3f} -> last 50 steps {f1:.3f}')
print(f'same label among agents without inputs (label has no effect, drift control): {g0:.3f} -> {g1:.3f}')
