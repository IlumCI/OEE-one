#include "l0/world.hpp"

#include <algorithm>
#include <cmath>

namespace oee::l0 {

namespace {
std::uint64_t mix(std::uint64_t x){x+=0x9e3779b97f4a7c15ULL;x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;return x^(x>>31);}
// counter-based: value depends only on (seed,t,cell,k), never on call order
std::uint64_t ctr(std::uint64_t s,long t,int c,int k){return mix(mix(mix(s^0x5bd1e995ULL)+(std::uint64_t)t)+((std::uint64_t)c<<8|(std::uint64_t)k));}
double u01(std::uint64_t x){return (x>>11)*(1.0/9007199254740992.0);}
double gauss(std::uint64_t x){double u=u01(x),v=u01(mix(x));if(u<1e-300)u=1e-300;return std::sqrt(-2*std::log(u))*std::cos(6.283185307179586*v);}

void fft1(double* re,double* im,int n,int st,bool inv){
  for(int i=1,j=0;i<n;++i){int b=n>>1;for(;j&b;b>>=1)j^=b;j^=b;if(i<j){std::swap(re[i*st],re[j*st]);std::swap(im[i*st],im[j*st]);}}
  for(int len=2;len<=n;len<<=1){
    const double a=(inv?2:-2)*3.141592653589793/len;
    for(int i=0;i<n;i+=len)for(int k=0;k<len/2;++k){
      const double wr=std::cos(a*k),wi=std::sin(a*k);
      double& ur=re[(i+k)*st];double& ui=im[(i+k)*st];double& vr=re[(i+k+len/2)*st];double& vi=im[(i+k+len/2)*st];
      const double tr=vr*wr-vi*wi,ti=vr*wi+vi*wr;
      vr=ur-tr;vi=ui-ti;ur+=tr;ui+=ti;
    }
  }
}
}  // namespace

void fft2(std::vector<double>& re,std::vector<double>& im,int n,bool inv){
  for(int r=0;r<n;++r)fft1(&re[r*n],&im[r*n],n,1,inv);
  for(int c=0;c<n;++c)fft1(&re[c],&im[c],n,n,inv);
  if(inv){const double s=1.0/((double)n*n);for(std::size_t i=0;i<re.size();++i){re[i]*=s;im[i]*=s;}}
}

World::World(const Params& p):p_(p){
  const int n=p_.n,N=n*n,K=nk();
  m_.assign(N,0);P_.assign((std::size_t)N*K,0.f);A_.assign(N,0);
  for(const Kernel& k:p_.bank){
    std::vector<double> re(N,0),im(N,0);double s=0;
    for(int y=0;y<n;++y)for(int x=0;x<n;++x){
      const int dx=x<n/2?x:x-n,dy=y<n/2?y:y-n;const double r=std::sqrt((double)dx*dx+dy*dy)/k.R;
      if(r>=1)continue;
      double v=0;for(int j=0;j<3;++j)v+=k.b[j]*std::exp(-(r-k.a[j])*(r-k.a[j])/(2*k.w[j]*k.w[j]));
      re[y*n+x]=v;s+=v;
    }
    if(s>0)for(double& v:re)v/=s;
    fft2(re,im,n,false);khat_re_.push_back(re);khat_im_.push_back(im);
  }
}

std::int64_t World::total()const{std::int64_t s=0;for(auto v:m_)s+=v;return s;}

std::uint64_t World::hash()const{
  std::uint64_t h=1469598103934665603ULL;
  for(auto v:m_)h=(h^(std::uint64_t)v)*1099511628211ULL;
  for(float f:P_){std::uint32_t b;static_assert(sizeof b==sizeof f);std::copy_n(reinterpret_cast<const char*>(&f),4,reinterpret_cast<char*>(&b));h=(h^b)*1099511628211ULL;}
  return h^(std::uint64_t)t_;
}

void World::seed_patches(int count,int radius,double density){
  const int n=p_.n,K=nk();
  for(int i=0;i<count;++i){
    const std::uint64_t r=ctr(p_.seed,-1,i,0);const int cx=(int)(u01(r)*n),cy=(int)(u01(mix(r))*n);
    std::vector<float> pv(K);for(int k=0;k<K;++k)pv[k]=(float)u01(ctr(p_.seed,-2,i,k));
    for(int dy=-radius;dy<=radius;++dy)for(int dx=-radius;dx<=radius;++dx){
      if(dx*dx+dy*dy>radius*radius)continue;
      const int c=((cy+dy+n)%n)*n+(cx+dx+n)%n;
      const std::int64_t add=(std::int64_t)(u01(ctr(p_.seed,-3,c,i))*density*kOne);
      if(add<=0)continue;
      const double w0=(double)m_[c],w1=(double)add;
      for(int k=0;k<K;++k)P_[(std::size_t)c*K+k]=(float)((P_[(std::size_t)c*K+k]*w0+pv[k]*w1)/(w0+w1));
      m_[c]+=add;
    }
  }
}

void World::affinity_(){
  const int n=p_.n,N=n*n,K=nk();
  if(p_.neutral&&pbar_.empty()&&(int)p_.fixedP.size()==K)pbar_=p_.fixedP;
  if(p_.neutral&&pbar_.empty()){pbar_.assign(K,0);double W=0;for(int c=0;c<N;++c){const double m=(double)m_[c];W+=m;for(int k=0;k<K;++k)pbar_[k]+=m*P_[(std::size_t)c*K+k];}if(W>0)for(double& v:pbar_)v/=W;}
  std::vector<double> fr(N),fi(N,0);
  for(int c=0;c<N;++c)fr[c]=(double)m_[c]/kOne;
  fft2(fr,fi,n,false);
  std::fill(A_.begin(),A_.end(),0.0);
  std::vector<double> re(N),im(N);
  for(int k=0;k<K;++k){
    const auto& kr=khat_re_[k];const auto& ki=khat_im_[k];
    for(int c=0;c<N;++c){re[c]=fr[c]*kr[c]-fi[c]*ki[c];im[c]=fr[c]*ki[c]+fi[c]*kr[c];}
    fft2(re,im,n,true);
    const Kernel& kb=p_.bank[k];
    for(int c=0;c<N;++c){const double d=re[c]-kb.mu;A_[c]+=(p_.neutral?pbar_[k]:P_[(std::size_t)c*K+k])*(2*std::exp(-d*d/(2*kb.sigma*kb.sigma))-1);}
  }
}

void World::transport_(){
  const int n=p_.n,N=n*n,K=nk();
  std::vector<double> e(N);
  for(int c=0;c<N;++c)e[c]=std::exp(std::clamp(p_.beta*A_[c],-60.0,60.0));
  std::vector<std::int64_t> nm(N,0);std::vector<double> acc((std::size_t)N*K,0.0);
  std::vector<int> pick(p_.sample?N:0,-1),cnt(p_.sample?N:0,0);  // sample mode: chosen source per target
  for(int y=0;y<n;++y)for(int x=0;x<n;++x){
    const int c=y*n+x;const std::int64_t mc=m_[c];if(mc==0)continue;
    int q[9];double w[9],W=0;
    for(int j=0,dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx,++j){q[j]=((y+dy+n)%n)*n+(x+dx+n)%n;w[j]=e[q[j]];W+=w[j];}
    std::int64_t sh[9],used=0;double fr[9];
    for(int j=0;j<9;++j){const double ideal=(double)mc*(w[j]/W);sh[j]=(std::int64_t)std::floor(ideal);fr[j]=ideal-(double)sh[j];used+=sh[j];}
    std::int64_t rem=mc-used;
    while(rem>0){int b=0;for(int j=1;j<9;++j)if(fr[j]>fr[b])b=j;++sh[b];fr[b]=-1;--rem;}  // largest remainder, ties to lowest j
    while(rem<0){int b=0;for(int j=1;j<9;++j)if(sh[j]>sh[b])b=j;--sh[b];++rem;}            // guard (rounding), never in practice
    const float* pc=&P_[(std::size_t)c*K];
    for(int j=0;j<9;++j){if(!sh[j])continue;const int tq=q[j];nm[tq]+=sh[j];
      if(p_.sample){if(u01(ctr(p_.seed^0xA5A5ULL,t_,tq,cnt[tq]++))*(double)nm[tq]<(double)sh[j])pick[tq]=c;continue;}  // weighted reservoir, fixed source order
      double* a=&acc[(std::size_t)tq*K];for(int k=0;k<K;++k)a[k]+=(double)sh[j]*pc[k];}
  }
  const std::vector<float> old=p_.sample?P_:std::vector<float>();
  for(int c=0;c<N;++c){
    float* pc=&P_[(std::size_t)c*K];const double* a=&acc[(std::size_t)c*K];
    if(nm[c]==0){for(int k=0;k<K;++k)pc[k]=0.f;continue;}
    if(p_.sample)for(int k=0;k<K;++k)pc[k]=old[(std::size_t)pick[c]*K+k];
    else for(int k=0;k<K;++k)pc[k]=(float)(a[k]/(double)nm[c]);
    if(p_.mut_p>0&&u01(ctr(p_.seed,t_,c,255))<p_.mut_p)
      for(int k=0;k<K;++k)pc[k]=(float)std::clamp((double)pc[k]+p_.mut_s*gauss(ctr(p_.seed,t_,c,k)),0.0,1.0);
  }
  m_.swap(nm);
}

void World::step(){affinity_();transport_();++t_;}

}  // namespace oee::l0
