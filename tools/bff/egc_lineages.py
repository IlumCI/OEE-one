#!/usr/bin/env python3
"""Per-lineage composition of option-2 (egc 4) BFF checkpoints (docs/EGC.md).
For each checkpoint: programs per table (top K), and the self-replicator share per table on a random sample,
scored by bff_inspect on a mini checkpoint of the sampled programs with their own tables.
Usage: egc_lineages.py BFF_INSPECT CK [CK ...] [--top 3] [--sample 2048]"""
import argparse,struct,collections,random,subprocess,json,tempfile,os
ap=argparse.ArgumentParser();ap.add_argument('inspect');ap.add_argument('cks',nargs='+')
ap.add_argument('--top',type=int,default=3);ap.add_argument('--sample',type=int,default=2048);a=ap.parse_args()
std=b'[]+-.,<>{}'
def lab(k):
 d=[f'{chr(std[i])}={k[i]}' for i in range(10) if k[i]!=std[i]];return 'std' if not d else ' '.join(d)
for ck in a.cks:
 b=open(ck,'rb').read();h=list(struct.unpack('<6Q',b[:48]));n=h[1]
 P=b[48:48+n*64];T=b[48+n*64:48+n*64+n*10]
 by=collections.defaultdict(list)
 for i in range(n):by[T[i*10:i*10+10]].append(i)
 rows=[]
 for k,idx in sorted(by.items(),key=lambda x:-len(x[1]))[:a.top]:
  s=random.Random(1).sample(idx,min(a.sample,len(idx)));m=len(s)-len(s)%2
  if m<2:rows.append((lab(k),len(idx),None));continue
  s=s[:m];hh=h[:];hh[1]=m
  body=struct.pack('<6Q',*hh)+b''.join(P[i*64:i*64+64] for i in s)+b''.join(T[i*10:i*10+10] for i in s)
  with tempfile.NamedTemporaryFile(delete=False) as f:f.write(body);fn=f.name
  try:r=json.loads(subprocess.run([a.inspect,fn],capture_output=True,text=True).stdout)
  finally:os.unlink(fn)
  rows.append((lab(k),len(idx),r['selfreplicators']/m))
 print(f'{os.path.basename(ck)} epoch {h[4]}: '+'; '.join(f'{l} {c} ({c/n:.1%}) selfrep {s:.0%}' if s is not None else f'{l} {c}' for l,c,s in rows),flush=True)
