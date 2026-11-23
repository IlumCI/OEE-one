import sys,struct,glob,os,json,numpy as np
K=3;out={}
for d in sorted(glob.glob(sys.argv[1]+'/*_s*')):
  fs=sorted(glob.glob(d+'/f*.bin'));b=open(fs[-1],'rb').read();n=struct.unpack_from('<H',b)[0]
  m=np.frombuffer(b,np.float32,n*n,2);P=np.frombuffer(b,np.float32,n*n*K,2+4*n*n).reshape(-1,K)
  live=m>0.1;Q=np.round(P[live]/0.05).astype(int);w=m[live]
  keys,inv=np.unique(Q,axis=0,return_inverse=True);mass=np.bincount(inv.ravel(),weights=w);p=mass/mass.sum()
  types=int((p>0.01).sum());H=float(-(p*np.log2(p)).sum())
  log=[l.split() for l in open(d+'/log.txt') if not l.startswith('#')];last=log[-1]
  out[os.path.basename(d)]=dict(types=types,entropy=H,blobs=int(last[3]),area=float(last[4]),psd=float(last[5]))
json.dump(out,open(sys.argv[2],'w'),indent=1)
for k,v in out.items():print(k,v)
