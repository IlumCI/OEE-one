// int8 x int8 -> int32 matrix multiply for the evolved models (docs/EVO.md).
// Uses Intel AMX tiles (TDPBSSD) when the CPU and kernel allow it, otherwise a scalar reference.
// C[MxN] (+)= A[MxK] * B[KxN], A row-major, B pre-packed with pack_b (VNNI layout: 4 consecutive k per column).
// Shapes: M and N multiples of 16, K multiple of 64 (callers pad).
#pragma once
#include <cstdint>
#include <vector>

namespace oee::evo {

bool amx_on();  // true if AMX is usable in this process (permission requested on first call)
void use_amx(bool on);  // force the scalar path (tests, benchmarks); no effect if AMX is unavailable

// Packed B: (K/4) rows of N*4 bytes; Bp[(k/4)*N*4 + n*4 + k%4] = B[k*N + n].
std::vector<std::int8_t> pack_b(const std::int8_t* B,int K,int N);

// C = A*B (acc=false) or C += A*B (acc=true). lda = row stride of A in bytes, ldc = row stride of C in int32.
void gemm(const std::int8_t* A,int lda,const std::int8_t* Bp,std::int32_t* C,int ldc,int M,int N,int K,bool acc=false);
// Same, rows of A split over T threads (each thread a contiguous block of 16-row stripes).
void gemm_mt(const std::int8_t* A,int lda,const std::int8_t* Bp,std::int32_t* C,int ldc,int M,int N,int K,int T,bool acc=false);

// Reference on unpacked B (KxN row-major), for tests.
void gemm_ref(const std::int8_t* A,int lda,const std::int8_t* B,std::int32_t* C,int ldc,int M,int N,int K,bool acc=false);

}  // namespace oee::evo
