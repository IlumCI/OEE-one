#!/usr/bin/env python3
"""W4b world-level measures (docs/WORLD.md §6, W4b protocol) from runs' log.txt, over epochs [e0, e1]:
boom time (fraction of epochs with more than `thr` agents), mean population, number of booms (maximal runs of such
epochs), mean largest-lineage share of agents during boom epochs, and peak population.
Usage: tools/w/booms.py [-e0 200] [-e1 1000] [-thr 150] RUNDIR..."""
import sys
a=sys.argv[1:];e0,e1,thr=200,1000,150
while a and a[0].startswith('-'):
  k,v=a[0],a[1];a=a[2:]
  if k=='-e0':e0=int(v)
  elif k=='-e1':e1=int(v)
  elif k=='-thr':thr=int(v)
print('run\tepochs\tboom_time\tmean_pop\tbooms\tlargest_share_in_booms\tpeak')
for d in a:
  rows=[]
  for ln in open(d+'/log.txt'):
    if ln.startswith('#'):continue
    p=ln.split('\t')
    try:e=int(p[0]);pop=int(p[1])
    except:continue
    if e<e0 or e>e1:continue
    lc=[int(x.split(':')[1]) for x in p[13].split(',') if ':' in x] if len(p)>13 else []
    rows.append((e,pop,lc))
  if not rows:print(d+'\t0');continue
  b=[r[1]>thr for r in rows];nb=sum(1 for i,x in enumerate(b) if x and (i==0 or not b[i-1]))
  sh=[max(r[2])/max(1,sum(r[2])) for r in rows if r[1]>thr and r[2]]
  print('%s\t%d\t%.3f\t%.1f\t%d\t%s\t%d'%(d,len(rows),sum(b)/len(rows),sum(r[1] for r in rows)/len(rows),nb,('%.2f'%(sum(sh)/len(sh))) if sh else '-',max(r[1] for r in rows)))
