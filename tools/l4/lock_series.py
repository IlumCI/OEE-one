#!/usr/bin/env python3
"""L4a lock series per window (docs/L4.md): mean locks, opened share, mean/max key length, turnover per
lock-window. Usage: lock_series.py RUN [RUN ...] [--window 250000]"""
import argparse,re,sys,os
ap=argparse.ArgumentParser();ap.add_argument('runs',nargs='+');ap.add_argument('--window',type=int,default=250000)
a=ap.parse_args()
pat=re.compile(rb'K(\d+)K\tlocks (\d+) opened (\d+) meanlen ([\d.]+) maxlen (\d+) born (\d+) dissolved (\d+) decayed (\d+)')
for r in a.runs:
 f=os.path.join(r,'log.txt')
 if not os.path.exists(f):print(r,'no log',file=sys.stderr);continue
 rows={};prev=None
 for line in open(f,'rb'):
  m=pat.search(line)
  if not m:continue
  t,n,o,ml,mx,b,d,dc=m.groups();t=int(t);w=t//a.window;n=int(n);ev=int(b)+int(d)+int(dc)
  s=rows.setdefault(w,[0,0,0,0.0,0,None,None,0])
  s[0]+=1;s[1]+=n;s[2]+=int(o);s[3]+=float(ml);s[4]=max(s[4],int(mx))
  if s[5] is None:s[5]=(t,ev)
  s[6]=(t,ev)
 print(f'# {r}');print('window_start locks opened_share meanlen maxlen turnover_per_lock_window')
 for w in sorted(rows):
  s=rows[w];k=s[0];locks=s[1]/k;dt=(s[6][0]-s[5][0])/1000 or 1
  print(w*a.window,f'{locks:.1f}',f'{(s[2]/s[1] if s[1] else 0):.3f}',f'{s[3]/k:.3f}',s[4],f'{(s[6][1]-s[5][1])/dt/max(locks,1):.3f}')
