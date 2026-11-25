#!/usr/bin/env python3
"""Summarise W1 era progress across runs (docs/WORLD.md §6, W1 protocol).
Usage: w1_report.py RUNDIR... (each with era_250.tsv from tools/w/w1_eval.sh and log.txt)
Per run: epochs, share of epochs with one lineage, splits, extinctions, and the era comparisons (e vs e-250, excluding
e = 250, whose opponent is the random initial brain): how many favour the later champion with t > 3 and with t < -3,
for the protocol statistic (integrated share, all ticks) and the post-founder share.
"""
import os,sys
def log_stats(d):
    n=one=0;sp=ex=0
    for l in open(os.path.join(d,'log.txt')):
        if l.startswith('#'):continue
        x=l.split('\t');n+=1;one+=x[10]=='1';sp=int(x[11]);ex=int(x[12])
    return n,(one/n if n else 0),sp,ex
print('run\tepochs\tone_lineage\tsplits\textinct\tcomparisons\tall:t>3\tall:t<-3\tpost:t>3\tpost:t<-3\tmean_all\tmean_post')
for d in sys.argv[1:]:
    f=os.path.join(d,'era_250.tsv')
    if not os.path.exists(f):continue
    rows=[l.rstrip('\n').split('\t') for l in open(f) if l[0].isdigit()]
    rows=[r for r in rows if int(r[1])>0]
    n,one,sp,ex=log_stats(d)
    ta=[float(r[4]) for r in rows];tl=[float(r[9]) for r in rows];ma=[float(r[2]) for r in rows];ml=[float(r[7]) for r in rows]
    k=len(rows)
    print(f'{os.path.basename(d)}\t{n}\t{one:.2f}\t{sp}\t{ex}\t{k}\t{sum(t>3 for t in ta)}\t{sum(t<-3 for t in ta)}\t{sum(t>3 for t in tl)}\t{sum(t<-3 for t in tl)}\t'
          f'{(sum(ma)/k if k else 0):.3f}\t{(sum(ml)/k if k else 0):.3f}')
