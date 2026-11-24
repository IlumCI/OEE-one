#!/usr/bin/env python3
# Causal effect of lifetime learning (docs/PLASTIC.md): replay a checkpoint with plasticity on vs frozen,
# R reseeded replicates each (paired by seed), and compare births, population and behaviour.
# Usage: freeze.py SIGNALS_BIN CHECKPOINT [steps=500] [reps=12]
import subprocess,sys,statistics as st,math
b,ck=sys.argv[1],sys.argv[2];T=int(sys.argv[3]) if len(sys.argv)>3 else 500;R=int(sys.argv[4]) if len(sys.argv)>4 else 12
def run(fz,s):
  p=subprocess.run(["nice",b,ck,str(T),"0",str(fz),str(s)],capture_output=True,text=True)
  last=p.stderr.strip().splitlines()[-1].split();rows=[l.split("\t") for l in p.stdout.splitlines()]
  did=[int(r[2]) for r in rows];n=len(did) or 1
  fr=[sum((d>>k)&1 for d in did)/n for k in range(6)]
  return int(last[1]),int(last[2]),fr
D={0:[],1:[]}
for s in range(1,R+1):
  for fz in (0,1):D[fz].append(run(fz,s))
def cmp(name,f):
  a=[f(x) for x in D[0]];z=[f(x) for x in D[1]];d=[x-y for x,y in zip(a,z)]
  m=st.mean(d);se=st.stdev(d)/math.sqrt(len(d)) if len(d)>1 else 0
  print(f"{name:10s} learning {st.mean(a):10.3f} frozen {st.mean(z):10.3f} diff {m:+.3f} se {se:.3f} t {m/se if se else 0:+.2f} wins {sum(x>0 for x in d)}/{len(d)}")
cmp("births",lambda x:x[1]);cmp("pop",lambda x:x[0])
for k,nm in enumerate(["reproduce","attack","move","turn","bond","signal"]):cmp(nm,lambda x,k=k:x[2][k])
