#!/usr/bin/env python3
# Weight health of EGG genomes (src/evo/egg.hpp; v2 files with Din > 0): per matrix and vector the share of weights at
# the int8 limits (|w| >= 127, where the integer EGGROLL update can no longer move them outward), zeros, mean |w|,
# effective rank srank (Kumar et al. 2021, arXiv:2010.14498), and, against a reference genome (default: the first
# file), the share of weights changed and the mean |change|. The input matrix is also summarised per observation group
# (column mass of World::observe entries), the head per action row.
# -drift E0 E1 E2: correlation of successive changes per parameter (selection keeps a direction; drift does not).
# Usage: tools/w/eggstat.py [-groups] REF.bin FILE.bin... | -drift E0.bin E1.bin E2.bin
import sys,struct,numpy as np
G=[('food',0,25),('agents',25,50),('energy',50,51),('age',51,52),('bias',52,53),('symbols',53,79),('material',79,104),
   ('tape',104,120),('store',120,121),('wpos',121,122),('land',122,123),('cue',123,127),('unused',127,128)]
AN=['noop','fwd','left','right','eat','repro','attack','write','make','feed']
def load(fn):
  b=open(fn,'rb').read();mg=struct.unpack('<Q',b[:8])[0]
  if mg!=0x32474745:raise SystemExit(fn+': not an EGG v2 file')
  L,D,V,sig,Din=struct.unpack('<5i',b[8:28]);step=struct.unpack('<q',b[28:36])[0];p=36;M={};Vs={}
  def mat(nm,m,n):
    nonlocal p;M[nm]=np.frombuffer(b[p:p+m*n],np.int8).reshape(m,n).astype(np.int16);p+=m*n
  def vec(nm,n):
    nonlocal p;Vs[nm]=np.frombuffer(b[p:p+n],np.int8).astype(np.int16);p+=n
  mat('inp',D,Din);mat('head',V,D)
  for l in range(L):
    for nm in ('Wf','Uf','Wh','Uh'):mat('%s%d'%(nm,l),D,D)
    mat('m1_%d'%l,4*D,D);mat('m2_%d'%l,D,4*D)
  vec('lnout',D)
  for l in range(L):
    for nm in ('ln1','ln2','bf','bh'):vec('%s%d'%(nm,l),D)
  if p!=len(b):raise SystemExit(fn+': size mismatch')
  return dict(L=L,D=D,V=V,Din=Din,step=step,M=M,V_=Vs)
def srank(X,d=0.01):
  s=np.linalg.svd(X.astype(np.float64),compute_uv=False);c=np.cumsum(s)/max(s.sum(),1e-12);return int(np.searchsorted(c,1-d)+1)
def drift(fs):  # consistency of change: corr(w1-w0, w2-w1) per parameter group; null parameters show pure drift
  E=[load(f) for f in fs];w=[dict(list(e['M'].items())+list(e['V_'].items())) for e in E];nm=list(w[0])
  def cc(x,y):
    x=x.astype(np.float64).ravel();y=y.astype(np.float64).ravel();x-=x.mean();y-=y.mean();return float((x*y).sum()/np.sqrt(max((x*x).sum()*(y*y).sum(),1e-12)))
  print('corr of successive changes (%s -> %s -> %s): > 0 consistent direction, ~0 random walk'%tuple(fs))
  for k in nm:print('  %-8s %7.4f'%(k,cc(w[1][k]-w[0][k],w[2][k]-w[1][k])))
  D=E[0]['Din'];sel=[k for k in range(32) if k not in (10,11)]
  print('  null: input entry 127 (always 0) %7.4f   head rows 10-11 (unused logits) %7.4f'%(cc(w[1]['inp'][:,D-1]-w[0]['inp'][:,D-1],w[2]['inp'][:,D-1]-w[1]['inp'][:,D-1]),
    cc(w[1]['head'][10:12]-w[0]['head'][10:12],w[2]['head'][10:12]-w[1]['head'][10:12])))
  print('  used: input entries 0-126 %7.4f   head rows of actions 0-9 %7.4f'%(cc(w[1]['inp'][:,:D-1]-w[0]['inp'][:,:D-1],w[2]['inp'][:,:D-1]-w[1]['inp'][:,:D-1]),
    cc(w[1]['head'][:10]-w[0]['head'][:10],w[2]['head'][:10]-w[1]['head'][:10])))
def main(av):
  if av and av[0]=='-drift':return drift(av[1:4])
  grp='-groups' in av;fs=[a for a in av if a!='-groups']
  if len(fs)<1:raise SystemExit('usage: eggstat.py [-groups] REF.bin FILE.bin...')
  E=[load(f) for f in fs];R=E[0]
  for f,e in zip(fs,E):
    print('== %s (step %d)'%(f,e['step']))
    print('%-8s %9s %7s %7s %7s %6s %8s %8s %8s'%('param','shape','at127','zero','mean|w|','srank','changed','mean|d|','max|w|'))
    tot=sat=ch=0
    for nm,w in list(e['M'].items())+list(e['V_'].items()):
      r=(R['M'].get(nm) if nm in R['M'] else R['V_'].get(nm));d=w-r;sh='x'.join(str(x) for x in w.shape)
      sr=srank(w) if w.ndim==2 else ''
      print('%-8s %9s %7.4f %7.4f %7.2f %6s %8.4f %8.2f %8d'%(nm,sh,(np.abs(w)>=127).mean(),(w==0).mean(),np.abs(w).mean(),sr,(d!=0).mean(),np.abs(d).mean(),np.abs(w).max()))
      tot+=w.size;sat+=(np.abs(w)>=127).sum();ch+=(d!=0).sum()
    print('all      %9d %7.4f %15s %6s %8.4f'%(tot,sat/tot,'','',ch/tot))
    if grp:
      w=e['M']['inp'];r=R['M']['inp'];print('input columns: mean |w| per observation entry (ref in brackets), mean |change|')
      print('  '+'  '.join('%s %.1f (%.1f) d%.1f'%(nm,np.abs(w[:,a:b]).mean(),np.abs(r[:,a:b]).mean(),np.abs(w[:,a:b]-r[:,a:b]).mean()) for nm,a,b in G if b<=e['Din']))
      h=e['M']['head'];hr=R['M']['head'];print('head rows: |w| row norm (ref), mean |change|')
      print('  '+'  '.join('%s %.0f (%.0f) d%.1f'%(AN[k],np.sqrt((h[k]**2).sum()),np.sqrt((hr[k]**2).sum()),np.abs(h[k]-hr[k]).mean()) for k in range(min(len(AN),e['V']))))
if __name__=='__main__':main(sys.argv[1:])
