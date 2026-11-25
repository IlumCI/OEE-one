#!/usr/bin/env python3
# Summarise tournament outputs (docs/TOURNEY.md). Usage: analyse.py DIR
# Per matchup (run, A step, B step, neuron override): final marker share of A and final genome share of A,
# mean and range over seeds. Same-era rows (A == B) are the drift controls.
import glob,os,re,statistics as st,sys
d=sys.argv[1];g={}
for f in sorted(glob.glob(os.path.join(d,"*.tsv"))):
  m=re.match(r"(.+?)_(\d+)_(\d+)(_n(\d+))?_s(\d+)\.tsv$",os.path.basename(f))
  if not m:continue
  rows=[l.split() for l in open(f) if l.strip()]
  if len(rows)<2:continue
  last=rows[-1];k=(m.group(1),int(m.group(2)),int(m.group(3)),m.group(5) or "-")
  g.setdefault(k,[]).append((float(last[4]),float(last[2]),int(last[0])))
print("run\tA\tB\tneurons\tseeds\tmark_A mean [min,max]\tgenome_A mean\tsteps")
for k in sorted(g):
  v=g[k];mk=[x[0] for x in v];an=[x[1] for x in v]
  print(f"{k[0]}\t{k[1]}\t{k[2]}\t{k[3]}\t{len(v)}\t{st.mean(mk):.3f} [{min(mk):.3f},{max(mk):.3f}]\t{st.mean(an):.3f}\t{min(x[2] for x in v)}")
