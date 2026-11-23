import sys,struct,glob,numpy as np
K=3
def traj(d):
  out=[]
  for f in sorted(glob.glob(d+'/f*.bin')):
    b=open(f,'rb').read();n=struct.unpack_from('<H',b)[0];m=np.frombuffer(b,np.float32,n*n,2).astype(float);P=np.frombuffer(b,np.float32,n*n*K,2+4*n*n).reshape(-1,K)
    w=m/m.sum();mu=(w[:,None]*P).sum(0);sd=np.sqrt((w[:,None]*(P-mu)**2).sum(0)).mean();out.append((mu,sd))
  return out
print('seed  D:|dmean| D:sd_end  E:|dmean| E:sd_end')
rd=[];re=[]
for s in range(1,7):
  D=traj(sys.argv[1]+'/D_s%d'%s);E=traj(sys.argv[1]+'/E_s%d'%s)
  dd=np.linalg.norm(D[-1][0]-D[0][0]);de=np.linalg.norm(E[-1][0]-E[0][0]);rd.append(dd);re.append(de)
  print(s,'%.3f %.3f   %.3f %.3f'%(dd,D[-1][1],de,E[-1][1]), 'D mean P end',np.round(D[-1][0],2),'E',np.round(E[-1][0],2))
print('mean |dmean|: D %.3f  E %.3f ; D>E in %d/6'%(np.mean(rd),np.mean(re),sum(a>b for a,b in zip(rd,re))))
