#!/usr/bin/env python3
"""Render `replay CK STEPS EVERY l4` output to PNG frames (nations look plus locked land).

Usage: render_l4.py IN.bin OUTDIR [W H] [--p P] [--trail A]
Whole torus drawn as one square, centred, so world growth is visible as the square widening.
Agents: 3x3 body in colony colour, red core = warrior/attacking, white corners = reproducing.
Bonds: dim colony colour. Locked cells: hollow grey square (an unopened lock on that land).
"""
import argparse,os,struct
import numpy as np
from PIL import Image

H2=lambda h:tuple(int(h[i:i+2],16) for i in (1,3,5))
VOID,LAT,BONE,STRIKE,GOLD=H2('#08090b'),H2('#1b1e24'),H2('#ecebe4'),H2('#ff2e4d'),H2('#f5d76e')
PAL=[H2(c) for c in ('#f5d76e','#4aa3c8','#e0a040','#ecebe4','#7fd1a8','#b48cf0')]
dim=lambda c,k:tuple(int(v*k) for v in c)
PALD=[dim(c,.42) for c in PAL];LOCK=dim(BONE,.3)
CELL=6

def frames(b):
  o=0
  while o<len(b):
    t,L,n=struct.unpack_from('<IHH',b,o);o+=8
    a=np.frombuffer(b,np.uint8,4*n,o).reshape(n,4);o+=4*n
    m,=struct.unpack_from('<H',b,o);o+=2
    e=np.frombuffer(b,np.uint16,2*m,o).reshape(m,2);o+=4*m
    lk=np.frombuffer(b,np.uint8,L*L,o).reshape(L,L);o+=L*L
    yield t,L,a,e,lk

def draw(L,a,e,lk):
  T=L*CELL;img=np.zeros((T,T,3),np.uint8);img[:]=VOID
  img[::CELL,::CELL]=LAT
  ys,xs=np.nonzero(lk)
  for y,x in zip(ys,xs):
    X,Y=x*CELL+1,y*CELL+1
    img[Y,X:X+4]=LOCK;img[Y+3,X:X+4]=LOCK;img[Y:Y+4,X]=LOCK;img[Y:Y+4,X+3]=LOCK
  c=lambda i:(int(a[i,0])*CELL+3,int(a[i,1])*CELL+3)
  for i,q in e:
    x0,y0=c(i);x1,y1=c(q);dx,dy=x1-x0,y1-y0
    if dx>T/2:dx-=T
    if dx<-T/2:dx+=T
    if dy>T/2:dy-=T
    if dy<-T/2:dy+=T
    s=max(abs(dx),abs(dy)) or 1;col=PALD[a[i,2]]
    for k in range(s+1):img[(y0+round(dy*k/s))%T,(x0+round(dx*k/s))%T]=col
  for i in range(len(a)):
    x,y=c(i);col=PAL[a[i,2]];fl=int(a[i,3]);pen=(fl>>1)-1
    img[(y-1)%T:(y-1)%T+3,(x-1)%T:(x-1)%T+3]=col
    if (fl&1) or pen==2:img[y,x]=STRIKE
    if pen==1:
      for ox,oy in ((-2,-2),(2,-2),(-2,2),(2,2)):img[(y+oy)%T,(x+ox)%T]=BONE
  return img

def main():
  ap=argparse.ArgumentParser();ap.add_argument('inp');ap.add_argument('out')
  ap.add_argument('W',nargs='?',type=int,default=1920);ap.add_argument('H',nargs='?',type=int,default=1080)
  ap.add_argument('--p',type=int,default=2);ap.add_argument('--trail',type=float,default=.6)
  g=ap.parse_args();os.makedirs(g.out,exist_ok=True)
  prev=None;meta=[]
  for k,(t,L,a,e,lk) in enumerate(frames(open(g.inp,'rb').read())):
    w=draw(L,a,e,lk).repeat(g.p,0).repeat(g.p,1)
    cv=np.zeros((g.H,g.W,3),np.uint8);cv[:]=VOID
    oy,ox=(g.H-w.shape[0])//2,(g.W-w.shape[1])//2
    sy,sx=max(0,-oy),max(0,-ox);h,wd=min(w.shape[0]-sy,g.H),min(w.shape[1]-sx,g.W)
    cv[max(0,oy):max(0,oy)+h,max(0,ox):max(0,ox)+wd]=w[sy:sy+h,sx:sx+wd]
    if prev is not None and prev.shape==cv.shape and g.trail<1:
      cv=np.maximum(cv,(prev*(1-g.trail)).astype(np.uint8))
    prev=cv
    Image.fromarray(cv).save(f'{g.out}/f{k:05d}.png')
    meta.append(f'{k} {t} {L} {len(a)} {len(e)} {int((lk>0).sum())}')
  open(f'{g.out}/frames.txt','w').write('\n'.join(meta)+'\n')

if __name__=='__main__':main()
