#!/usr/bin/env python3
"""Four structurally different design stills from one netdump creature (+ optional l4 world frame).

Usage: designs.py CREATURE.json WORLD.bin FRAME ACT OUTDIR -> OUTDIR/{lecture,raster,worldmind,words}.html
Layered layout: sense inputs left, hidden by forward distance from inputs, action outputs right (grouped by action).
"""
import json,os,sys,struct
from collections import defaultdict,deque
import numpy as np

ACT=[('01','reproduce'),('100','attack'),('101','turn left'),('110','turn right'),('111','move'),('000','bond'),('001','unbond')]
def act_of(c):
  for k,v in ACT:
    if c.startswith(k):return v
  return None
def listens(c):  # input: first bit = side (1 right), rest = prefix of neighbour outputs it responds to
  side='right' if c[:1]=='1' else 'left';rest=c[1:]
  hits=[v for k,v in ACT if k.startswith(rest)]
  what='any signal' if rest=='' else (hits[0] if len(hits)==1 else ' / '.join(hits) if hits else 'signal '+rest)
  return side,what

def layered(d):
  f=d['dev'][-1];N=len(f['n']);ix={n[0]:i for i,n in enumerate(f['n'])}
  E=[(ix[a],ix[b],e) for a,b,e in f['l'] if a in ix and b in ix]
  out=defaultdict(list);inn=defaultdict(list)
  for a,b,e in E:out[a].append(b);inn[b].append(a)
  isin=[bool(n[2]) for n in f['n']];isout=[bool(n[3]) and not n[2] for n in f['n']]
  dep=[None]*N;q=deque(i for i in range(N) if isin[i])
  for i in q:dep[i]=0
  while q:
    u=q.popleft()
    for v in out[u]:
      if dep[v] is None:dep[v]=dep[u]+1;q.append(v)
  hid=[i for i in range(N) if not isin[i] and not isout[i]]
  K=max([dep[i] for i in hid if dep[i] is not None]+[1])
  col=[0]*N
  for i in range(N):
    if isin[i]:col[i]=0
    elif isout[i]:col[i]=K+1
    else:col[i]=min(K,dep[i]) if dep[i] is not None else max(1,K//2+1)
  cols=defaultdict(list)
  for i in range(N):cols[col[i]].append(i)
  y={};C=max(cols)
  for c in range(C+1):
    m=cols[c]
    if c==C:m.sort(key=lambda i:(ACT.index(next((x for x in ACT if x[1]==act_of(f['n'][i][1])),('', '')) ) if act_of(f['n'][i][1]) else 99,f['n'][i][1]))
    elif c>0:m.sort(key=lambda i:np.mean([y[p] for p in inn[i] if p in y]) if any(p in y for p in inn[i]) else .5)
    for k,i in enumerate(m):y[i]=(k+.5)/len(m)
  return f,E,col,y,C,isin,isout

def page(body,fonts,bg):
  return f'''<!doctype html><html><head><meta charset="utf-8"><link rel="stylesheet" href="https://fonts.googleapis.com/css2?{fonts}&display=block">
<style>html,body{{margin:0;width:1920px;height:1080px;overflow:hidden;background:{bg}}}svg{{display:block}}</style></head><body><svg width="1920" height="1080">{body}</svg></body></html>'''

def acts(d,t):a=np.clip(np.array(d['act'][min(t,len(d['act'])-1)],float)/1023,0,1);return a

def lecture(d,t):
  f,E,col,y,C,isin,isout=layered(d);a=acts(d,t)
  X0,X1,Y0,Y1=470,1450,120,960;P={i:(X0+(X1-X0)*col[i]/C,Y0+(Y1-Y0)*y[i]) for i in col.__iter__() if False} if False else {i:(X0+(X1-X0)*col[i]/C,Y0+(Y1-Y0)*y[i]) for i in range(len(col))}
  s=['<rect width="1920" height="1080" fill="#FFFFFF"/>']
  for a_,b,e in sorted(E,key=lambda x:a[x[0]]):
    (x0,y0),(x1,y1)=P[a_],P[b];on=a[a_]>.4
    if x1>x0:pth=f'M{x0:.1f},{y0:.1f} C{(x0+x1)/2:.1f},{y0:.1f} {(x0+x1)/2:.1f},{y1:.1f} {x1:.1f},{y1:.1f}'
    else:h=60+abs(x0-x1)*.25;pth=f'M{x0:.1f},{y0:.1f} C{x0+80:.1f},{y0-h:.1f} {x1-80:.1f},{y1-h:.1f} {x1:.1f},{y1:.1f}'
    col_='#0A0A0A' if (on and e) else '#E2421F' if on else '#D9D9D9'
    s.append(f'<path d="{pth}" fill="none" stroke="{col_}" stroke-width="{1.4 if on else 0.6}" stroke-opacity="{0.55 if on else 0.7}"/>')
  for i,(x,yy) in P.items():
    r=11 if (isin[i] or isout[i]) else 7
    s.append(f'<circle cx="{x:.1f}" cy="{yy:.1f}" r="{r}" fill="#FFFFFF" stroke="#0A0A0A" stroke-width="1.5"/><circle cx="{x:.1f}" cy="{yy:.1f}" r="{r*a[i]:.1f}" fill="#0A0A0A"/>')
  for i in range(len(col)):
    x,yy=P[i]
    if isin[i]:sd,w=listens(f['n'][i][1]);s.append(f'<text x="{x-24}" y="{yy+7}" text-anchor="end" font-family="Public Sans" font-size="20" fill="#0A0A0A">{sd} · {w}</text>')
    if isout[i]:s.append(f'<text x="{x+24}" y="{yy+7}" font-family="Public Sans" font-size="20" fill="{"#0A0A0A" if act_of(f["n"][i][1]) else "#A0A0A0"}">{act_of(f["n"][i][1]) or "unwired"}</text>')
  s.append(f'<text x="{X0}" y="1030" font-family="Public Sans" font-size="22" fill="#6B6B6B">sees</text><text x="{(X0+X1)/2}" y="1030" text-anchor="middle" font-family="Public Sans" font-size="22" fill="#6B6B6B">thinks</text><text x="{X1}" y="1030" text-anchor="end" font-family="Public Sans" font-size="22" fill="#6B6B6B">acts</text>')
  return page(''.join(s),'family=Public+Sans:wght@400;600','#FFFFFF')

def raster(d,t):
  f,E,col,y,C,isin,isout=layered(d);A=np.clip(np.array(d['act'],float)/1023,0,1);T=len(A)
  order=sorted(range(len(col)),key=lambda i:(col[i],y[i]));grp=lambda i:'sense' if isin[i] else ('act' if isout[i] else 'think')
  s=['<rect width="1920" height="1080" fill="#D9DCDF"/>']
  x0,x1,y0,y1=360,1840,80,1000;cw=(x1-x0)/T;rh=(y1-y0)/len(order)
  for r,i in enumerate(order):
    yy=y0+r*rh
    for k in range(T):
      v=A[k][i] if i<A.shape[1] else 0
      if v>.05:s.append(f'<rect x="{x0+k*cw+1:.1f}" y="{yy+1:.1f}" width="{cw-2:.1f}" height="{max(rh-2,1):.1f}" fill="#111" opacity="{.15+.85*v:.2f}"/>')
  prev=None
  for r,i in enumerate(order):
    g=grp(i)
    if g!=prev:
      yy=y0+r*rh;s.append(f'<line x1="300" y1="{yy:.1f}" x2="{x1}" y2="{yy:.1f}" stroke="#111" stroke-width="1"/><text x="60" y="{yy+30:.1f}" font-family="Martian Mono" font-size="22" fill="#111">{g}</text>');prev=g
    if g=='act':
      lab=act_of(f['n'][i][1]) or '—';s.append(f'<text x="330" y="{y0+r*rh+rh*.75:.1f}" text-anchor="end" font-family="Martian Mono" font-size="{min(rh*.8,14):.1f}" fill="#444">{lab}</text>')
  px=x0+(t+.5)*cw;s.append(f'<line x1="{px:.1f}" y1="{y0-20}" x2="{px:.1f}" y2="{y1+10}" stroke="#E2421F" stroke-width="3"/>')
  s.append(f'<text x="{x0}" y="1050" font-family="Martian Mono" font-size="20" fill="#111">step {format(d["time"]+1,",")}</text><text x="{x1}" y="1050" text-anchor="end" font-family="Martian Mono" font-size="20" fill="#111">step {format(d["time"]+T,",")}</text>')
  return page(''.join(s),'family=Martian+Mono:wght@400','#D9DCDF')

def worldmind(d,wf,t):
  f,E,col,y,C,isin,isout=layered(d);a=acts(d,t);tt,L,ag,ed,lk=wf
  hx,hy=d['x'],d['y'];R=3;cs=88;ox,oy=110,540-cs*3.5
  s=['<rect width="1920" height="1080" fill="#E7EBEE"/>']
  pal=['#C9A227','#2F7FB5','#D07A2C','#5E5E5E','#3E9E78','#7B5BB5']
  for dy in range(-R,R+1):
    for dx in range(-R,R+1):
      x,yy=ox+(dx+R)*cs,oy+(dy+R)*cs;cx,cy=(hx+dx)%L,(hy+dy)%L
      s.append(f'<rect x="{x+2}" y="{yy+2}" width="{cs-4}" height="{cs-4}" fill="{"#D3D9DE" if lk[cy,cx] else "#F4F6F7"}"/>')
  pos={(int(p[0]),int(p[1])):p for p in ag}
  for dy in range(-R,R+1):
    for dx in range(-R,R+1):
      p=pos.get(((hx+dx)%L,(hy+dy)%L))
      if p is None:continue
      x,yy=ox+(dx+R)*cs+cs/2,oy+(dy+R)*cs+cs/2;me=dx==0 and dy==0
      s.append(f'<circle cx="{x}" cy="{yy}" r="{30 if me else 20}" fill="{"#111" if me else pal[p[2]]}"/>')
  mx=ox+3.5*cs;my=oy+3.5*cs
  X0,X1,Y0,Y1=1000,1700,130,950;P={i:(X0+(X1-X0)*col[i]/C,Y0+(Y1-Y0)*y[i]) for i in range(len(col))}
  for i in range(len(col)):
    if isin[i] and a[i]>.05:x,yy=P[i];s.append(f'<path d="M{mx+30},{my} C{mx+300},{my} {x-300},{yy} {x},{yy}" fill="none" stroke="#111" stroke-width="{1+3*a[i]:.1f}" stroke-opacity="0.5"/>')
  for a_,b,e in E:
    (x0,y0),(x1,y1)=P[a_],P[b];on=a[a_]>.4
    if on:s.append(f'<line x1="{x0:.1f}" y1="{y0:.1f}" x2="{x1:.1f}" y2="{y1:.1f}" stroke="{"#111" if e else "#E2421F"}" stroke-width="1" stroke-opacity=".35"/>')
  for i,(x,yy) in P.items():s.append(f'<circle cx="{x:.1f}" cy="{yy:.1f}" r="{3+6*a[i]:.1f}" fill="#111"/>')
  best={}
  for i in range(len(col)):
    if isout[i] and act_of(f['n'][i][1]):k=act_of(f['n'][i][1]);best[k]=best.get(k,0)+a[i]
  y_=170
  for k,v in sorted(best.items(),key=lambda kv:-kv[1]):
    s.append(f'<text x="1760" y="{y_}" font-family="Instrument Sans" font-size="30" fill="#111" opacity="{.25+.75*min(1,v):.2f}">{k}</text>');y_+=56
  s.append(f'<text x="{ox}" y="1030" font-family="Instrument Sans" font-size="24" fill="#111">its neighbourhood, step {format(tt,",")}</text><text x="{X0}" y="1030" font-family="Instrument Sans" font-size="24" fill="#111">its brain, same step</text>')
  return page(''.join(s),'family=Instrument+Sans:wght@400;600','#E7EBEE')

def words(d,t):
  f,E,col,y,C,isin,isout=layered(d);a=acts(d,t)
  X0,X1,Y0,Y1=200,1720,70,1010;P={i:(X0+(X1-X0)*col[i]/C,Y0+(Y1-Y0)*y[i]) for i in range(len(col))}
  s=['<rect width="1920" height="1080" fill="#101010"/>']
  for a_,b,e in E:
    (x0,y0),(x1,y1)=P[a_],P[b]
    if a[a_]>.4:s.append(f'<path d="M{x0+30:.1f},{y0:.1f} C{(x0+x1)/2:.1f},{y0:.1f} {(x0+x1)/2:.1f},{y1:.1f} {x1-30:.1f},{y1:.1f}" fill="none" stroke="#FFFFFF" stroke-width="0.7" stroke-opacity="0.18"/>')
  for i,(x,yy) in P.items():
    c=f['n'][i][1];s.append(f'<text x="{x:.1f}" y="{yy+6:.1f}" text-anchor="middle" font-family="DM Mono" font-size="{16 if not(isin[i] or isout[i]) else 18}" fill="#FFFFFF" opacity="{.18+.82*a[i]:.2f}">{c}</text>')
  return page(''.join(s),'family=DM+Mono','#101010')

def world_frame(b,k):
  o=0;i=0
  while o<len(b):
    t,L,n=struct.unpack_from('<IHH',b,o);o+=8;a=np.frombuffer(b,np.uint8,4*n,o).reshape(n,4).astype(int);o+=4*n
    m,=struct.unpack_from('<H',b,o);o+=2;e=np.frombuffer(b,np.uint16,2*m,o).reshape(m,2);o+=4*m
    lk=np.frombuffer(b,np.uint8,L*L,o).reshape(L,L);o+=L*L
    if i==k:return t,L,a,e,lk
    i+=1

if __name__=='__main__':
  d=json.load(open(sys.argv[1]));wf=world_frame(open(sys.argv[2],'rb').read(),int(sys.argv[3]));t=int(sys.argv[4]);out=sys.argv[5];os.makedirs(out,exist_ok=True)
  for n,h in (('lecture',lecture(d,t)),('raster',raster(d,t)),('worldmind',worldmind(d,wf,t)),('words',words(d,t))):open(f'{out}/{n}.html','w').write(h)
