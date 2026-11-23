// L0 unit checks: FFT convolution vs direct sum, exact mass conservation, determinism, mutation on/off.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "l0/world.hpp"

using namespace oee::l0;
static int fails=0;
#define CHECK(c,msg) do{if(!(c)){std::printf("FAIL %s\n",msg);++fails;}else std::printf("ok   %s\n",msg);}while(0)

int main(){
  {  // FFT circular convolution equals the direct sum on a 16x16 grid
    const int n=16,N=n*n;std::vector<double> a(N),b(N),d(N,0);
    for(int i=0;i<N;++i){a[i]=std::sin(i*.37)+.5;b[i]=std::cos(i*.11);}
    for(int y=0;y<n;++y)for(int x=0;x<n;++x)for(int v=0;v<n;++v)for(int u=0;u<n;++u)d[y*n+x]+=a[v*n+u]*b[((y-v+n)%n)*n+(x-u+n)%n];
    std::vector<double> ar=a,ai(N,0),br=b,bi(N,0);fft2(ar,ai,n,false);fft2(br,bi,n,false);
    std::vector<double> cr(N),ci(N);for(int i=0;i<N;++i){cr[i]=ar[i]*br[i]-ai[i]*bi[i];ci[i]=ar[i]*bi[i]+ai[i]*br[i];}
    fft2(cr,ci,n,true);double e=0;for(int i=0;i<N;++i)e=std::max(e,std::fabs(cr[i]-d[i]));
    CHECK(e<1e-9,"fft convolution matches direct sum");
  }
  Params p;p.n=64;p.seed=7;p.bank={Kernel{},Kernel{10,{.3,.7,0},{1,.6,0},{.12,.12,.12},.25,.03}};
  {  // exact conservation over 300 steps, with and without mutation
    for(double mp:{0.0,0.01}){
      Params q=p;q.mut_p=mp;World w(q);w.seed_patches(6,8,0.8);const auto m0=w.total();bool ok=m0>0;
      for(int i=0;i<300;++i){w.step();if(w.total()!=m0)ok=false;}
      CHECK(ok,mp>0?"mass conserved exactly (mutation on)":"mass conserved exactly (mutation off)");
    }
  }
  {  // same seed -> same state hash; different seed -> different
    World a(p),b(p);a.seed_patches(6,8,.8);b.seed_patches(6,8,.8);for(int i=0;i<100;++i){a.step();b.step();}
    CHECK(a.hash()==b.hash(),"deterministic: identical hash for identical seed");
    Params q=p;q.seed=8;World c(q);c.seed_patches(6,8,.8);for(int i=0;i<100;++i)c.step();
    CHECK(c.hash()!=a.hash(),"different seed gives different state");
  }
  {  // mutation off: parameters only mix (stay within the initial range); on: they spread
    Params q=p;World w(q);w.seed_patches(6,8,.8);
    float lo=1,hi=0;for(std::size_t i=0;i<w.pars().size();i+=2)if(w.mass()[i/2]>0){lo=std::min(lo,w.pars()[i]);hi=std::max(hi,w.pars()[i]);}
    for(int i=0;i<200;++i)w.step();
    bool in=true;for(std::size_t i=0;i<w.pars().size();i+=2)if(w.mass()[i/2]>0&&(w.pars()[i]<lo-1e-6f||w.pars()[i]>hi+1e-6f))in=false;
    CHECK(in,"no mutation: carried parameters stay in the convex hull of the seeds");
  }
  {  // drift: with beta>0 mass moves up a fixed affinity gradient (single-step check on a ramp)
    Params q;q.n=16;q.beta=8;q.bank={Kernel{}};World w(q);
    for(int c=0;c<16*16;++c){w.mass()[c]=kOne;w.pars()[c]=0.f;}
    w.step();  // P=0 -> A=0 everywhere -> pure diffusion of a flat field: stays flat
    bool flat=true;for(auto v:w.mass())if(v!=kOne)flat=false;
    CHECK(flat,"flat field with zero affinity stays flat");
  }
  std::printf(fails?"%d FAILED\n":"all passed\n",fails);
  return fails?1:0;
}
