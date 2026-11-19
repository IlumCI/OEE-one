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
// option 2: external standard tables reproduce plain BFF; a self-copy into B is authored by A.
int xbad=0;for(std::uint64_t t=0;t<20000;++t){
for(int i=0;i<kPair;++i)a[i]=std::uint8_t(splitmix64(~t*kPair+i));
std::memcpy(b,a,kPair);int wr[2];evaluate(a);eval_ext(b,(const std::uint8_t*)kStd,(const std::uint8_t*)kStd,wr);xbad+=std::memcmp(a,b,kPair)!=0;}
std::memset(a,0,kPair);{int wr[2];
int p0=0;a[p0++]='{';for(int k=0;k<20;++k){a[p0++]='.';a[p0++]='>';a[p0++]='{';}  // code in A copies 20 bytes into B (127 down)
(void)p0;eval_ext(a,(const std::uint8_t*)kStd,(const std::uint8_t*)kStd,wr);
std::printf("option 2: %d differ from plain BFF; authored-by-A bytes in B: %d\n",xbad,wr[1]);
xbad+=wr[1]<20;}
Params q;q.num_programs=512;q.seed=7;q.egc=4;Soup s4(q);for(int e=0;e<5;++e)s4.epoch();
s4.save("egc_ck.bin");Soup r4=Soup::load("egc_ck.bin",1);
const int ck4=r4.params().egc==4&&r4.data()==s4.data()&&r4.tables()==s4.tables();
std::printf("option 2 checkpoint keeps tables: %s\n",ck4?"yes":"no");
// option 3: a standard 256-entry map reproduces plain BFF; a synonym (second byte for '>') also moves the head.
std::uint8_t sm[kMap]={};for(int k=0;k<kCodon;++k)sm[std::uint8_t(kStd[k])]=std::uint8_t(k+1);
int mbad=0;for(std::uint64_t t=0;t<20000;++t){
for(int i=0;i<kPair;++i)a[i]=std::uint8_t(splitmix64(t*7+i));
std::memcpy(b,a,kPair);int wr[2];evaluate(a);eval_ext(b,sm,sm,wr,kMaxSteps,kMap);mbad+=std::memcmp(a,b,kPair)!=0;}
std::uint8_t syn[kMap];std::memcpy(syn,sm,kMap);syn[200]=sm[std::uint8_t('>')];
std::memset(a,0,kPair);for(int k=0;k<6;++k)a[k]=200;a[6]='+';{int wr[2];eval_ext(a,syn,syn,wr,kMaxSteps,kMap);}
const int syn_ok=a[6]=='+'+1;  // h0 moved 6 cells by the synonym, then '+' incremented tape[6]
Params q5;q5.num_programs=512;q5.seed=7;q5.egc=5;Soup s5(q5);for(int e=0;e<5;++e)s5.epoch();
s5.save("egc_ck.bin");Soup r5=Soup::load("egc_ck.bin",1);
const int ck5=r5.params().egc==5&&r5.tables().size()==512u*kMap&&r5.tables()==s5.tables()&&r5.data()==s5.data();
std::printf("option 3: %d differ from plain BFF; synonym works %s; checkpoint keeps maps %s\n",mbad,syn_ok?"yes":"no",ck5?"yes":"no");
return (bad||!same||!ck||!run_ok||!inert_ok||xbad||!ck4||mbad||!syn_ok||!ck5)?1:0;
}
