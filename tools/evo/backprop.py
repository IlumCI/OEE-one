#!/usr/bin/env python3
# Backprop baseline for EGG (docs/EVO.md): byte-level pre-LN Transformer, same width and depth as EGG
# (6L-256D, about 4.9M parameters), trained by gradient descent on the same byte streams as oee_egg.
# Numpy only, hand-written backward pass (checked by --gradcheck).
# Usage: backprop.py DATA [--opt sgd|adam] [--lr X] [--steps N] [--seqs 16] [--tokens 100] [--ctx 256]
#        [--eval-every 50] [--eval-streams 32] [--eval-tokens 256] [--seed 1] [--log FILE]
#        backprop.py --gradcheck
# Each step every sequence q in 0..seqs-1 advances by `tokens` bytes, at the same positions as oee_egg
# (q * (train_len / seqs) + step * tokens). The loss covers those new bytes only; the window also includes up to
# ctx - tokens preceding bytes as context (more context than EGG's carried state is not needed to be fair:
# this only favours the baseline). Evaluation uses the same validation windows as oee_egg (fresh context).
# Log: step bytes_seen train_bpb valid_bpb sec flops_total
import argparse, math, sys, time
import numpy as np

def gelu(x):
    c = math.sqrt(2/math.pi); t = np.tanh(c*(x+0.044715*x**3)); return 0.5*x*(1+t), t
def gelu_back(x, t, g):
    c = math.sqrt(2/math.pi); dt = (1-t*t)*c*(1+3*0.044715*x*x); return g*(0.5*(1+t)+0.5*x*dt)

class Model:
    def __init__(s, V=256, C=256, L=6, H=4, T=256, seed=1, dtype=np.float32):
        r = np.random.default_rng(seed); s.V,s.C,s.L,s.H,s.T = V,C,L,H,T; s.dt = dtype
        n = lambda *sh, sc=0.02: (r.standard_normal(sh)*sc).astype(dtype)
        s.p = {'emb': n(V,C), 'pos': n(T,C), 'gf': np.ones(C,dtype), 'bf': np.zeros(C,dtype), 'out': n(C,V)}
        for l in range(L):
            for k in ('g1','g2'): s.p[f'{k}{l}'] = np.ones(C,dtype)
            for k in ('b1','b2','c2'): s.p[f'{k}{l}'] = np.zeros(C,dtype)
            s.p[f'c1{l}'] = np.zeros(4*C,dtype)
            for k in ('Wq','Wk','Wv'): s.p[f'{k}{l}'] = n(C,C)
            s.p[f'Wo{l}'] = n(C,C,sc=0.02/math.sqrt(2*L)); s.p[f'W1{l}'] = n(C,4*C); s.p[f'W2{l}'] = n(4*C,C,sc=0.02/math.sqrt(2*L))
    def nparams(s): return sum(v.size for v in s.p.values())
    @staticmethod
    def ln(x, g, b):
        mu = x.mean(-1, keepdims=True); xc = x-mu; var = (xc*xc).mean(-1, keepdims=True); rs = 1/np.sqrt(var+1e-5); xh = xc*rs
        return xh*g+b, (xh, rs)
    @staticmethod
    def ln_back(dy, g, cache):
        xh, rs = cache; dg = (dy*xh).reshape(-1, xh.shape[-1]).sum(0); db = dy.reshape(-1, dy.shape[-1]).sum(0)
        dxh = dy*g; C = xh.shape[-1]
        dx = rs*(dxh - dxh.mean(-1, keepdims=True) - xh*(dxh*xh).mean(-1, keepdims=True)); return dx, dg, db
    def loss_grad(s, tok, w, need_grad=True):
        # tok (B, T+1) ints, w (B, T) weights of each prediction in the loss; returns sum of -log2 p * w, grads
        p = s.p; B, T1 = tok.shape; T = T1-1; C, H = s.C, s.H; hd = C//H
        x = p['emb'][tok[:, :T]] + p['pos'][:T]
        caches = []; mask = np.triu(np.full((T,T), -1e9, s.dt), 1)
        for l in range(s.L):
            h, c1 = s.ln(x, p[f'g1{l}'], p[f'b1{l}'])
            q = (h@p[f'Wq{l}']).reshape(B,T,H,hd).transpose(0,2,1,3); k = (h@p[f'Wk{l}']).reshape(B,T,H,hd).transpose(0,2,1,3)
            v = (h@p[f'Wv{l}']).reshape(B,T,H,hd).transpose(0,2,1,3)
            a = q@k.transpose(0,1,3,2)/math.sqrt(hd)+mask; a = a-a.max(-1, keepdims=True); a = np.exp(a); a /= a.sum(-1, keepdims=True)
            o = (a@v).transpose(0,2,1,3).reshape(B,T,C); x2 = x+o@p[f'Wo{l}']
            h2, c2 = s.ln(x2, p[f'g2{l}'], p[f'b2{l}']); z = h2@p[f'W1{l}']+p[f'c1{l}']; gz, tz = gelu(z)
            x3 = x2+gz@p[f'W2{l}']+p[f'c2{l}']
            caches.append((x, h, c1, q, k, v, a, o, x2, h2, c2, z, gz, tz)); x = x3
        hf, cf = s.ln(x, p['gf'], p['bf']); lg = hf@p['out']
        lg = lg-lg.max(-1, keepdims=True); e = np.exp(lg); Z = e.sum(-1, keepdims=True); lp = lg-np.log(Z)
        tgt = tok[:, 1:]; nll = -np.take_along_axis(lp, tgt[..., None], -1)[..., 0]
        loss = float((nll*w).sum()/math.log(2))
        if not need_grad: return loss, None
        g = {}; dlg = e/Z; np.put_along_axis(dlg, tgt[..., None], np.take_along_axis(dlg, tgt[..., None], -1)-1, -1); dlg *= w[..., None]/math.log(2)
        g['out'] = hf.reshape(-1,C).T@dlg.reshape(-1,s.V); dhf = dlg@p['out'].T
        dx, g['gf'], g['bf'] = s.ln_back(dhf, p['gf'], cf)
        for l in reversed(range(s.L)):
            x0, h, c1, q, k, v, a, o, x2, h2, c2, z, gz, tz = caches[l]
            g[f'c2{l}'] = dx.reshape(-1,C).sum(0); g[f'W2{l}'] = gz.reshape(-1,4*C).T@dx.reshape(-1,C)
            dz = gelu_back(z, tz, dx@p[f'W2{l}'].T); g[f'c1{l}'] = dz.reshape(-1,4*C).sum(0); g[f'W1{l}'] = h2.reshape(-1,C).T@dz.reshape(-1,4*C)
            dh2 = dz@p[f'W1{l}'].T; dx2a, g[f'g2{l}'], g[f'b2{l}'] = s.ln_back(dh2, p[f'g2{l}'], c2); dx2 = dx+dx2a
            g[f'Wo{l}'] = o.reshape(-1,C).T@dx2.reshape(-1,C); do = (dx2@p[f'Wo{l}'].T).reshape(B,T,H,hd).transpose(0,2,1,3)
            da = do@v.transpose(0,1,3,2); dv = a.transpose(0,1,3,2)@do
            ds = a*(da-(da*a).sum(-1, keepdims=True))/math.sqrt(hd); dq = ds@k; dk = ds.transpose(0,1,3,2)@q
            back = lambda t: t.transpose(0,2,1,3).reshape(B,T,C)
            dq, dk, dv = back(dq), back(dk), back(dv); hh = h.reshape(-1,C)
            g[f'Wq{l}'] = hh.T@dq.reshape(-1,C); g[f'Wk{l}'] = hh.T@dk.reshape(-1,C); g[f'Wv{l}'] = hh.T@dv.reshape(-1,C)
            dh = dq@p[f'Wq{l}'].T+dk@p[f'Wk{l}'].T+dv@p[f'Wv{l}'].T
            dx0, g[f'g1{l}'], g[f'b1{l}'] = s.ln_back(dh, p[f'g1{l}'], c1); dx = dx2+dx0
        g['pos'] = np.zeros_like(p['pos']); g['pos'][:T] = dx.sum(0)
        g['emb'] = np.zeros_like(p['emb']); np.add.at(g['emb'], tok[:, :T].reshape(-1), dx.reshape(-1,C))
        return loss, g
    def flops(s, B, T):  # forward+backward, matmuls only: 6 * params_in_matmuls * tokens + attention
        C, L, V = s.C, s.L, s.V; mm = L*(4*C*C+8*C*C)+C*V; return 6*mm*B*T+L*(12*B*T*T*C)

def gradcheck():
    m = Model(V=11, C=8, L=2, H=2, T=6, seed=3, dtype=np.float64); r = np.random.default_rng(0)
    tok = r.integers(0, 11, (2, 7)); w = np.ones((2, 6)); w[:, :2] = 0
    _, g = m.loss_grad(tok, w); worst = 0
    for k, v in m.p.items():
        for _ in range(4):
            i = tuple(r.integers(0, d) for d in v.shape); old = v[i]; e = 1e-6
            v[i] = old+e; lp, _ = m.loss_grad(tok, w, False); v[i] = old-e; lm, _ = m.loss_grad(tok, w, False); v[i] = old
            num = (lp-lm)/(2*e); d = abs(num-g[k][i])
            if d > 1e-8: worst = max(worst, d/max(1e-8, abs(num)+abs(g[k][i])))  # tiny gradients: finite differences are roundoff-limited
    print(f"gradcheck worst relative error (|diff| > 1e-8) {worst:.2e}", "ok" if worst < 1e-5 else "FAIL"); return worst < 1e-5

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('data', nargs='?'); ap.add_argument('--gradcheck', action='store_true')
    ap.add_argument('--opt', default='sgd'); ap.add_argument('--lr', type=float, default=None); ap.add_argument('--steps', type=int, default=1000)
    ap.add_argument('--seqs', type=int, default=16); ap.add_argument('--tokens', type=int, default=100); ap.add_argument('--ctx', type=int, default=256)
    ap.add_argument('--eval-every', type=int, default=50); ap.add_argument('--eval-streams', type=int, default=32); ap.add_argument('--eval-tokens', type=int, default=256)
    ap.add_argument('--seed', type=int, default=1); ap.add_argument('--log'); a = ap.parse_args()
    if a.gradcheck: sys.exit(0 if gradcheck() else 1)
    data = np.fromfile(a.data, dtype=np.uint8); N = len(data); ntr = N//10*9; nva = N//20; tr = data[:ntr]; va = data[ntr:ntr+nva]
    m = Model(T=max(a.ctx, a.eval_tokens), seed=a.seed); lr = a.lr if a.lr is not None else (0.5 if a.opt == 'sgd' else 1e-3)
    lg = open(a.log, 'w') if a.log else sys.stdout
    print(f"# transformer L{m.L} C{m.C} H{m.H} params {m.nparams()} opt {a.opt} lr {lr} seqs {a.seqs} tokens {a.tokens} ctx {a.ctx} seed {a.seed}", file=lg, flush=True)
    mom = {k: np.zeros_like(v) for k, v in m.p.items()}; vel = {k: np.zeros_like(v) for k, v in m.p.items()}
    flops = 0.0; T, ctx = a.tokens, a.ctx
    def evaluate():
        E, Te = a.eval_streams, a.eval_tokens; tok = np.stack([va[s*((nva-Te-1)//E): s*((nva-Te-1)//E)+Te+1] for s in range(E)]).astype(np.int64)
        tot = 0.0
        for i in range(0, E, 8): l, _ = m.loss_grad(tok[i:i+8], np.ones((len(tok[i:i+8]), Te), np.float32), False); tot += l
        return tot/(E*Te)
    for st in range(a.steps):
        t0 = time.time(); rows = []; w = np.zeros((a.seqs, ctx), np.float32)
        for q in range(a.seqs):
            p0 = (q*(ntr//a.seqs)+st*T) % (ntr-T-1); s0 = max(0, p0-(ctx-T)); win = tr[s0:p0+T+1]
            pad = ctx+1-len(win); rows.append(np.concatenate([np.zeros(pad, np.uint8), win])); w[q, ctx-T:] = 1
        tok = np.stack(rows).astype(np.int64); loss, g = m.loss_grad(tok, w); flops += m.flops(a.seqs, ctx)
        for k in m.p:
            gk = g[k]/(a.seqs*T)
            if a.opt == 'sgd': m.p[k] -= (lr*gk).astype(m.dt)
            else:
                mom[k] = 0.9*mom[k]+0.1*gk; vel[k] = 0.999*vel[k]+0.001*gk*gk
                mh = mom[k]/(1-0.9**(st+1)); vh = vel[k]/(1-0.999**(st+1)); m.p[k] -= (lr*mh/(np.sqrt(vh)+1e-8)).astype(m.dt)
        ev = a.eval_every > 0 and ((st+1) % a.eval_every == 0 or st+1 == a.steps)
        print(f"{st+1}\t{(st+1)*a.seqs*T}\t{loss/(a.seqs*T):.4f}\t{evaluate():.4f}" if ev else f"{st+1}\t{(st+1)*a.seqs*T}\t{loss/(a.seqs*T):.4f}\t-", end='', file=lg)
        print(f"\t{time.time()-t0:.2f}\t{flops:.4g}", file=lg, flush=True)

if __name__ == '__main__': main()
