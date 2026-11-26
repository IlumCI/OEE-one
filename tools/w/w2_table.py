#!/usr/bin/env python3
"""W2a final table over seeds (docs/WORLD.md §9): post-founder share (t) of the ablated copy at epoch 3000, and era
collapses (< 0.25) and mean era share, from docs/results/w2a/final_*. Usage: tools/w/w2_table.py [results dir]"""
import re,sys,glob,os
d=sys.argv[1] if len(sys.argv)>1 else 'docs/results/w2a'
runs={}  # (seed,arm) -> {(ablation,arena):(lshare,t)}, era list
era={}
for f in sorted(glob.glob(os.path.join(d,'final_*3000.txt'))):
  cur=None
  for ln in open(f):
    m=re.match(r'== (w2_(sig|nosig)_s(\d)) epoch 3000',ln)
    if m:cur=(int(m.group(3)),m.group(2));runs.setdefault(cur,{});continue
    p=ln.split('\t')
    if cur and len(p)==8 and p[0] in('amnesia','shuffle','deaf'):runs[cur][(p[0],p[1])]=(float(p[5]),float(p[7]))
    m=re.match(r'(?:(w2_(sig|nosig)_s(\d)): |era: )(.*)',ln.strip())
    if m:
      k=(int(m.group(3)),m.group(2)) if m.group(1) else cur
      era[k]=[float(x.split(':')[1]) for x in m.group(4).split()]
cols=[('amnesia','seasons'),('shuffle','seasons'),('deaf','seasons'),('amnesia','static'),('shuffle','static'),('deaf','static')]
print('seed\tarm\t'+'\t'.join(a+'/'+b for a,b in cols)+'\tcollapses\tmean_era')
for k in sorted(runs):
  r=runs[k];e=era.get(k,[])
  c=[('%.3f (%.1f)'%r[x]) if x in r else '-' for x in cols]
  print('%d\t%s\t%s\t%s\t%s'%(k[0],k[1],'\t'.join(c),sum(v<0.25 for v in e) if e else '-',('%.2f'%(sum(e)/len(e))) if e else '-'))
