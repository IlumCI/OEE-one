#!/usr/bin/env python3
"""Four style directions as SVG stills from one netdump hero (same real data in each).

Usage: directions.py HERO.json OUTDIR   -> OUTDIR/{figure,stain,draft,printout}.html (render with Chromium)
"""
import json,os,sys,math
import numpy as np
sys.path.insert(0,os.path.dirname(__file__))
from render_brain import layout

def net(d):
  fin=d['dev'][-1];ids=[n[0] for n in fin['n']];ix={v:i for i,v in enumerate(ids)}
  L=[(ix[a],ix[b],e) for a,b,e in fin['l'] if a in ix and b in ix and a!=b]
  P=layout(len(ids),L);return fin,P,L

def rules(d):return [(r['pre'] or '·',r['rep'] or '–',r['new'] or '–',r['links']) for r in d['rules']]

def page(body,fonts,bg):
  return f'''<!doctype html><html><head><meta charset="utf-8"><link rel="stylesheet" href="https://fonts.googleapis.com/css2?{fonts}&display=block">
<style>html,body{{margin:0;width:1920px;height:1080px;overflow:hidden;background:{bg}}}svg{{display:block}}</style></head><body>{body}</body></html>'''

def figure(d,fin,P,L):
  # A: published scientific figure. White, black hairlines, red = excitatory, blue = inhibitory. Panels a/b/c.
  cx,cy,R=1260,500,330;xy=np.c_[cx+P[:,0]*R,cy+P[:,1]*R]
  s=[f'<rect width="1920" height="1080" fill="#FFFFFF"/>']
  for a,b,e in L:s.append(f'<line x1="{xy[a,0]:.1f}" y1="{xy[a,1]:.1f}" x2="{xy[b,0]:.1f}" y2="{xy[b,1]:.1f}" stroke="{"#D7262B" if e else "#1F5BA8"}" stroke-width="0.6" stroke-opacity="0.45"/>')
  for k,n in enumerate(fin['n']):
    io=n[2] or n[3];s.append(f'<circle cx="{xy[k,0]:.1f}" cy="{xy[k,1]:.1f}" r="{5 if io else 3.2}" fill="{"#FFFFFF" if io else "#111"}" stroke="#111" stroke-width="1.2"/>')
  s.append('<text x="150" y="190" font-family="IBM Plex Sans" font-weight="600" font-size="30" fill="#111">a</text>')
  y=240
  for p,r,nw,l in rules(d):
    s.append(f'<text x="150" y="{y}" font-family="IBM Plex Mono" font-size="24" fill="#111">{p}<tspan fill="#888"> → </tspan>{r}<tspan fill="#888"> + </tspan>{nw}</text>');y+=40
  s.append(f'<text x="830" y="190" font-family="IBM Plex Sans" font-weight="600" font-size="30" fill="#111">b</text>')
  s.append(f'<text x="150" y="930" font-family="IBM Plex Sans" font-size="26" fill="#111"><tspan font-weight="600">Fig. 1</tspan>  Genome of the most complex agent at step 980,000 (a) and the network it develops (b):</text>')
  s.append(f'<text x="150" y="968" font-family="IBM Plex Sans" font-size="26" fill="#111">100 neurons, 1,268 links. <tspan fill="#D7262B">Red</tspan> excitatory, <tspan fill="#1F5BA8">blue</tspan> inhibitory, open circles sensors and actuators.</text>')
  return page(f'<svg width="1920" height="1080">{"".join(s)}</svg>','family=IBM+Plex+Mono&family=IBM+Plex+Sans:wght@400;600','#fff')

def stain(d,fin,P,L):
  # B: histology slide, H&E-like stain. Pink eosin field, purple haematoxylin nuclei, fibres as eosin strands.
  cx,cy,R=960,540,470;xy=np.c_[cx+P[:,0]*R,cy+P[:,1]*R*0.95]
  rng=np.random.default_rng(3);s=['<defs><filter id="tex"><feTurbulence type="fractalNoise" baseFrequency="0.012" numOctaves="4" seed="5"/><feColorMatrix values="0 0 0 0 0.93  0 0 0 0 0.78  0 0 0 0 0.85  0 0 0 0.55 0"/></filter>'
     '<filter id="soft"><feGaussianBlur stdDeviation="1.1"/></filter><radialGradient id="vig" cx="50%" cy="50%" r="60%"><stop offset="70%" stop-color="#000" stop-opacity="0"/><stop offset="100%" stop-color="#3a1a2e" stop-opacity="0.35"/></radialGradient></defs>',
     '<rect width="1920" height="1080" fill="#F3DCE6"/><rect width="1920" height="1080" filter="url(#tex)"/>']
  for i in range(260):
    x,y=rng.uniform(0,1920),rng.uniform(0,1080);s.append(f'<ellipse cx="{x:.0f}" cy="{y:.0f}" rx="{rng.uniform(3,7):.1f}" ry="{rng.uniform(2,5):.1f}" fill="#C9A3C8" opacity="0.35"/>')
  for a,b,e in L:
    mx,my=(xy[a]+xy[b])/2+rng.normal(0,12,2)
    s.append(f'<path d="M{xy[a,0]:.1f},{xy[a,1]:.1f} Q{mx:.1f},{my:.1f} {xy[b,0]:.1f},{xy[b,1]:.1f}" stroke="{"#C2457A" if e else "#7D5BA6"}" stroke-width="{1.3 if e else 1.1}" fill="none" opacity="0.32" filter="url(#soft)"/>')
  for k,n in enumerate(fin['n']):
    r=rng.uniform(6,10);s.append(f'<ellipse cx="{xy[k,0]:.1f}" cy="{xy[k,1]:.1f}" rx="{r:.1f}" ry="{r*rng.uniform(.7,1):.1f}" fill="#3E1F5C" opacity="0.85" filter="url(#soft)"/>')
  s.append('<rect width="1920" height="1080" fill="url(#vig)"/>')
  s.append('<text x="96" y="1000" font-family="Libre Franklin" font-size="28" fill="#3E1F5C">Agent 322096372 · step 980,000 · 100 neurons · 1,268 links</text>')
  return page(f'<svg width="1920" height="1080">{"".join(s)}</svg>','family=Libre+Franklin:wght@400;500','#F3DCE6')

def draft(d,fin,P,L):
  # C: engineering drawing / cyanotype. The genome as a construction plan, the network as the drawn part.
  cx,cy,R=1240,500,380;xy=np.c_[cx+P[:,0]*R,cy+P[:,1]*R];ink='#EAF2FB'
  s=['<defs><filter id="pap"><feTurbulence type="fractalNoise" baseFrequency="0.9" numOctaves="2" seed="2"/><feColorMatrix values="0 0 0 0 1  0 0 0 0 1  0 0 0 0 1  0 0 0 0.05 0"/></filter>'
     '<pattern id="grid" width="48" height="48" patternUnits="userSpaceOnUse"><path d="M48 0H0V48" fill="none" stroke="#2C5D8E" stroke-width="1"/></pattern></defs>',
     '<rect width="1920" height="1080" fill="#123E6B"/><rect width="1920" height="1080" fill="url(#grid)" opacity="0.6"/><rect width="1920" height="1080" filter="url(#pap)"/>',
     f'<rect x="60" y="60" width="1800" height="960" fill="none" stroke="{ink}" stroke-width="2"/>']
  for a,b,e in L:s.append(f'<line x1="{xy[a,0]:.1f}" y1="{xy[a,1]:.1f}" x2="{xy[b,0]:.1f}" y2="{xy[b,1]:.1f}" stroke="{ink}" stroke-width="0.7" stroke-opacity="{0.42 if e else 0.25}" {"" if e else "stroke-dasharray=\"4 4\""}/>')
  for k,n in enumerate(fin['n']):
    io=n[2] or n[3];s.append(f'<circle cx="{xy[k,0]:.1f}" cy="{xy[k,1]:.1f}" r="{6 if io else 3.5}" fill="#123E6B" stroke="{ink}" stroke-width="1.4"/>')
  x0,x1=cx-R,cx-R+540;s.append(f'<line x1="{x0}" y1="995" x2="{x1}" y2="995" stroke="{ink}" stroke-width="1"/><line x1="{x0}" y1="985" x2="{x0}" y2="1005" stroke="{ink}"/><line x1="{x1}" y1="985" x2="{x1}" y2="1005" stroke="{ink}"/>')
  y=170;s.append(f'<text x="120" y="130" font-family="B612 Mono" font-size="22" fill="{ink}" opacity="0.7">PRODUCTION RULES</text>')
  for p,r,nw,l in rules(d):s.append(f'<text x="120" y="{y}" font-family="B612 Mono" font-size="24" fill="{ink}">{p} → {r} + {nw}</text>');y+=42
  s.append(f'<g font-family="B612 Mono" font-size="20" fill="{ink}"><rect x="1440" y="900" width="420" height="120" fill="none" stroke="{ink}" stroke-width="2"/>'
           f'<text x="1460" y="935">AGENT 322096372</text><text x="1460" y="967">STEP 980,000   L 60</text><text x="1460" y="999">100 NEURONS  1,268 LINKS</text></g>')
  return page(f'<svg width="1920" height="1080">{"".join(s)}</svg>','family=B612+Mono','#123E6B')

def printout(d,fin,P,L):
  # D: fanfold line-printer paper and a pen plotter. The genome printed, the network plotted in two pens.
  cx,cy,R=1240,560,380;xy=np.c_[cx+P[:,0]*R,cy+P[:,1]*R]
  s=['<rect width="1920" height="1080" fill="#FBFBF7"/>']
  for y in range(0,1080,120):s.append(f'<rect x="0" y="{y}" width="1920" height="60" fill="#DCEEDB"/>')
  for y in range(30,1080,60):s.append(f'<circle cx="34" cy="{y}" r="9" fill="#E7E7E1"/><circle cx="1886" cy="{y}" r="9" fill="#E7E7E1"/>')
  s.append('<line x1="68" y1="0" x2="68" y2="1080" stroke="#CFCFC6" stroke-dasharray="3 6"/><line x1="1852" y1="0" x2="1852" y2="1080" stroke="#CFCFC6" stroke-dasharray="3 6"/>')
  for a,b,e in L:s.append(f'<line x1="{xy[a,0]:.1f}" y1="{xy[a,1]:.1f}" x2="{xy[b,0]:.1f}" y2="{xy[b,1]:.1f}" stroke="{"#1B1B1B" if e else "#D2232A"}" stroke-width="0.8" stroke-opacity="0.5" stroke-linecap="round"/>')
  for k,n in enumerate(fin['n']):s.append(f'<circle cx="{xy[k,0]:.1f}" cy="{xy[k,1]:.1f}" r="4" fill="none" stroke="#1B1B1B" stroke-width="1.6"/>')
  y=150;s.append(f'<text x="120" y="{y-60}" font-family="VT323" font-size="34" fill="#222">OEE-ONE  RUN L4LB_S1  STEP 980000  AGENT 322096372</text>')
  for i,(p,r,nw,l) in enumerate(rules(d)):s.append(f'<text x="120" y="{y}" font-family="VT323" font-size="34" fill="#222">RULE {i+1:02d}  {p:>6} -> {r:<6} + {nw:<7} {l}</text>');y+=60
  return page(f'<svg width="1920" height="1080">{"".join(s)}</svg>','family=VT323','#FBFBF7')

if __name__=='__main__':
  d=json.load(open(sys.argv[1]));out=sys.argv[2];os.makedirs(out,exist_ok=True);fin,P,L=net(d)
  for n,f in (('figure',figure),('stain',stain),('draft',draft),('printout',printout)):open(f'{out}/{n}.html','w').write(f(d,fin,P,L))
