#!/usr/bin/env python3
# Agent study (docs/WORLD.md, agent study): reads oee_w -probe files (one record per living agent of the probed
# lineage per tick) and reports, one column per file:
#  policy: action shares, policy entropy (P(a) ~ 2^(logit/16), as the world samples), near-deterministic steps;
#  futile actions: share of each action that had no effect (blocked move, empty bite, feed with nothing in front...);
#  situations: choices conditioned on what the agent saw (food underfoot, machine or farm in front, energy...);
#  cue (W4g): turning and moving relative to the direction of the own farm, distance to it against random positions;
#  hidden state: saturated (|h| >= 127), constant and tau-dormant units (Sokar et al. 2023, arXiv:2302.12902),
#  effective rank srank (Kumar et al. 2021, arXiv:2010.14498), lag-1 persistence;
#  outputs: action logits that never change or sit at the int8 limits, actions never drawn;
#  lifetimes: lifespan, causes of death, births, food, movement, spinning and stuck agents.
# Usage: tools/w/probe.py [-names a,b,..] [-trace GAME:ID] FILE...
import sys,os,numpy as np
AN=['noop','fwd','left','right','eat','repro','attack','write','make','feed']
def load(fn):
  b=open(fn,'rb').read()
  if b[:8]!=b'WPRB0001':raise SystemExit(fn+': not a probe file')
  din,V,LD,nact,S,L,ng,solo=[int(x) for x in np.frombuffer(b[8:40],'<i4')]
  dt=np.dtype([('t','<u4'),('id','<u4'),('g','u1'),('s','u1'),('x','u1'),('y','u1'),('dir','u1'),('a','u1'),('die','u1'),('pad','u1'),
    ('eff','<i2'),('e0','<i2'),('e1','<i2'),('age','<i2'),('o','i1',(din,)),('l','i1',(V,)),('h','i1',(LD,))])
  return dict(din=din,V=V,LD=LD,nact=nact,S=S,L=L,ng=ng,solo=solo,r=np.frombuffer(b[40:],dt))
def pol(l,n):  # P(a) ~ 2^(l/16) over the first n logits
  z=l[:,:n].astype(np.float64)/16*np.log(2);z-=z.max(1,keepdims=True);p=np.exp(z);return p/p.sum(1,keepdims=True)
def srank(X,d=0.01):
  s=np.linalg.svd(X.astype(np.float64),compute_uv=False);c=np.cumsum(s)/max(s.sum(),1e-12);return int(np.searchsorted(c,1-d)+1)
def frac(m):return float(m.mean()) if m.size else float('nan')
def cond(a,m,k):  # P(action k | situation m), count
  n=int(m.sum());return (float((a[m]==k).mean()) if n else float('nan')),n
def study(D,trace=None):
  r=D['r'];n=len(r);out={};na=D['nact'];o=r['o'].astype(np.int16);a=r['a'].astype(np.int16);ef=r['eff'].astype(np.int32)
  out['records']=n;out['games']=len(np.unique(r['g']))
  if n==0:return out
  # policy
  for k in range(na):out['share '+AN[k]]=frac(a==k)
  P=pol(r['l'],na);H=-(P*np.log2(np.maximum(P,1e-300))).sum(1)
  out['entropy bits (max %.2f)'%np.log2(na)]=float(H.mean());out['steps with max P > 0.9']=frac(P.max(1)>0.9)
  # futile actions: the share of each action with no effect
  for k,nm in [(1,'fwd blocked'),(4,'eat nothing'),(5,'repro failed'),(6,'attack no target'),(7,'write no machine'),(8,'make/unmake nothing'),(9,'feed nothing')]:
    if k<na:m=a==k;out['futile '+nm]=frac(ef[m]==0)
  out['futile per agent-tick']=frac(np.isin(a,[1,4,5,6,7,8,9])&(ef==0))
  ag=r['age'].astype(np.int32);fu=np.isin(a,[1,4,5,6,7,8,9])&(ef==0)
  for lo,hi in [(0,50),(50,200),(200,500)]:
    m=(ag>=lo)&(ag<hi);out['futile, age %d-%d'%(lo,hi-1)]=frac(fu[m]);out['  n age %d-%d'%(lo,hi-1)]=int(m.sum())
  # situations (observation layout: World::observe); window cell 7 is in front, 12 underfoot
  fin=o[:,25+7];fmat=o[:,79+7] if D['din']>=104 else np.zeros(n,np.int16);food=o[:,12]*2;en=o[:,50]*8
  farm=(fmat==48)&np.all(o[:,104:120]==60,1) if D['din']>=120 else np.zeros(n,bool)  # tape of 16 ',' (irrigation)
  st=o[:,120]*4 if D['din']>=121 else np.zeros(n,np.int16)
  out['P(eat | food underfoot >= 32)'],out['  n food']=cond(a,food>=32,4)
  out['P(eat | no food underfoot)'],out['  n nofood']=cond(a,food==0,4)
  out['P(eat | food underfoot 2-30)'],out['  n little food']=cond(a,(food>0)&(food<32),4)
  lg=r['l'].astype(np.float64)
  for nm,m in [('0',food==0),('2-30',(food>0)&(food<32)),('>= 32',food>=32)]:out['mean logit eat - fwd | food %s'%nm]=float((lg[m,4]-lg[m,1]).mean()) if m.any() else float('nan')
  bl=(fin!=0)|(fmat==-48)|(fmat==96)
  for nm,m in [('free',~bl),('blocked',bl)]:out['mean logit fwd - right | front %s'%nm]=float((lg[m,1]-lg[m,3]).mean()) if m.any() else float('nan')
  out['P(fwd | agent/wall/node in front)'],out['  n blocked']=cond(a,(fin!=0)|(fmat==-48)|(fmat==96),1)
  out['P(repro | energy >= 120, front free)'],out['  n can repro']=cond(a,(en>=120)&(fin==0)&(fmat==0),5)
  out['P(attack | other lineage in front)'],out['  n other']=cond(a,fin==-64,6)
  out['P(attack | kin in front)'],out['  n kin']=cond(a,fin==64,6)
  out['P(feed | machine in front)'],out['  n machine']=cond(a,fmat==48,9)
  out['P(feed | farm in front)'],out['  n farm']=cond(a,farm,9)
  out['P(feed | farm in front, store < 4)'],out['  n farm empty']=cond(a,farm&(st<4),9)
  out['P(eat | farm in front, store >= 40)'],out['  n farm full']=cond(a,farm&(st>=40),4)
  out['P(make | front empty)'],out['  n empty']=cond(a,(fmat==0)&(fin==0),8)
  if D['din']>=123:out['share on own land']=frac(o[:,122]==64);out['share on other land']=frac(o[:,122]==-64)
  out['mean energy']=float(r['e0'].mean());out['share energy < 20']=frac(r['e0']<20)
  # cue following (W4g entries 123-126: forward and right offset x8, farm store/2, irrigation/2)
  if D['din']>=127 and np.any(o[:,123:127]!=0):
    f=o[:,123]//8;g=o[:,124]//8;d=np.abs(f)+np.abs(g)
    for nm,m in [('ahead',(f>0)&(g==0)),('behind',(f<0)&(g==0)),('right',g>0),('left',g<0)]:
      for k in (1,2,3):out['cue %s: P(%s)'%(nm,AN[k])]=cond(a,m,k)[0]
    out['cue index right-turn (P(right|R)-P(right|L))']=out['cue right: P(right)']-out['cue left: P(right)']
    out['cue index left-turn (P(left|L)-P(left|R))']=out['cue left: P(left)']-out['cue right: P(left)']
    out['cue index fwd (P(fwd|ahead)-P(fwd|behind))']=out['cue ahead: P(fwd)']-out['cue behind: P(fwd)']
    L=D['L'];yy,xx=np.mgrid[0:L,0:L];best=np.full((L,L),1<<30)
    for i in range(6):
      cx,cy=i%3,i//3;dx=np.abs(xx-cx);dx=np.minimum(dx,L-dx);dy=np.abs(yy-cy);dy=np.minimum(dy,L-dy);best=np.minimum(best,dx+dy)
    out['farm distance mean']=float(d.mean());out['farm distance mean if random']=float(best.mean())
    out['share within 2 of farm']=frac(d<=2);out['share within 2 if random']=float((best<=2).mean())
  # hidden state (state after the step; layer-major L x D)
  h=r['h'].astype(np.int16);LD=D['LD'];nl=2 if LD%2==0 else 1;Dw=LD//nl;sub=np.random.default_rng(1).choice(n,min(n,20000),replace=False)
  for li in range(nl):
    x=h[:,li*Dw:(li+1)*Dw];ax=np.abs(x);sat=(ax>=127).mean(0);sd=x.std(0);sc=ax.mean(0)/max(ax.mean(),1e-9)
    out['L%d share of values saturated'%li]=float(sat.mean());out['L%d units saturated > 90%%'%li]=int((sat>0.9).sum())
    out['L%d units constant'%li]=int((sd==0).sum());out['L%d units tau-dormant (0.1)'%li]=int((sc<=0.1).sum())
    out['L%d mean |h|'%li]=float(ax.mean());out['L%d srank (of %d)'%(li,Dw)]=srank(x[sub])
  # persistence: lag-1 correlation of each unit along each agent's life
  k=np.lexsort((r['t'],r['id'],r['g']));rs=r[k];same=(rs['g'][1:]==rs['g'][:-1])&(rs['id'][1:]==rs['id'][:-1])&(rs['t'][1:]==rs['t'][:-1]+1)
  if same.sum()>100:
    h0=rs['h'][:-1][same].astype(np.float64);h1=rs['h'][1:][same].astype(np.float64)
    h0-=h0.mean(0);h1-=h1.mean(0);c=(h0*h1).sum(0)/np.sqrt(np.maximum((h0*h0).sum(0)*(h1*h1).sum(0),1e-12))
    out['median unit lag-1 correlation']=float(np.median(c));out['units lag-1 corr > 0.99']=int((c>0.99).sum())
  # outputs
  l=r['l'].astype(np.int16);la=l[:,:na]
  out['action logits constant']=int((la.std(0)==0).sum());out['action logit values at int8 limit']=frac((la>=127)|(la<=-127))
  out['actions with mean P < 0.001']=' '.join(AN[k] for k in range(na) if P[:,k].mean()<0.001) or '-'
  # lifetimes
  gi=rs['g'].astype(np.int64)*(1<<32)+rs['id'];u,first,cnt=np.unique(gi,return_index=True,return_counts=True);last=first+cnt-1
  die=rs['die'][last];full=(rs['t'][first]>0)  # agents born after tick 0 (founders start at t 0)
  out['agents']=len(u);out['median life (ticks seen)']=float(np.median(cnt))
  for v,nm in [(1,'starved'),(2,'aged'),(3,'killed'),(0,'alive at end')]:out['died '+nm if v else nm]=frac(die==v)
  ra=rs['a'].astype(np.int16);re=rs['eff'].astype(np.int32);ida=np.repeat(np.arange(len(u)),cnt)
  births=np.bincount(ida,weights=(ra==5)&(re>0),minlength=len(u));moves=np.bincount(ida,weights=(ra==1)&(re>0),minlength=len(u))
  turns=np.bincount(ida,weights=(ra==2)|(ra==3),minlength=len(u));food_=np.bincount(ida,weights=np.where(ra==4,re,0),minlength=len(u))
  fed=np.bincount(ida,weights=np.where(ra==9,re,0),minlength=len(u));lng=cnt>=50
  out['births per agent']=float(births.mean());out['share agents with no birth (life >= 50)']=frac(births[lng]==0)
  out['food per agent-tick']=float(food_.sum()/cnt.sum());out['fed per agent-tick']=float(fed.sum()/cnt.sum())
  out['share agents never moving (life >= 50)']=frac(moves[lng]==0);out['share agents turning > 50% (life >= 50)']=frac(turns[lng]/cnt[lng]>0.5)
  out['share agents feeding at all (life >= 50)']=frac(fed[lng]>0)
  if trace is not None:
    g,i=trace;m=(rs['g']==g)&(rs['id']==i)
    print('trace game %d id %d: t x y dir energy age act eff die | food_under front_agent front_mat front_store land cue_f cue_r cue_store cue_irr'%(g,i))
    for q in rs[m]:
      ob=q['o'].astype(int);cue=(ob[123]//8,ob[124]//8,ob[125]*2,ob[126]*2) if D['din']>=127 else (0,0,0,0)
      print(q['t'],q['x'],q['y'],q['dir'],q['e0'],q['age'],AN[q['a']] if q['a']<len(AN) else q['a'],q['eff'],q['die'],'|',ob[12]*2,ob[32],ob[86] if D['din']>=104 else 0,ob[120]*4 if D['din']>=121 else 0,ob[122] if D['din']>=123 else 0,*cue)
  return out
def main(av):
  names=None;trace=None;fs=[]
  i=0
  while i<len(av):
    if av[i]=='-names':names=av[i+1].split(',');i+=2
    elif av[i]=='-trace':g,x=av[i+1].split(':');trace=(int(g),int(x));i+=2
    else:fs.append(av[i]);i+=1
  if not fs:raise SystemExit(__doc__ if __doc__ else 'usage: probe.py [-names a,b] [-trace GAME:ID] FILE...')
  names=names or [os.path.basename(f) for f in fs];res=[study(load(f),trace if j==0 else None) for j,f in enumerate(fs)]
  keys=[];[keys.append(k) for o in res for k in o if k not in keys]
  w=max(len(k) for k in keys);cw=max(12,max(len(x) for x in names)+1)
  print(' '*w+''.join(x.rjust(cw) for x in names))
  for k in keys:
    row=[]
    for o in res:
      v=o.get(k,'');row.append(('%.4f'%v if isinstance(v,float) else str(v)).rjust(cw))
    print(k.ljust(w)+''.join(row))
if __name__=='__main__':main(sys.argv[1:])
