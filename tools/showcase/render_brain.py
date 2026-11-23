#!/usr/bin/env python3
"""Render a creature's evolved network from `netdump` JSON.

Usage: render_brain.py HERO.json OUT.png [--step K] [--act T] [--code]
Layout: Fruchterman-Reingold on the final developed network (seeded, deterministic); earlier development
steps reuse those positions so growth reads as the same brain filling in. Excitatory links warm, inhibitory
cool; node glow = live activity at world step T (if given); --code draws the production rules on the left.
"""
import argparse,json
import numpy as np
from PIL import Image,ImageDraw,ImageFont
from scipy.ndimage import gaussian_filter

def layout(nn,links,seed=7,it=400):
  rng=np.random.default_rng(seed);p=rng.normal(size=(nn,2));k=1/np.sqrt(nn)
  E=np.array([[a,b] for a,b,_ in links if a!=b]) if links else np.zeros((0,2),int)
  t=.1
  for i in range(it):
    d=p[:,None]-p[None];r=np.sqrt((d**2).sum(-1))+1e-9
    f=(d/r[...,None]*(k*k/r)[...,None]).sum(1)
    if len(E):
      dv=p[E[:,0]]-p[E[:,1]];dl=np.sqrt((dv**2).sum(-1))+1e-9;fa=dv/dl[:,None]*(dl**2/k)[:,None]
      np.add.at(f,E[:,0],-fa);np.add.at(f,E[:,1],fa)
    f-=p*.05
    ln=np.sqrt((f**2).sum(-1))+1e-9;p+=f/ln[:,None]*np.minimum(ln,t)[:,None];t*=.99
  p-=p.mean(0);p/=np.abs(p).max()+1e-9
  return p

def tonemap(x):return 1-np.exp(-x)

def render(d,step,act,code,W=1920,H=1080):
  fin=d['dev'][-1];ids=[n[0] for n in fin['n']];ix={v:i for i,v in enumerate(ids)}
  P=layout(len(ids),[(ix[a],ix[b],e) for a,b,e in fin['l']])
  cx,cy,R=(W*0.62 if code else W/2),H/2,H*0.42
  xy=np.c_[cx+P[:,0]*R,cy+P[:,1]*R]
  st=d['dev'][step];present={n[0] for n in st['n']}
  img=np.zeros((H,W,3))
  warm=np.array([1,.62,.25]);cool=np.array([.35,.6,1])
  acc=np.zeros((H,W,3))
  for a,b,e in st['l']:
    if a not in ix or b not in ix:continue
    (x0,y0),(x1,y1)=xy[ix[a]],xy[ix[b]];n=int(max(abs(x1-x0),abs(y1-y0)))+1
    xs=np.linspace(x0,x1,n).astype(int);ys=np.linspace(y0,y1,n).astype(int);ok=(xs>=0)&(xs<W)&(ys>=0)&(ys<H)
    acc[ys[ok],xs[ok]]+=(warm if e else cool)*.18
  img+=gaussian_filter(acc,(.7,.7,0))*2.2+gaussian_filter(acc,(6,6,0))*3
  nod=np.zeros((H,W,3))
  A=None
  if act is not None and d['act']:
    A=np.array(d['act'][min(act,len(d['act'])-1)],float);A=np.clip(A/1023,0,1)
  for k,n in enumerate(fin['n']):
    if n[0] not in present:continue
    x,y=xy[k].astype(int)
    if not(0<=x<W and 0<=y<H):continue
    io=n[2] or n[3];base=np.array([1,1,1])*(1.6 if io else 1.0)
    lvl=.6+(1.8*A[k] if A is not None and k<len(A) else .5)
    nod[y,x]+=base*lvl*(np.array([.7,1,.85]) if n[2] else np.array([1,.85,.6]) if n[3] else 1)
  img+=gaussian_filter(nod,(1.6,1.6,0))*30+gaussian_filter(nod,(9,9,0))*22
  img=tonemap(img)+np.array([.012,.013,.02])
  im=Image.fromarray((np.clip(img,0,1)*255).astype(np.uint8));dr=ImageDraw.Draw(im)
  try:fnt=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf',22);big=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf',30)
  except OSError:fnt=big=ImageFont.load_default()
  if code:
    dr.text((90,110),'GENOME  %d production rules'%len(d['rules']),font=big,fill=(236,235,228))
    y=170
    for r in d['rules']:
      s='%-7s -> %-7s + %-6s  %s'%(r['pre'] or '.',r['rep'] or '-',r['new'] or '-',r['links'])
      dr.text((90,y),s,font=fnt,fill=(245,215,110) if r['used'] else (110,110,110));y+=34
  dr.text((90,H-110),'step %s   neurons %d   links %d'%(format(d['time'],','),len(st['n']),len(st['l'])),font=fnt,fill=(154,154,146))
  return im

if __name__=='__main__':
  ap=argparse.ArgumentParser();ap.add_argument('inp');ap.add_argument('out')
  ap.add_argument('--step',type=int,default=-1);ap.add_argument('--act',type=int);ap.add_argument('--code',action='store_true')
  g=ap.parse_args();d=json.load(open(g.inp))
  render(d,g.step if g.step>=0 else len(d['dev'])-1,g.act,g.code).save(g.out)
