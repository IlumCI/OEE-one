// Evolvable genetic code checks: (1) with the standard code in both tables, mode 2 equals the fixed-code
// control (mode 1) on every tape whose tables are not rewritten during the run; (2) a program in a
// non-standard code runs under its own table and is inert under the standard one; (3) a checkpoint keeps
// the mode.
#include <cstdio>
#include <cstring>
#include "bff/soup.hpp"
using namespace oee::bff;
static const char kStd[kCodon+1]="[]+-.,<>{}";
int main(){
int bad=0,same=0,rewritten=0;
std::uint8_t a[kPair],b[kPair];
for(std::uint64_t t=0;t<20000;++t){
for(int i=0;i<kPair;++i)a[i]=std::uint8_t(splitmix64(t*kPair+i));
for(int h=0;h<2;++h)std::memcpy(a+h*kTape,kStd,kCodon);
std::memcpy(b,a,kPair);
eval_egc(a,1);eval_egc(b,2);
const bool tab=!std::memcmp(b,kStd,kCodon)&&!std::memcmp(b+kTape,kStd,kCodon)&&!std::memcmp(a,kStd,kCodon)&&!std::memcmp(a+kTape,kStd,kCodon);
if(!tab){++rewritten;continue;}
if(std::memcmp(a,b,kPair))++bad;else ++same;
}
// table use: a program written in a non-standard code ('>' = 207, '+' = 202) runs under its own table
// and is inert under the standard code.
const std::uint8_t alt[kCodon]={200,201,202,203,204,205,206,207,208,209};
auto prog=[&](std::uint8_t*t,const std::uint8_t*tab,std::uint8_t gt,std::uint8_t plus){
std::memset(t,0,kPair);for(int h=0;h<2;++h)std::memcpy(t+h*kTape,tab,kCodon);
int p=kCodon;for(int k=0;k<12;++k)t[p++]=gt;t[p++]=plus;t[30]=1;};
prog(a,alt,207,202);eval_egc(a,2);
const int run_ok=a[12]==208&&a[30]==1;  // h0 moved 12 cells, incremented tape[12] (a '>' cell, 207 -> 208)
prog(b,(const std::uint8_t*)kStd,207,202);eval_egc(b,2);
const int inert_ok=b[12]==207;
std::printf("table use: own code runs %s, standard code inert %s\n",run_ok?"yes":"no",inert_ok?"yes":"no");
std::printf("unchanged-table tapes: %d equal, %d differ; rewritten %d\n",same,bad,rewritten);
Params p;p.num_programs=512;p.seed=7;p.egc=2;
Soup s(p);for(int e=0;e<5;++e)s.epoch();
s.save("egc_ck.bin");Soup r=Soup::load("egc_ck.bin",1);
const int ck=r.params().egc==2&&r.epochs_done()==5&&r.data()==s.data();
std::printf("checkpoint keeps mode: %s\n",ck?"yes":"no");
return (bad||!same||!ck||!run_ok||!inert_ok)?1:0;
}
