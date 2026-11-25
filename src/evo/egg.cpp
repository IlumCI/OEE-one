#include "evo/egg.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>

#include "evo/gemm.hpp"

namespace oee::evo {

namespace {
std::uint64_t mix(std::uint64_t x){x+=0x9e3779b97f4a7c15ULL;x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;return x^(x>>31);}
std::int8_t c8(std::int64_t x){return (std::int8_t)(x>127?127:x<-127?-127:x);}  // I8 cast: clip to [-127,127]
inline std::int8_t c8i(int x){return (std::int8_t)std::min(127,std::max(-127,x));}  // same, int32 (vectorises)

// Acklam's inverse normal CDF (relative error < 1.2e-9).
double invphi(double p){
  static const double a[]={-3.969683028665376e+01,2.209460984245205e+02,-2.759285104469687e+02,1.383577518672690e+02,-3.066479806614716e+01,2.506628277459239e+00};
  static const double b[]={-5.447609879822406e+01,1.615858368580409e+02,-1.556989798598866e+02,6.680131188771972e+01,-1.328068155288572e+01};
  static const double c[]={-7.784894002430293e-03,-3.223964580411365e-01,-2.400758277161838e+00,-2.549732539343734e+00,4.374664141464968e+00,2.938163982698783e+00};
  static const double d[]={7.784695709041462e-03,3.224671290700398e-01,2.445134137142996e+00,3.754408661907416e+00};
  const double pl=0.02425;
  if(p<pl){const double q=std::sqrt(-2*std::log(p));return (((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5])/((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1);}
  if(p>1-pl){const double q=std::sqrt(-2*std::log(1-p));return -(((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5])/((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1);}
  const double q=p-0.5,r=q*q;return (((((a[0]*r+a[1])*r+a[2])*r+a[3])*r+a[4])*r+a[5])*q/(((((b[0]*r+b[1])*r+b[2])*r+b[3])*r+b[4])*r+1);
}
// int8 Gaussian sample from 16 uniform bits: round(16 * N(0,1)), clipped (Appendix G.3 / H.1).
const std::int8_t* gauss_table(){
  static std::vector<std::int8_t> t=[]{std::vector<std::int8_t> v(65536);for(int u=0;u<65536;++u)v[u]=c8(std::llround(16*invphi((u+0.5)/65536.0)));return v;}();
  return t.data();
}
void fill_gauss(std::int8_t* out,std::size_t n,std::uint64_t key){
  const std::int8_t* g=gauss_table();
  for(std::size_t i=0;i<n;i+=4){const std::uint64_t h=mix(key+i);for(std::size_t k=0;k<4&&i+k<n;++k)out[i+k]=g[(h>>(16*k))&0xffff];}
}
int ilog2(int x){int k=0;while((1<<k)<x)++k;if((1<<k)!=x)throw std::runtime_error("EGG: width must be a power of two");return k;}

// Fitness tables (Appendix G.9): EXP2[i] = 16*2^(i/16) for i in 0..255; LOG2(x) = 16*log2(x/16).
const std::int32_t* exp2_table(){static std::int32_t t[256];static bool done=[]{for(int i=0;i<256;++i)t[i]=(std::int32_t)std::llround(16*std::exp2(i/16.0));return true;}();(void)done;return t;}
std::int32_t log2_16(std::int64_t x){return (std::int32_t)std::llround(16*std::log2(x/16.0));}

// floor(x/m) for |x| < 2^15, 1 <= m <= 127, by multiply-shift (exactness checked exhaustively in tests/evo/test_egg.cpp).
struct Div{std::uint64_t mg[128];Div(){mg[0]=0;for(int m=1;m<128;++m)mg[m]=(1ULL<<32)/m+1;}};  // 64-bit: m = 1 needs 2^32 + 1
const Div& divs(){static Div d;return d;}
// branch-free: floor(x/m) = x>=0 ? x/m : -((-x+m-1)/m), each unsigned quotient by multiply-shift
inline int fdiv_g(int x,int m,std::uint64_t g){const int t=x>=0?x:m-1-x;const int q=(int)(((std::uint64_t)t*g)>>32);return x>=0?q:-q;}
inline int fdiv(int x,int m){return fdiv_g(x,m,divs().mg[m]);}
}  // namespace

long Egg::nparams() const{long n=0;for(auto* m:mats)n+=(long)m->m*m->n;for(auto* v:vecs)n+=v->n;return n;}

Egg::Egg(const EggCfg& cc,std::uint64_t seed):c(cc){
  const int D=c.D,V=c.V;ilog2(D);if((ilog2(D)&1))throw std::runtime_error("EGG: D must be a power of 4");
  if(c.Din<0||c.Din%64||V%16)throw std::runtime_error("EGG: Din must be a multiple of 64, V of 16");
  auto mat=[&](Mat& M,int m,int n){M.m=m;M.n=n;M.id=(int)mats.size();mats.push_back(&M);};
  auto vec=[&](Vec& v,int n,int init){v.n=n;v.id=(int)vecs.size();vecs.push_back(&v);v.w.assign(n,(std::int8_t)init);};
  ly.resize(c.L);
  if(c.Din==0)mat(emb,V,D);else mat(inp,D,c.Din);  // same registry order as before for the language model
  mat(head,V,D);vec(lnout,D,16);
  for(auto& l:ly){vec(l.ln1,D,16);vec(l.ln2,D,16);vec(l.bf,D,0);vec(l.bh,D,0);
    mat(l.Wf,D,D);mat(l.Uf,D,D);mat(l.Wh,D,D);mat(l.Uh,D,D);mat(l.m1,4*D,D);mat(l.m2,D,4*D);}
  for(auto* M:mats){M->w.resize((std::size_t)M->m*M->n);fill_gauss(M->w.data(),M->w.size(),mix(seed*0x100000001b3ULL+(std::uint64_t)M->id)<<20);}
  repack();
}

void Egg::repack(){
  for(auto* M:mats){if(M==&emb)continue;  // emb is a lookup table (inp is a matmul and is packed)
    std::vector<std::int8_t> t((std::size_t)M->n*M->m);for(int i=0;i<M->m;++i)for(int j=0;j<M->n;++j)t[(std::size_t)j*M->m+i]=M->w[(std::size_t)i*M->n+j];
    M->p=pack_b(t.data(),M->n,M->m);}
}

void Egg::save(const std::string& fn,long step) const{
  std::FILE* f=std::fopen(fn.c_str(),"wb");if(!f)throw std::runtime_error("cannot write "+fn);save(f,step);std::fclose(f);
}
void Egg::save(std::FILE* f,long step) const{
  const std::uint64_t magic=0x32474745ULL;std::fwrite(&magic,8,1,f);std::fwrite(&c,sizeof c,1,f);std::fwrite(&step,sizeof step,1,f);
  for(auto* M:mats){std::fwrite(M->w.data(),1,M->w.size(),f);}
  for(auto* v:vecs){std::fwrite(v->w.data(),1,v->w.size(),f);}
}
long Egg::load(const std::string& fn){
  std::FILE* f=std::fopen(fn.c_str(),"rb");if(!f)throw std::runtime_error("cannot read "+fn);
  long step=-1;try{step=load(f);}catch(...){std::fclose(f);throw std::runtime_error("bad EGG checkpoint "+fn);}std::fclose(f);return step;
}
long Egg::load(std::FILE* f){
  std::uint64_t magic=0;EggCfg cc;long step=0;bool ok=std::fread(&magic,8,1,f)==1&&(magic==0x31474745ULL||magic==0x32474745ULL);
  if(ok)ok=std::fread(&cc,magic==0x31474745ULL?4*sizeof(int):sizeof cc,1,f)==1;  // v1: {L,D,V,sig}, Din = 0
  if(ok&&magic==0x31474745ULL)cc.Din=0;
  ok=ok&&std::fread(&step,sizeof step,1,f)==1&&cc.L==c.L&&cc.D==c.D&&cc.V==c.V&&cc.Din==c.Din;
  for(auto* M:mats){ok=ok&&std::fread(M->w.data(),1,M->w.size(),f)==M->w.size();}
  for(auto* v:vecs){ok=ok&&std::fread(v->w.data(),1,v->w.size(),f)==v->w.size();}
  if(!ok){throw std::runtime_error("bad EGG record");}c.sig=cc.sig;repack();return step;
}

int egg_fdiv(int x,int m){return fdiv(x,m);}

double egg_macs_per_token(const EggCfg& c){return (double)c.L*12*c.D*c.D+(double)c.V*c.D+(double)c.Din*c.D;}

void Noise::gen(const Egg& g,int PP,std::uint64_t seed,long step){
  P=PP;const std::size_t K=g.mats.size()+g.vecs.size();offA.resize(K);offB.resize(K);lenA.resize(K);lenB.resize(K);
  std::size_t off=0;
  for(std::size_t k=0;k<K;++k){
    if(k<g.mats.size()){lenA[k]=g.mats[k]->m;lenB[k]=g.mats[k]->n;}else{lenA[k]=lenB[k]=g.vecs[k-g.mats.size()]->n;}
    offA[k]=off;off+=(std::size_t)P*lenA[k];offB[k]=off;off+=(std::size_t)P*lenB[k];}
  buf.resize(off);
  const std::uint64_t base=mix(mix(seed^0x6e6f697365ULL)+(std::uint64_t)step);
  // one key per (param, pair, A/B): pairs are independent streams, so the noise does not depend on thread count
  for(std::size_t k=0;k<K;++k)for(int p=0;p<P;++p){
    fill_gauss(buf.data()+offA[k]+(std::size_t)p*lenA[k],lenA[k],mix(base+(k<<40)+((std::uint64_t)p<<1))<<16);
    fill_gauss(buf.data()+offB[k]+(std::size_t)p*lenB[k],lenB[k],mix(base+(k<<40)+((std::uint64_t)p<<1)+1)<<16);}
}

void Rows::init(const Egg& g,int RR){R=RR;pair.assign(R,0);sign.assign(R,0);state.assign((std::size_t)R*g.c.L*g.c.D,0);}

namespace {
// Per-thread forward over rows [r0, r0+n) (n a multiple of 32 or the tail), all T tokens.
struct Fwd{
  const Egg& g;const Noise* nz;Rows& rows;int r0,n;int D,sh;
  const int* mp=nullptr;int nn=0;  // step over a subset: compacted row i is local row mp[i] (nn rows); else all n rows
  int R(int i) const{return r0+(mp?mp[i]:i);}
  std::vector<std::int8_t> y,u,z,f,fh,a,b2;std::vector<std::int32_t> acc;
  std::vector<int> voff;int vtot=0;std::vector<std::int8_t> pvb;  // perturbed vector parameters per row (fixed within a step)
  Fwd(const Egg& gg,const Noise* z0,Rows& rr,int r,int nn):g(gg),nz(z0),rows(rr),r0(r),n(nn),D(gg.c.D),sh(4+gg.c.sig){
    const int npad=(n+15)/16*16;y.assign((std::size_t)npad*D,0);u=y;f=y;fh=y;a=y;b2=y;z.assign((std::size_t)npad*4*D,0);
    acc.assign((std::size_t)64*std::max(4*D,gg.c.V),0);
    for(auto* v:g.vecs){voff.push_back(vtot);vtot+=v->n;}
    pvb.resize((std::size_t)n*vtot);
    for(int i=0;i<n;++i){const int r=r0+i,s=rows.sign[r];
      for(auto* v:g.vecs){std::int8_t* o=pvb.data()+(std::size_t)i*vtot+voff[v->id];
        if(!nz||!s){std::memcpy(o,v->w.data(),v->n);continue;}
        const int k=(int)g.mats.size()+v->id,p=rows.pair[r];const std::int8_t* A=nz->A(k,p);const std::int8_t* B=nz->B(k,p);
        for(int j=0;j<v->n;++j)o[j]=(std::int8_t)std::clamp((int)v->w[j]+((s*(int)A[j]*(int)B[j])>>sh),-127,127);}}
  }
  // perturbed vector parameter value for row r
  const std::int8_t* pvr(const Vec& v,int r) const{return pvb.data()+(std::size_t)(r-r0)*vtot+voff[v.id];}
  int pv(const Vec& v,int i,int r) const{return pvr(v,r)[i];}
  // out (n x M.m int8) = I8((U M^T + ((U.B) A >> sh)) / (16 sqrt(M.n))). Rows go in blocks of 64 so the int32
  // accumulator block stays in cache; the noise term and the int8 conversion are fused into one pass.
  void mm(const Mat& M,const std::int8_t* U,std::int8_t* out,std::int32_t* c){
    const int ds=4+ilog2(M.n)/2,npad=(nn+15)/16*16;
    for(int b0=0;b0<npad;b0+=64){const int bn=std::min(64,npad-b0);
      gemm(U+(std::size_t)b0*M.n,M.n,M.p.data(),c,M.m,bn,M.m,M.n);
      for(int bi=0;bi<bn&&b0+bi<nn;++bi){const int i=b0+bi,r=R(i),s=nz?rows.sign[r]:0;const std::int32_t* ci=c+(std::size_t)bi*M.m;std::int8_t* oi=out+(std::size_t)i*M.m;
        if(!s){for(int o=0;o<M.m;++o)oi[o]=c8i(ci[o]>>ds);continue;}
        const int p=rows.pair[r];const std::int8_t* B=nz->B(M.id,p);const std::int8_t* A=nz->A(M.id,p);const std::int8_t* x=U+(std::size_t)i*M.n;
        std::int32_t dot=0;for(int j=0;j<M.n;++j)dot+=(std::int32_t)x[j]*B[j];
        if(M.n<=1024){const std::int32_t sd=s*dot;for(int o=0;o<M.m;++o)oi[o]=c8i((ci[o]+((sd*(std::int32_t)A[o])>>sh))>>ds);}  // |dot*A| < 1024*127^3 < 2^31
        else{const std::int64_t sd=(std::int64_t)s*dot;for(int o=0;o<M.m;++o)oi[o]=c8i((ci[o]+(std::int32_t)((sd*A[o])>>sh))>>ds);}}
    }
  }
  void ln(const Vec& w,const std::int8_t* in,std::int8_t* out){
    const int s2=ilog2(D);
    for(int i=0;i<nn;++i){const std::int8_t* x=in+(std::size_t)i*D;std::int8_t* o=out+(std::size_t)i*D;
      std::int32_t sum=0;for(int j=0;j<D;++j)sum+=std::abs((int)x[j]);const int m=std::max(1,std::min(127,(int)(sum>>s2)));
      const std::int8_t* wv=pvr(w,R(i));const std::uint64_t gm=divs().mg[m];for(int j=0;j<D;++j)o[j]=c8i(fdiv_g((int)x[j]*(int)wv[j],m,gm));}
  }
  std::vector<std::int8_t> lg,ob;  // logits (npad x V); padded observations (npad x Din)
  // One forward step for all n rows, or for the m rows listed in idx (local indices; compacted inputs and outputs).
  // tk: input bytes (Din == 0) or obs: rows x Din (Din > 0). Logits go to lg.
  void step(const std::uint8_t* tk,const std::int8_t* obs,const int* idx=nullptr,int m=0){
    mp=idx;nn=idx?m:n;
    const int L=g.c.L,V=g.c.V,npad=(nn+15)/16*16;if(lg.size()<(std::size_t)npad*V)lg.assign((std::size_t)npad*V,0);
    if(g.c.Din>0){if(ob.size()<(std::size_t)npad*g.c.Din)ob.assign((std::size_t)npad*g.c.Din,0);
      std::memcpy(ob.data(),obs,(std::size_t)nn*g.c.Din);mm(g.inp,ob.data(),y.data(),acc.data());}
    else for(int i=0;i<nn;++i){const int r=R(i);const int t0=tk[i];std::int8_t* yi=y.data()+(std::size_t)i*D;
        const std::int8_t* w=g.emb.w.data()+(std::size_t)t0*D;const int s=rows.sign[r];
        if(nz&&s){const int p=rows.pair[r];const int at=s*(int)nz->A(g.emb.id,p)[t0];const std::int8_t* B=nz->B(g.emb.id,p);
          for(int j=0;j<D;++j)yi[j]=c8i((int)w[j]+((at*(int)B[j])>>sh));}
        else std::memcpy(yi,w,D);}
    {
      for(int l=0;l<L;++l){const auto& Ly=g.ly[l];
        ln(Ly.ln1,y.data(),u.data());
        // GRU (Appendix G.8)
        mm(Ly.Wf,u.data(),f.data(),acc.data());
        for(int i=0;i<nn;++i)std::memcpy(a.data()+(std::size_t)i*D,rows.state.data()+((std::size_t)R(i)*L+l)*D,D);  // s
        mm(Ly.Uf,a.data(),b2.data(),acc.data());
        for(int i=0;i<nn;++i){std::int8_t* fi=f.data()+(std::size_t)i*D;const std::int8_t* us=b2.data()+(std::size_t)i*D;const std::int8_t* s=a.data()+(std::size_t)i*D;std::int8_t* hh=fh.data()+(std::size_t)i*D;
          const std::int8_t* bf=pvr(Ly.bf,R(i));for(int j=0;j<D;++j){fi[j]=c8i((int)fi[j]+(int)us[j]+(int)bf[j]);hh[j]=c8i((((int)fi[j]+127)*(int)s[j])>>8);}}
        mm(Ly.Wh,u.data(),z.data(),acc.data());  // u@Wh (into z, first D columns used)
        mm(Ly.Uh,fh.data(),b2.data(),acc.data());
        for(int i=0;i<nn;++i){const std::int8_t* fi=f.data()+(std::size_t)i*D;const std::int8_t* wh=z.data()+(std::size_t)i*D;const std::int8_t* uh=b2.data()+(std::size_t)i*D;
          std::int8_t* s=rows.state.data()+((std::size_t)R(i)*L+l)*D;std::int8_t* yi=y.data()+(std::size_t)i*D;
          const std::int8_t* bh=pvr(Ly.bh,R(i));for(int j=0;j<D;++j){const int hh=c8i((int)wh[j]+(int)uh[j]+(int)bh[j]);
            const int h=c8i((int)s[j]+(int)c8i((((int)fi[j]+127)*(hh-(int)s[j]))>>8));s[j]=(std::int8_t)h;yi[j]=c8i((int)yi[j]+h);}}
        // MLP (Appendix G.7)
        ln(Ly.ln2,y.data(),u.data());
        mm(Ly.m1,u.data(),z.data(),acc.data());
        mm(Ly.m2,z.data(),a.data(),acc.data());
        for(std::size_t k=0;k<(std::size_t)nn*D;++k)y[k]=c8i((int)y[k]+(int)a[k]);
      }
      ln(g.lnout,y.data(),u.data());mm(g.head,u.data(),lg.data(),acc.data());
    }
  }
  void run(const std::uint8_t* tok,int T,std::int64_t* fit){
    const int V=g.c.V;const std::int32_t* E2=exp2_table();std::vector<std::uint8_t> tk(n);
    for(int t=0;t<T;++t){
      for(int i=0;i<n;++i)tk[i]=tok[(std::size_t)(r0+i)*(T+1)+t];
      step(tk.data(),nullptr);
      for(int i=0;i<n;++i){const int r=r0+i;const int tgt=tok[(std::size_t)r*(T+1)+t+1];const std::int8_t* li=lg.data()+(std::size_t)i*V;
        std::int64_t se=0;for(int v=0;v<V;++v)se+=E2[(int)li[v]+128];fit[r]+=(int)li[tgt]+128-log2_16(se);}
    }
  }
};
}  // namespace

struct Stepper::Impl{Fwd f;Impl(const Egg& g,const Noise* nz,Rows& rows,int r0,int n):f(g,nz,rows,r0,n){}};
Stepper::Stepper(const Egg& g,const Noise* nz,Rows& rows,int r0,int n):p_(new Impl(g,nz,rows,r0,n)){if(g.c.Din<=0)throw std::runtime_error("Stepper needs Din > 0");}
Stepper::~Stepper(){delete p_;}
const std::int8_t* Stepper::step(const std::int8_t* obs){p_->f.step(nullptr,obs);return p_->f.lg.data();}
const std::int8_t* Stepper::step(const std::int8_t* obs,const int* idx,int m){p_->f.step(nullptr,obs,idx,m);return p_->f.lg.data();}

void run_rows(const Egg& g,const Noise* nz,Rows& rows,const std::uint8_t* tok,int T,std::int64_t* fit,int threads){
  const int R=rows.R;const int blocks=(R+31)/32;threads=std::max(1,std::min(threads,blocks));
  std::vector<std::thread> th;
  for(int t=0;t<threads;++t){const int b0=blocks*t/threads,b1=blocks*(t+1)/threads;const int r0=b0*32,r1=std::min(R,b1*32);if(r1<=r0)continue;
    th.emplace_back([&,r0,r1]{Fwd w(g,nz,rows,r0,r1-r0);w.run(tok,T,fit);});}
  for(auto& x:th)x.join();
}

long egg_update(Egg& g,const Noise& nz,const std::vector<int>& F,double alpha){
  const int P=nz.P;const int Pp=(P+63)/64*64;long changed=0;
  const std::int32_t thr=(std::int32_t)(16*invphi(1-alpha/2)*16*std::sqrt((double)P));
  for(auto* M:g.mats){const int k=M->id,m=M->m,n=M->n;
    std::vector<std::int8_t> Lt((std::size_t)m*Pp,0),Bt((std::size_t)Pp*n,0);
    for(int p=0;p<P;++p){if(!F[p])continue;const std::int8_t* A=nz.A(k,p);for(int i=0;i<m;++i)Lt[(std::size_t)i*Pp+p]=(std::int8_t)(F[p]*A[i]);
      std::memcpy(Bt.data()+(std::size_t)p*n,nz.B(k,p),n);}
    auto Bp=pack_b(Bt.data(),Pp,n);std::vector<std::int32_t> E((std::size_t)m*n);gemm(Lt.data(),Pp,Bp.data(),E.data(),n,m,n,Pp);
    for(std::size_t i=0;i<E.size();++i)if(E[i]>thr||E[i]<-thr){const int w=std::clamp((int)M->w[i]+(E[i]>0?1:-1),-127,127);if(w!=M->w[i]){M->w[i]=(std::int8_t)w;++changed;}}
  }
  for(auto* v:g.vecs){const int k=(int)g.mats.size()+v->id;
    for(int i=0;i<v->n;++i){std::int64_t e=0;for(int p=0;p<P;++p)if(F[p])e+=F[p]*(int)nz.A(k,p)[i]*(int)nz.B(k,p)[i];
      if(e>thr||e<-thr){const int w=std::clamp((int)v->w[i]+(e>0?1:-1),-127,127);if(w!=v->w[i]){v->w[i]=(std::int8_t)w;++changed;}}}
  }
  g.repack();return changed;
}

}  // namespace oee::evo
