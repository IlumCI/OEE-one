#!/usr/bin/env python3
"""Style test frames from `replay CK STEPS EVERY l4` data (one frame each).

Usage: style_frames.py IN.bin FRAME OUTDIR
  tissue.png : microscopy look; colonies as soft stained tissue, bonds as filaments, warriors as hot nuclei
  lights.png : night-from-orbit look; creatures as city lights, bonds as roads, locked land as dark terrain
"""
import struct,sys,os
import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter

def frame(b,k):
  o=0;i=0
  while o<len(b):
    t,L,n=struct.unpack_from('<IHH',b,o);o+=8
    a=np.frombuffer(b,np.uint8,4*n,o).reshape(n,4);o+=4*n
    m,=struct.unpack_from('<H',b,o);o+=2
    e=np.frombuffer(b,np.uint16,2*m,o).reshape(m,2);o+=4*m
    lk=np.frombuffer(b,np.uint8,L*L,o).reshape(L,L);o+=L*L
    if i==k:return t,L,a.astype(int),e.astype(int),lk
    i+=1

H2=lambda h:np.array([int(h[i:i+2],16) for i in (1,3,5)],float)/255
def tonemap(x):return 1-np.exp(-x)
def lines(acc,a,e,L,S,w,col):
  T=L*S
  for i,q in e:
    x0,y0=(a[i,0]+.5)*S,(a[i,1]+.5)*S;x1,y1=(a[q,0]+.5)*S,(a[q,1]+.5)*S
    dx,dy=x1-x0,y1-y0
    if dx>T/2:dx-=T
    if dx<-T/2:dx+=T
    if dy>T/2:dy-=T
    if dy<-T/2:dy+=T
    n=int(max(abs(dx),abs(dy)))+1
    xs=((x0+dx*np.linspace(0,1,n))%T).astype(int);ys=((y0+dy*np.linspace(0,1,n))%T).astype(int)
    acc[ys,xs]+=w*col[a[i,2]]

def tissue(L,a,e,lk,S=18):
  T=L*S;pal=np.array([H2(c) for c in ('#e8a0bf','#7fb7e6','#f0c27b','#d9d4f5','#8fd6b4','#c79bf2')])
  img=np.zeros((T,T,3))
  for c in range(6):
    m=np.zeros((T,T));s=a[a[:,2]==c]
    m[(s[:,1].astype(int)*S+S//2),(s[:,0].astype(int)*S+S//2)]=1
    img+=gaussian_filter(m,S*.9,mode='wrap')[...,None]*pal[c]*S*S*1.6
  lf=gaussian_filter(np.kron(lk.astype(float),np.ones((S,S))),S*.4,mode='wrap')
  img+=lf[...,None]*np.array([.05,.06,.08])
  fil=np.zeros((T,T,3));lines(fil,a,e,L,S,1.0,pal)
  img+=gaussian_filter(fil,(1.2,1.2,0),mode='wrap')*1.4+gaussian_filter(fil,(5,5,0),mode='wrap')*1.5
  nuc=np.zeros((T,T,3))
  for x,y,c,f in a:
    hot=(f&1) or ((f>>1)-1)==2
    nuc[y*S+S//2,x*S+S//2]+=np.array([1,.25,.2])*3 if hot else pal[c]*1.2
  img+=gaussian_filter(nuc,(2,2,0),mode='wrap')*14+gaussian_filter(nuc,(7,7,0),mode='wrap')*6
  bg=np.array([.02,.02,.035])
  return tonemap(img*1.1)+bg

def lights(L,a,e,lk,S=18):
  T=L*S;img=np.zeros((T,T,3))
  land=gaussian_filter(np.kron(1-lk.astype(float),np.ones((S,S))),S*.7,mode='wrap')
  img+=land[...,None]*np.array([.035,.04,.05])
  warm=np.array([1.,.72,.35]);cool=np.array([.55,.75,1.])
  road=np.zeros((T,T,3));pal=np.tile(warm*.9,(6,1));lines(road,a,e,L,S,1.0,pal)
  img+=gaussian_filter(road,(.8,.8,0),mode='wrap')*1.2+gaussian_filter(road,(6,6,0),mode='wrap')*.9
  city=np.zeros((T,T,3))
  for x,y,c,f in a:
    hot=(f&1) or ((f>>1)-1)==2
    city[y*S+S//2,x*S+S//2]+=np.array([1,.3,.25])*2 if hot else (warm if c%2 else (warm*.7+cool*.3))
  img+=gaussian_filter(city,(1.5,1.5,0),mode='wrap')*18+gaussian_filter(city,(10,10,0),mode='wrap')*14+gaussian_filter(city,(30,30,0),mode='wrap')*16
  return tonemap(img)

def save(arr,path,W=1920,H=1080):
  arr=np.clip(arr,0,1);T=arr.shape[0]
  tile=np.tile(arr,(2,2,1))[:H,:W] if T<W else arr[:H,:W]
  Image.fromarray((tile*255).astype(np.uint8)).save(path)

if __name__=='__main__':
  b=open(sys.argv[1],'rb').read();k=int(sys.argv[2]);out=sys.argv[3];os.makedirs(out,exist_ok=True)
  t,L,a,e,lk=frame(b,k)
  save(tissue(L,a,e,lk),f'{out}/tissue.png');save(lights(L,a,e,lk),f'{out}/lights.png')
  print(t,L,len(a),len(e),int(lk.sum()))
