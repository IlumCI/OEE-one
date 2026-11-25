#!/usr/bin/env python3
"""Champion tournament matrix of a World v1 run, and its transitive / cyclic split (docs/WORLD.md §6, W1 protocol).

Usage: ciao.py BIN RUNDIR [--every 250] [--seeds 16] [--threads 4] [--upto E] [--world "-S 64 -regrow 2 -nact 7"]
Plays every pair of champions archived every `every` epochs (mirrored seeds, integrated share; cached in
RUNDIR/ciao_<every>.tsv, so reruns only play new pairs), then:
- A_ij = logit(p_ij), p_ij = share of i against j (clipped to [0.01, 0.99]);
- transitive part r_i - r_j with r = row means of A; cyclic part = A minus that (combinatorial Hodge decomposition,
  Balduzzi et al. 2018, arXiv:1806.02643);
- prints each champion's rating r and the cyclic share |cyclic|^2 / |A|^2 (0 = fully transitive).
"""
import argparse,math,os,re,subprocess,sys

def play(bin_,a,b,seeds,threads,world):
    out=subprocess.run([bin_,'-tourney',a,b,'-seeds',str(seeds),'-threads',str(threads)]+world.split(),capture_output=True,text=True,check=True).stdout
    m=re.search(r'^mean_ishareA ([0-9.]+)\tse ([0-9.]+)',out,re.M)
    return float(m[1]),float(m[2])

def hodge(E,P):
    n=len(E);lg=lambda p:math.log(min(.99,max(.01,p))/(1-min(.99,max(.01,p))))
    A=[[0.0]*n for _ in range(n)]
    for i in range(n):
        for j in range(n):
            if i!=j:A[i][j]=lg(P[(E[i],E[j])]) if (E[i],E[j]) in P else -lg(P[(E[j],E[i])])
    r=[sum(A[i])/n for i in range(n)]
    tot=sum(A[i][j]**2 for i in range(n) for j in range(n))
    cyc=sum((A[i][j]-(r[i]-r[j]))**2 for i in range(n) for j in range(n))
    return r,(cyc/tot if tot>0 else 0.0)

def main():
    ap=argparse.ArgumentParser();ap.add_argument('bin');ap.add_argument('run');ap.add_argument('--every',type=int,default=250)
    ap.add_argument('--seeds',type=int,default=16);ap.add_argument('--threads',type=int,default=4);ap.add_argument('--upto',type=int,default=0)
    ap.add_argument('--world',default='-S 64 -regrow 2 -nact 7 -ticks 2000');a=ap.parse_args()
    arch=os.path.join(a.run,'archive')
    E=sorted(int(m[1]) for f in os.listdir(arch) for m in [re.match(r'egg_(\d+)\.bin$',f)] if m and int(m[1])%a.every==0 and (a.upto<=0 or int(m[1])<=a.upto))
    cache=os.path.join(a.run,f'ciao_{a.every}.tsv');P={}
    if os.path.exists(cache):
        for l in open(cache):
            x=l.split()
            if len(x)>=3 and x[0].isdigit():P[(int(x[0]),int(x[1]))]=float(x[2])
    with open(cache,'a') as f:
        for i,ei in enumerate(E):
            for ej in E[:i]:
                if (ei,ej) in P:continue
                p,se=play(a.bin,os.path.join(arch,f'egg_{ei}.bin'),os.path.join(arch,f'egg_{ej}.bin'),a.seeds,a.threads,a.world)
                P[(ei,ej)]=p;f.write(f'{ei}\t{ej}\t{p:.4f}\t{se:.4f}\n');f.flush()
    if len(E)<3:print('need at least 3 champions');return
    r,c=hodge(E,P)
    print('epoch\trating');[print(f'{e}\t{x:+.3f}') for e,x in zip(E,r)]
    rk=lambda v:[sorted(v).index(x) for x in v];re_,rr=rk(E),rk(r);n=len(E)
    rho=1-6*sum((x-y)**2 for x,y in zip(re_,rr))/(n*(n*n-1))
    print(f'champions {n}  cyclic_share {c:.4f}  spearman(epoch,rating) {rho:+.3f}')

if __name__=='__main__':main()
