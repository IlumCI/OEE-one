// AMX int8 GEMM vs scalar reference, plus throughput (docs/EVO.md).
#include <chrono>
#include <cstdio>
#include <random>
#include <vector>

#include "evo/gemm.hpp"

using namespace oee::evo;
static int fails=0;
#define CHECK(c,msg) do{if(!(c)){std::printf("FAIL %s\n",msg);++fails;}else std::printf("ok   %s\n",msg);}while(0)

int main(int argc,char**){
  std::mt19937 r(1);std::uniform_int_distribution<int> d(-128,127);
  std::printf("amx available: %d\n",(int)amx_on());
  for(auto [M,N,K]:std::vector<std::tuple<int,int,int>>{{16,16,64},{48,80,192},{64,32,128},{32,48,64}}){
    std::vector<std::int8_t> A((std::size_t)M*K),B((std::size_t)K*N);for(auto& x:A)x=(std::int8_t)d(r);for(auto& x:B)x=(std::int8_t)d(r);
    std::vector<std::int32_t> C0((std::size_t)M*N),C1((std::size_t)M*N,7),C2((std::size_t)M*N,7);
    gemm_ref(A.data(),K,B.data(),C0.data(),N,M,N,K);auto Bp=pack_b(B.data(),K,N);
    gemm(A.data(),K,Bp.data(),C1.data(),N,M,N,K);gemm_mt(A.data(),K,Bp.data(),C2.data(),N,M,N,K,3);
    char msg[96];std::snprintf(msg,sizeof msg,"gemm %dx%dx%d matches reference (1 and 3 threads)",M,N,K);CHECK(C0==C1&&C0==C2,msg);
    std::vector<std::int32_t> C3=C0;gemm(A.data(),K,Bp.data(),C3.data(),N,M,N,K,true);bool ok=true;for(std::size_t i=0;i<C3.size();++i)if(C3[i]!=2*C0[i])ok=false;
    std::snprintf(msg,sizeof msg,"gemm %dx%dx%d accumulate",M,N,K);CHECK(ok,msg);
  }
  if(argc>1){  // benchmark
    const int M=1024,N=1024,K=1024;std::vector<std::int8_t> A((std::size_t)M*K),B((std::size_t)K*N);for(auto& x:A)x=(std::int8_t)d(r);for(auto& x:B)x=(std::int8_t)d(r);
    auto Bp=pack_b(B.data(),K,N);std::vector<std::int32_t> C((std::size_t)M*N);
    for(int T:{1,4}){auto t0=std::chrono::steady_clock::now();int it=0;
      while(std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count()<2){gemm_mt(A.data(),K,Bp.data(),C.data(),N,M,N,K,T);++it;}
      const double s=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
      std::printf("1024^3 int8 gemm, %d thread(s): %.2f TOPS\n",T,2.0*M*N*K*it/s/1e12);}
  }
  std::printf(fails?"%d FAILED\n":"all passed\n",fails);return fails?1:0;
}
