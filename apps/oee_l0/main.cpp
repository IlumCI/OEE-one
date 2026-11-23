// L0 runner. Usage: oee_l0 [-n 256] [-seed S] [-steps T] [-beta B] [-kernels K] [-mut P] [-muts S]
//   [-patches C] [-radius R] [-density D] [-every E] [-dump DIR] [-log FILE] [-sample 0|1]
//   [-qdump FILE -qfrom T0]  per-step u8 cell states from T0 on (0 empty, else 1+3*(dominant kernel)+density level 0-2)
// Kernel bank: K random kernels drawn from the Flow-Lenia ranges (Table 1), seeded by -seed.
// Log line every E steps: t total_mass max_density blobs mean_blob_area param_sd hash
// Dump: DIR/f<t>.bin = u16 n, then n*n float32 density, then n*n*K float32 parameters.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "l0/world.hpp"

using namespace oee::l0;

static std::uint64_t sm(std::uint64_t& s){std::uint64_t z=(s+=0x9e3779b97f4a7c15ULL);z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;z=(z^(z>>27))*0x94d049bb133111ebULL;return z^(z>>31);}
static double ur(std::uint64_t& s,double a,double b){return a+(b-a)*((sm(s)>>11)*(1.0/9007199254740992.0));}

int main(int argc,char** argv){
  Params p;int K=3,steps=2000,patches=8,radius=12,every=50;double density=.6;std::string dump,logf,qdump;long qfrom=0;
  for(int a=1;a<argc;a+=2){
    const char* s=argv[a];const char* v=a+1<argc?argv[a+1]:"0";
    if(!std::strcmp(s,"-n"))p.n=std::atoi(v);else if(!std::strcmp(s,"-seed"))p.seed=std::strtoull(v,nullptr,10);
    else if(!std::strcmp(s,"-steps"))steps=std::atoi(v);else if(!std::strcmp(s,"-beta"))p.beta=std::atof(v);
    else if(!std::strcmp(s,"-kernels"))K=std::atoi(v);else if(!std::strcmp(s,"-mut"))p.mut_p=std::atof(v);
    else if(!std::strcmp(s,"-muts"))p.mut_s=std::atof(v);else if(!std::strcmp(s,"-patches"))patches=std::atoi(v);
    else if(!std::strcmp(s,"-radius"))radius=std::atoi(v);else if(!std::strcmp(s,"-density"))density=std::atof(v);
    else if(!std::strcmp(s,"-every"))every=std::atoi(v);else if(!std::strcmp(s,"-dump"))dump=v;
    else if(!std::strcmp(s,"-log"))logf=v;else if(!std::strcmp(s,"-sample"))p.sample=std::atoi(v)!=0;
    else if(!std::strcmp(s,"-qdump"))qdump=v;else if(!std::strcmp(s,"-qfrom"))qfrom=std::atol(v);else{std::fprintf(stderr,"unknown option %s\n",s);return 2;}
  }
  std::uint64_t rs=p.seed*0x2545F4914F6CDD1DULL+1;p.bank.clear();
  for(int k=0;k<K;++k){Kernel kb;kb.R=ur(rs,6,18);for(int j=0;j<3;++j){kb.a[j]=ur(rs,0,1);kb.b[j]=ur(rs,0,1);kb.w[j]=ur(rs,.05,.5);}
    kb.mu=ur(rs,.05,.5);kb.sigma=ur(rs,.01,.2);p.bank.push_back(kb);}
  World w(p);w.seed_patches(patches,radius,density);
  std::FILE* lg=logf.empty()?stdout:std::fopen(logf.c_str(),"w");if(!lg)return 2;
  for(int k=0;k<K;++k){const Kernel& b=p.bank[k];std::fprintf(lg,"# kernel %d R %.3f mu %.4f sigma %.4f a %.3f %.3f %.3f b %.3f %.3f %.3f w %.3f %.3f %.3f\n",k,b.R,b.mu,b.sigma,b.a[0],b.a[1],b.a[2],b.b[0],b.b[1],b.b[2],b.w[0],b.w[1],b.w[2]);}
  const int n=p.n,N=n*n;
  std::FILE* qf=qdump.empty()?nullptr:std::fopen(qdump.c_str(),"wb");std::vector<unsigned char> q(N);
  for(int t=0;t<=steps;++t){
    if(t%every==0){
      double mx=0;for(auto v:w.mass())mx=std::max(mx,(double)v/kOne);
      // blobs: 4-connected components of density > 0.1*max
      std::vector<int> lab(N,0);int nb=0;long area=0;std::vector<int> st;const double th=.1*mx;
      for(int c=0;c<N;++c){if(lab[c]||(double)w.mass()[c]/kOne<=th)continue;++nb;st.push_back(c);lab[c]=nb;
        while(!st.empty()){int u=st.back();st.pop_back();++area;int x=u%n,y=u/n;int nbh[4]={y*n+(x+1)%n,y*n+(x+n-1)%n,((y+1)%n)*n+x,((y+n-1)%n)*n+x};
          for(int q:nbh)if(!lab[q]&&(double)w.mass()[q]/kOne>th){lab[q]=nb;st.push_back(q);}}}
      double s1=0,s2=0,wt=0;for(int c=0;c<N;++c){const double m=(double)w.mass()[c];if(m<=0)continue;for(int k=0;k<K;++k){const double v=w.pars()[(std::size_t)c*K+k];s1+=m*v;s2+=m*v*v;}wt+=m*K;}
      const double sd=wt>0?std::sqrt(std::max(0.0,s2/wt-(s1/wt)*(s1/wt))):0;
      std::fprintf(lg,"%d %lld %.4f %d %.1f %.4f %016llx\n",t,(long long)w.total(),mx,nb,nb?(double)area/nb:0.0,sd,(unsigned long long)w.hash());
      std::fflush(lg);
      if(!dump.empty()){
        char fn[512];std::snprintf(fn,sizeof fn,"%s/f%07d.bin",dump.c_str(),t);std::FILE* f=std::fopen(fn,"wb");
        if(f){const unsigned short nn=(unsigned short)n;std::fwrite(&nn,2,1,f);std::vector<float> d(N);for(int c=0;c<N;++c)d[c]=(float)((double)w.mass()[c]/kOne);
          std::fwrite(d.data(),4,N,f);std::fwrite(w.pars().data(),4,w.pars().size(),f);std::fclose(f);}
      }
    }
    if(qf&&t>=qfrom){
      for(int c=0;c<N;++c){const double d=(double)w.mass()[c]/kOne;if(d<.05){q[c]=0;continue;}
        int kb=0;for(int k=1;k<K;++k)if(w.pars()[(std::size_t)c*K+k]>w.pars()[(std::size_t)c*K+kb])kb=k;
        q[c]=(unsigned char)(1+3*kb+(d<.3?0:d<.8?1:2));}
      std::fwrite(q.data(),1,N,qf);}
    if(t<steps)w.step();
  }
  if(lg!=stdout)std::fclose(lg);
  if(qf)std::fclose(qf);
}
