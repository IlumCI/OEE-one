#include "evo/gemm.hpp"

#include <cpuid.h>
#include <immintrin.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <thread>

namespace oee::evo {

namespace {
bool g_force_scalar=false;
int amx_state(){  // -1 unknown, 0 unavailable, 1 available
  static int s=[]{
    __builtin_cpu_init();
    unsigned a,b,c,d;if(!__get_cpuid_max(0,nullptr)||__get_cpuid_max(0,nullptr)<7)return 0;
    __cpuid_count(7,0,a,b,c,d);if(!((d>>24)&1)||!((d>>25)&1))return 0;  // AMX-TILE, AMX-INT8
    return syscall(SYS_arch_prctl,0x1023/*ARCH_REQ_XCOMP_PERM*/,18/*XFEATURE_XTILEDATA*/)==0?1:0;
  }();
  return s;
}
struct alignas(64) TileCfg{std::uint8_t palette=1,start=0;std::uint8_t r0[14]={};std::uint16_t colsb[16]={};std::uint8_t rows[16]={};};
thread_local bool t_cfg=false;
void cfg(){if(t_cfg)return;TileCfg c;for(int i=0;i<8;++i){c.colsb[i]=64;c.rows[i]=16;}_tile_loadconfig(&c);t_cfg=true;}

// One 32x32 block (2x2 tiles) or 16-wide remainders, AMX path.
void gemm_amx_kb(const std::int8_t* A,int lda,const std::int8_t* Bp,std::int32_t* C,int ldc,int M,int N,int K,bool acc,int ldb);
// K is split into blocks of up to kKB so that a 32-row A chunk and a 32-column B chunk stay in L1 while the block runs.
constexpr int kKB=512;
void gemm_amx(const std::int8_t* A,int lda,const std::int8_t* Bp,std::int32_t* C,int ldc,int M,int N,int K,bool acc){
  cfg();const int ldb=N*4;
  for(int k0=0;k0<K;k0+=kKB)gemm_amx_kb(A+k0,lda,Bp+(long)(k0/4)*ldb,C,ldc,M,N,std::min(kKB,K-k0),acc||k0>0,ldb);
}
void gemm_amx_kb(const std::int8_t* A,int lda,const std::int8_t* Bp,std::int32_t* C,int ldc,int M,int N,int K,bool acc,int ldb){
  for(int m=0;m<M;m+=32){const bool m2=m+16<M;
    for(int n=0;n<N;n+=32){const bool n2=n+16<N;
      std::int32_t* c00=C+(long)m*ldc+n;
      if(acc){_tile_loadd(0,c00,ldc*4);if(n2)_tile_loadd(1,c00+16,ldc*4);if(m2){_tile_loadd(2,c00+16L*ldc,ldc*4);if(n2)_tile_loadd(3,c00+16L*ldc+16,ldc*4);}}
      else{_tile_zero(0);_tile_zero(1);_tile_zero(2);_tile_zero(3);}
      for(int k=0;k<K;k+=64){
        _tile_loadd(4,A+(long)m*lda+k,lda);if(m2)_tile_loadd(5,A+(long)(m+16)*lda+k,lda);
        const std::int8_t* b=Bp+(long)(k/4)*ldb+n*4;_tile_loadd(6,b,ldb);if(n2)_tile_loadd(7,b+64,ldb);
        _tile_dpbssd(0,4,6);if(n2)_tile_dpbssd(1,4,7);if(m2){_tile_dpbssd(2,5,6);if(n2)_tile_dpbssd(3,5,7);}
      }
      _tile_stored(0,c00,ldc*4);if(n2)_tile_stored(1,c00+16,ldc*4);if(m2){_tile_stored(2,c00+16L*ldc,ldc*4);if(n2)_tile_stored(3,c00+16L*ldc+16,ldc*4);}
    }
  }
}
void gemm_scalar_packed(const std::int8_t* A,int lda,const std::int8_t* Bp,std::int32_t* C,int ldc,int M,int N,int K,bool acc){
  const int ldb=N*4;
  for(int m=0;m<M;++m)for(int n=0;n<N;++n){std::int32_t s=acc?C[(long)m*ldc+n]:0;
    for(int k=0;k<K;++k){s+=(std::int32_t)A[(long)m*lda+k]*Bp[(long)(k/4)*ldb+n*4+(k&3)];}
    C[(long)m*ldc+n]=s;}
}
}  // namespace

bool amx_on(){return !g_force_scalar&&amx_state()==1;}
void use_amx(bool on){g_force_scalar=!on;}

std::vector<std::int8_t> pack_b(const std::int8_t* B,int K,int N){
  std::vector<std::int8_t> p((std::size_t)K*N);
  for(int k=0;k<K;++k)for(int n=0;n<N;++n)p[(std::size_t)(k/4)*N*4+n*4+(k&3)]=B[(std::size_t)k*N+n];
  return p;
}

void gemm(const std::int8_t* A,int lda,const std::int8_t* Bp,std::int32_t* C,int ldc,int M,int N,int K,bool acc){
  if(amx_on())gemm_amx(A,lda,Bp,C,ldc,M,N,K,acc);else gemm_scalar_packed(A,lda,Bp,C,ldc,M,N,K,acc);
}

void gemm_mt(const std::int8_t* A,int lda,const std::int8_t* Bp,std::int32_t* C,int ldc,int M,int N,int K,int T,bool acc){
  const int stripes=M/32+(M%32?1:0);T=std::max(1,std::min(T,stripes));
  if(T==1){gemm(A,lda,Bp,C,ldc,M,N,K,acc);return;}
  std::vector<std::thread> th;
  for(int t=0;t<T;++t){const int s0=stripes*t/T,s1=stripes*(t+1)/T;const int m0=s0*32,m1=std::min(M,s1*32);if(m1<=m0)continue;
    th.emplace_back([=]{gemm(A+(long)m0*lda,lda,Bp,C+(long)m0*ldc,ldc,m1-m0,N,K,acc);});}
  for(auto& x:th)x.join();
}

void gemm_ref(const std::int8_t* A,int lda,const std::int8_t* B,std::int32_t* C,int ldc,int M,int N,int K,bool acc){
  for(int m=0;m<M;++m)for(int n=0;n<N;++n){std::int32_t s=acc?C[(long)m*ldc+n]:0;
    for(int k=0;k<K;++k){s+=(std::int32_t)A[(long)m*lda+k]*B[(long)k*N+n];}
    C[(long)m*ldc+n]=s;}
}

}  // namespace oee::evo
