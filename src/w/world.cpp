#include "w/world.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace oee::w {

namespace {
std::uint64_t mix(std::uint64_t x){x+=0x9e3779b97f4a7c15ULL;x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;return x^(x>>31);}
const int FX[4]={0,1,0,-1},FY[4]={-1,0,1,0};  // dir 0 = north (y-1), clockwise
}  // namespace

const std::uint8_t kCode[16]={'<','>','{','}','+','-','.',',','[',']','E','D','S','C','W','R'};
namespace {
struct CodeIdx{std::uint8_t v[256];CodeIdx(){for(auto& x:v)x=255;for(int i=0;i<16;++i)v[kCode[i]]=(std::uint8_t)i;}};
const CodeIdx& code_idx(){static CodeIdx c;return c;}
}  // namespace

int act_of(const std::int8_t* l,int n){int b=0;for(int a=1;a<n;++a)if(l[a]>l[b])b=a;return b;}
int act_sample(const std::int8_t* l,std::uint64_t bits,int n){
  static std::int64_t E[256];static bool init=[]{for(int i=0;i<256;++i)E[i]=(std::int64_t)std::llround(16*std::exp2(i/16.0));return true;}();(void)init;
  std::int64_t w[32],tot=0;for(int a=0;a<n;++a){w[a]=E[(int)l[a]+128];tot+=w[a];}
  std::int64_t u=(std::int64_t)(bits%(std::uint64_t)tot);for(int a=0;a<n;++a){if(u<w[a])return a;u-=w[a];}return n-1;
}

void World::init(const WParams& pp,int founders){
  if(pp.L<5)throw std::invalid_argument("World: L must be at least 5");
  p=pp;t=0;next_id=1;const int N=p.L*p.L;res.assign(N,0);fert.assign(N,0);racc.assign(N,0);occ.assign(N,-1);ag.assign((std::size_t)p.K*p.S,Agent{});
  std::vector<double> f(N,0);
  for(int i=0;i<p.patches;++i){const std::uint64_t h=mix(p.seed*0x51ed27ULL+i);const double cx=(h&0xffff)/65536.0*p.L,cy=((h>>16)&0xffff)/65536.0*p.L;
    const double r=3+((h>>32)&0xff)/255.0*3;
    for(int y=0;y<p.L;++y)for(int x=0;x<p.L;++x){double dx=std::fabs(x-cx),dy=std::fabs(y-cy);dx=std::min(dx,p.L-dx);dy=std::min(dy,p.L-dy);
      f[y*p.L+x]+=255*std::exp(-(dx*dx+dy*dy)/(2*r*r));}}
  for(int c=0;c<N;++c){fert[c]=(std::uint8_t)std::min(255.0,std::floor(f[c]));res[c]=fert[c];}
  fp.clear();ph.clear();per=0;
  if(p.season>0){fp.assign((std::size_t)p.patches*N,0);  // the same bumps, one layer per patch
    for(int i=0;i<p.patches;++i){const std::uint64_t h=mix(p.seed*0x51ed27ULL+i);const double cx=(h&0xffff)/65536.0*p.L,cy=((h>>16)&0xffff)/65536.0*p.L;
      const double r=3+((h>>32)&0xff)/255.0*3;
      for(int y=0;y<p.L;++y)for(int x=0;x<p.L;++x){double dx=std::fabs(x-cx),dy=std::fabs(y-cy);dx=std::min(dx,p.L-dx);dy=std::min(dy,p.L-dy);
        fp[(std::size_t)i*N+y*p.L+x]=(std::uint8_t)std::min(255.0,std::floor(255*std::exp(-(dx*dx+dy*dy)/(2*r*r))));}}
    set_climate(0);for(int c=0;c<N;++c)res[c]=(std::uint8_t)cap(c);}
  mat.clear();tape.clear();mip.clear();mh0.clear();mh1.clear();mdir.clear();mst.clear();mhalt.clear();mlin.clear();mbt.clear();mfx.clear();
  nph.clear();ncyc.clear();
  if(p.matter){mat.assign(N,0);tape.assign((std::size_t)N*16,0);mip.assign(N,0);mh0.assign(N,0);mh1.assign(N,0);mdir.assign(N,0);mst.assign(N,0);
    mhalt.assign(N,0);mlin.assign(N,-1);mbt.assign(N,0);mfx.assign((std::size_t)N*4,0);
    if(p.nodes>0){nph.assign(N,0);ncyc.assign(N,-1);const int nn=std::max(1,(int)std::lround((double)p.nodes*N/1024));int placed=0;
      for(std::uint64_t i=0;placed<nn&&i<(std::uint64_t)64*N;++i){const std::uint64_t h=mix(mix(p.seed^0x40deULL)+i);const int c=(int)(h%N);if(mat[c])continue;
        mat[c]=3;res[c]=0;nph[c]=(int)((h>>32)%(std::uint64_t)std::max(1,p.node_per));++placed;}}}
  for(int k=0;k<p.K;++k)reseed_lineage(k,founders,0);
}

void World::set_climate(std::uint64_t salt){
  if(p.season<=0)return;const std::uint64_t h=mix(mix(p.seed^0xc11eULL)+salt);
  per=std::max(4,p.season/2+(int)(h%(std::uint64_t)std::max(1,p.season)));ph.assign(p.patches,0);
  for(int i=0;i<p.patches;++i)ph[i]=(int)(mix(h+i+1)%(std::uint64_t)per);
}
namespace {
// productivity of a patch at phase x of period q: triangle wave, 256 at mid-cycle, 0 for the first and last quarter
inline int prod(int x,int q){const int a=std::abs(2*x-q);return std::max(0,256*(q-2*a)/q);}
}  // namespace
int World::cap(int c) const{
  if(p.season<=0)return fert[c];const int N=p.L*p.L;int s=0;
  for(int i=0;i<p.patches;++i)s+=fp[(std::size_t)i*N+c]*prod((int)((t+ph[i])%per),per);
  return std::min(255,s>>8);
}

void World::reseed_lineage(int k,int founders,std::uint64_t salt){
  const int N=p.L*p.L;int placed=0;
  for(std::uint64_t i=0;placed<founders&&i<(std::uint64_t)64*N;++i){
    const std::uint64_t h=mix(mix(p.seed^0xf00dULL)+((std::uint64_t)k<<40)+(salt<<20)+i);const int c=(int)(h%N);if(occ[c]>=0||(p.matter&&mat[c]))continue;
    if(fert[c]<32&&i<(std::uint64_t)32*N)continue;  // prefer fertile cells
    int s=-1;for(int j=0;j<p.S;++j)if(!ag[(std::size_t)k*p.S+j].alive){s=j;break;}if(s<0)break;
    Agent& a=ag[(std::size_t)k*p.S+s];a=Agent{};a.x=c%p.L;a.y=c/p.L;a.dir=(int)((h>>32)&3);a.energy=p.e0;a.lineage=k;a.alive=true;a.id=next_id++;
    occ[c]=k*p.S+s;++placed;}
}

void World::observe(int slot,std::int8_t* o) const{
  std::fill(o,o+din(),0);const Agent& a=ag[slot];if(!a.alive)return;
  const int L=p.L,fx=FX[a.dir],fy=FY[a.dir],rx=-fy,ry=fx,lin=a.lineage;int i=0;  // right = forward rotated clockwise
  const bool hear=p.signals&&!((deaf>>lin)&1);
  auto wr=[L](int z){return z<0?z+L:(z>=L?z-L:z);};  // offsets are at most 4 and L >= 5: one wrap suffices
  for(int v=2;v>=-2;--v){const int bx=a.x+v*fx,by=a.y+v*fy;
    for(int u=-2;u<=2;++u,++i){const int c=wr(by+u*ry)*L+wr(bx+u*rx);
      if(std::max(std::abs(u),std::abs(v))<=p.view)o[i]=(std::int8_t)(res[c]>>1);const int q=occ[c];
      if(q>=0&&q!=slot){o[25+i]=(std::int8_t)(ag[q].lineage==lin?64:-64);if(hear&&(!p.kin_sig||ag[q].lineage==lin))o[53+i]=(std::int8_t)(32*ag[q].sig);}}}
  o[50]=(std::int8_t)std::min(127,a.energy/8);o[51]=(std::int8_t)std::min(127,a.age/4);o[52]=64;
  if(p.signals)o[78]=(std::int8_t)(32*a.sig);
  if(p.matter){i=0;
    for(int v=2;v>=-2;--v){const int bx=a.x+v*fx,by=a.y+v*fy;
      for(int u=-2;u<=2;++u,++i){const int c=wr(by+u*ry)*L+wr(bx+u*rx);o[79+i]=(std::int8_t)(mat[c]==1?48:mat[c]==2?-48:mat[c]==3?96:0);
        if(mat[c]==3)o[i]=(std::int8_t)(std::max(std::abs(u),std::abs(v))<=p.view&&ripe(c)?127:0);}}
    const int fc=wr(a.y+fy)*L+wr(a.x+fx);
    if(mat[fc]==1){const CodeIdx& ci=code_idx();for(int j=0;j<16;++j){const int x=ci.v[tape[(std::size_t)fc*16+j]];o[104+j]=(std::int8_t)(x<16?8*x+4:-64);}
      o[120]=(std::int8_t)std::min(127,mst[fc]/4);}
    o[121]=(std::int8_t)(8*a.wpos);}
}

void World::tick(const std::int8_t* const* logits,std::vector<int>& born){
  const int N=p.L*p.L;std::vector<int> order;parents.clear();
  for(int s=0;s<p.K*p.S;++s)if(ag[s].alive)order.push_back(s);
  for(int i=(int)order.size()-1;i>0;--i){const int j=(int)(mix(mix(p.seed^0x7e1cULL)+((std::uint64_t)t<<20)+i)%(std::uint64_t)(i+1));std::swap(order[i],order[j]);}
  std::vector<int> act(p.K*p.S,0),code;if(p.matter)code.assign(p.K*p.S,0);const int nv_=nv(),so=sig_off();
  for(int s:order){const int k=s/p.S;const std::int8_t* l=logits[k]+(std::size_t)(s%p.S)*nv_;
    act[s]=p.stoch?act_sample(l,mix(mix(p.seed^0xac7ULL)+((std::uint64_t)t<<24)+ag[s].id),p.nact):act_of(l,p.nact);  // same draws in mirrored forks
    if(p.signals)ag[s].sig=act_sample(l+so,mix(mix(p.seed^0x5197ULL)+((std::uint64_t)t<<24)+ag[s].id),4);  // heard next tick
    if(p.matter)code[s]=p.stoch?act_sample(l+16,mix(mix(p.seed^0xc0deULL)+((std::uint64_t)t<<24)+ag[s].id),16):act_of(l+16,16);}
  if(track&&stat.size()<(std::size_t)p.K*32)stat.assign((std::size_t)p.K*32,0);
  if(track)for(int s:order){const int k=ag[s].lineage;++stat[(std::size_t)k*32+act[s]];++stat[(std::size_t)k*32+22];if(ag[s].sig>0)++stat[(std::size_t)k*32+21];}
  std::vector<std::pair<int,int>> neat;  // W3.3: (node cell, eater slot) this tick
  for(int s:order){Agent& a=ag[s];if(!a.alive)continue;const int c=a.y*p.L+a.x;
    const int fc=((a.y+FY[a.dir]+p.L)%p.L)*p.L+(a.x+FX[a.dir]+p.L)%p.L;
    switch(act[s]){
      case 1:if(occ[fc]<0&&(!p.matter||!mat[fc])){occ[c]=-1;a.x=fc%p.L;a.y=fc/p.L;occ[fc]=s;}a.energy-=p.move;break;
      case 2:a.dir=(a.dir+3)&3;a.energy-=p.turn;break;
      case 3:a.dir=(a.dir+1)&3;a.energy-=p.turn;break;
      case 4:{if(p.nodes>0&&p.matter&&mat[fc]==3)neat.push_back({fc,s});
        int take=std::min<int>(res[c],p.bite);res[c]-=(std::uint8_t)take;
        if(p.own&&p.matter&&take<p.bite&&mat[fc]==1&&mlin[fc]==a.lineage){const int t2=std::min<int>(p.bite-take,mst[fc]);mst[fc]=(std::int16_t)(mst[fc]-t2);take+=t2;}  // granary
        a.energy=std::min(p.ecap,a.energy+take);break;}
      case 5:if(a.energy>=p.repro_min&&occ[fc]<0&&(!p.matter||!mat[fc])){const int k=a.lineage;int j=-1;
          for(int q=0;q<p.S;++q)if(!ag[(std::size_t)k*p.S+q].alive){j=q;break;}
          if(j>=0){const int cs=k*p.S+j;Agent& b=ag[cs];b=Agent{};b.x=fc%p.L;b.y=fc/p.L;b.dir=a.dir;b.energy=p.repro_give;b.lineage=k;b.alive=true;b.id=next_id++;
            occ[fc]=cs;a.energy-=p.repro_give+p.repro_cost;born.push_back(cs);parents.push_back(s);if(track)++stat[(std::size_t)k*32+17];}}
        break;
      case 6:{const int q=occ[fc];a.energy-=p.atk_cost;
        if(q>=0&&ag[q].alive){Agent& v=ag[q];const int take=std::min(v.energy,p.atk_take);v.energy-=take;a.energy=std::min(p.ecap,a.energy+take);
          if(v.energy<=0){v.alive=false;occ[fc]=-1;if(track){++stat[(std::size_t)a.lineage*32+16];++stat[(std::size_t)v.lineage*32+20];}}}
        break;}
      case 7:if(p.matter){if(mat[fc]==1){tape[(std::size_t)fc*16+a.wpos]=kCode[code[s]];a.wpos=(a.wpos+1)&15;}a.energy-=p.wr_cost;}break;  // write
      case 8:if(p.matter){if(mat[fc]==0&&occ[fc]<0){make_machine(fc,a.dir,a.lineage);a.wpos=0;a.energy-=p.mk_cost;}  // make or unmake
          else if(mat[fc]==1){if(!p.own||mlin[fc]<0||mlin[fc]==a.lineage){a.energy=std::min(p.ecap,a.energy+mst[fc]);kill_machine(fc);a.energy-=p.move;}
            else{kill_machine(fc);a.energy-=2*p.mk_cost;}}}  // W3.2: someone else's machine can only be destroyed
        break;
      case 9:if(p.matter&&mat[fc]==1){const int amt=std::min(p.feed_amt,std::max(0,a.energy-1));a.energy-=amt;mst[fc]=(std::int16_t)std::min(1000,mst[fc]+amt);}break;  // feed
      default:break;}
  }
  if(!neat.empty()){std::sort(neat.begin(),neat.end());  // joint harvests: at least two eaters on a ripe node in the same tick
    for(std::size_t i=0;i<neat.size();){std::size_t j=i;while(j<neat.size()&&neat[j].first==neat[i].first)++j;const int nc=neat[i].first;
      if(j-i>=2&&ripe(nc)){for(std::size_t k=i;k<j;++k){Agent& e=ag[neat[k].second];if(e.alive)e.energy=std::min(p.ecap,e.energy+p.node_pay);}
        ncyc[nc]=(std::int32_t)((t+nph[nc])/std::max(1,p.node_per));if(track)stat[(std::size_t)ag[neat[i].second].lineage*32+23]+=(long)(j-i);}
      i=j;}}
  for(int s=0;s<p.K*p.S;++s){Agent& a=ag[s];if(!a.alive)continue;a.energy-=p.base+(a.sig>0?p.sig_cost:0);++a.age;
    if(a.energy<=0||a.age>p.max_age){a.alive=false;occ[a.y*p.L+a.x]=-1;if(track)++stat[(std::size_t)a.lineage*32+(a.energy<=0?18:19)];}}
  if(p.matter)run_machines();
  if(p.matter&&p.wall_decay>0)for(int c=0;c<N;++c)if(mat[c]==2&&(int)(mix(mix(p.seed^0xe40dULL)+((std::uint64_t)t<<24)+c)&0xffff)<p.wall_decay)mat[c]=0;  // erosion
  if(p.matter&&p.spawn>0){const long r=(long)p.spawn*N/1024;const std::uint64_t h=mix(mix(p.seed^0x5badULL)+(std::uint64_t)t);  // spontaneous generation
    const int n=(int)(r/1000)+((long)(h%1000)<r%1000?1:0);if(n>0)seed_programs(n,(std::uint64_t)t*16+7,50,0.7);}
  if(p.season>0){int w[64];for(int i=0;i<p.patches&&i<64;++i)w[i]=prod((int)((t+ph[i])%per),per);  // seasonal capacity, rot above it
    for(int c=0;c<N;++c){if(p.matter&&mat[c]>=2)continue;int cp=0;for(int i=0;i<p.patches&&i<64;++i)cp+=fp[(std::size_t)i*N+c]*w[i];cp=std::min(255,cp>>8);
      if(res[c]>cp){res[c]=(std::uint8_t)std::max(cp,(int)res[c]-p.rot);continue;}
      if(p.regrow>0&&res[c]<cp){int a=racc[c]+cp*p.regrow;int r=res[c];while(a>=4096&&r<cp){++r;a-=4096;}racc[c]=(std::uint16_t)std::min(a,4095);res[c]=(std::uint8_t)r;}}}
  else if(p.regrow>0)for(int c=0;c<N;++c)if(res[c]<fert[c]&&!(p.matter&&mat[c]>=2)){int a=racc[c]+fert[c]*p.regrow;int r=res[c];while(a>=4096&&r<fert[c]){++r;a-=4096;}
      racc[c]=(std::uint16_t)std::min(a,4095);res[c]=(std::uint8_t)r;}  // rate fert*regrow/4096 per tick; regrow 0: none
  ++t;
}

bool World::ripe(int c) const{if(p.nodes<=0||mat[c]!=3)return false;const long per=std::max(1,p.node_per),cyc=(t+nph[c])/per;
  return (t+nph[c])%per<p.node_ripe&&ncyc[c]!=(std::int32_t)cyc;}
int World::front_of(int c,int dir) const{const int L=p.L,x=c%L,y=c/L;return ((y+FY[dir]+L)%L)*L+(x+FX[dir]+L)%L;}
void World::make_machine(int c,int dir,int lin){mat[c]=1;std::fill(tape.begin()+(std::size_t)c*16,tape.begin()+(std::size_t)c*16+16,0);
  mip[c]=mh0[c]=mh1[c]=0;mdir[c]=(std::uint8_t)dir;mst[c]=0;mhalt[c]=0;mlin[c]=(std::int8_t)lin;mbt[c]=t;std::fill(mfx.begin()+(std::size_t)c*4,mfx.begin()+(std::size_t)c*4+4,0);}
void World::kill_machine(int c){mat[c]=0;mst[c]=0;mhalt[c]=0;mlin[c]=-1;std::fill(tape.begin()+(std::size_t)c*16,tape.begin()+(std::size_t)c*16+16,0);}
// Machines run in a rotated cell order (deterministic per tick); a machine made this tick starts next tick.
void World::run_machines(){
  const int N=p.L*p.L;const int st=(int)(mix(mix(p.seed^0x3ac4ULL)+(std::uint64_t)t)%(std::uint64_t)N);
  for(int q=0;q<N;++q){const int c=(st+q)%N;if(mat[c]!=1||mbt[c]==t)continue;
    if(mst[c]<=0){if(++mhalt[c]>p.m_decay)kill_machine(c);continue;}
    mhalt[c]=0;--mst[c];std::uint8_t* tp=&tape[(std::size_t)c*16];int ip=mip[c],h0=mh0[c],h1=mh1[c];std::int32_t* fx=&mfx[(std::size_t)c*4];
    for(int n=0;n<p.m_steps;++n){
      switch(tp[ip]){
        case '<':h0=(h0+15)&15;break;case '>':h0=(h0+1)&15;break;case '{':h1=(h1+15)&15;break;case '}':h1=(h1+1)&15;break;
        case '+':++tp[h0];break;case '-':--tp[h0];break;case '.':tp[h1]=tp[h0];break;case ',':tp[h0]=tp[h1];break;
        case '[':if(tp[h0]==0){int d=1,j=ip;for(int k=0;k<16&&d>0;++k){j=(j+1)&15;if(tp[j]=='[')++d;else if(tp[j]==']')--d;}if(d==0)ip=j;}break;
        case ']':if(tp[h0]!=0){int d=1,j=ip;for(int k=0;k<16&&d>0;++k){j=(j+15)&15;if(tp[j]==']')++d;else if(tp[j]=='[')--d;}if(d==0)ip=j;}break;
        case 'E':{const int take=std::min<int>({(int)res[c],8,1000-mst[c]});res[c]=(std::uint8_t)(res[c]-take);mst[c]=(std::int16_t)(mst[c]+take);fx[0]+=take;break;}
        case 'D':{const int f=front_of(c,mdir[c]);if(mat[f]<2){const int r=std::min(255,(int)res[f]+std::min<int>(8,mst[c]));const int mv=r-res[f];
            res[f]=(std::uint8_t)r;mst[c]=(std::int16_t)(mst[c]-mv);fx[1]+=mv;}break;}
        case 'S':tp[h0]=res[front_of(c,mdir[c])];break;
        case 'C':{const int f=front_of(c,mdir[c]);if(mat[f]==0&&occ[f]<0&&mst[c]>=p.rep_cost){mst[c]=(std::int16_t)(mst[c]-p.rep_cost);
            make_machine(f,mdir[c],mlin[c]);std::copy(tp,tp+16,tape.begin()+(std::size_t)f*16);++fx[2];
            if(p.mut>0)for(int j=0;j<16;++j){const std::uint64_t h=mix(mix(p.seed^0xbad5ULL)+((std::uint64_t)t<<24)+((std::uint64_t)c<<5)+j);  // imperfect copy
              if((int)(h&0xffff)<p.mut)tape[(std::size_t)f*16+j]=kCode[(h>>16)&15];}}
          break;}
        case 'W':{const int f=front_of(c,mdir[c]);if(mst[c]>=p.wall_cost&&occ[f]<0&&mat[f]!=1){mst[c]=(std::int16_t)(mst[c]-p.wall_cost);
            if(mat[f]==0){mat[f]=2;res[f]=0;}else mat[f]=0;++fx[3];}break;}
        case 'R':mdir[c]=(std::uint8_t)((mdir[c]+tp[h1])&3);break;
        default:break;}
      ip=(ip+1)&15;}
    mip[c]=(std::uint8_t)ip;mh0[c]=(std::uint8_t)h0;mh1[c]=(std::uint8_t)h1;}
}
int World::seed_programs(int n,std::uint64_t salt,int store,double cont){
  if(!p.matter)return 0;const int N=p.L*p.L;int placed=0;const std::uint64_t ct=(std::uint64_t)std::llround(std::clamp(cont,0.0,1.0)*65536);
  for(std::uint64_t i=0;placed<n&&i<(std::uint64_t)64*N;++i){const std::uint64_t h=mix(mix(p.seed^0x9a11ULL)+(salt<<20)+i);const int c=(int)(h%N);
    if(occ[c]>=0||mat[c])continue;
    make_machine(c,(int)((h>>32)&3),-1);mbt[c]=-1;mst[c]=(std::int16_t)std::min(1000,store);
    std::uint64_t g=mix(h^0x5eedULL);int len=1;while(len<16&&(g&0xffff)<ct){++len;g=mix(g);}
    std::uint64_t b=mix(h^0xb17eULL);for(int j=0;j<len;++j){tape[(std::size_t)c*16+j]=kCode[(b>>(4*j))&15];}
    ++placed;}
  return placed;
}
int World::swap_programs(std::uint64_t salt,int store,double cont){
  if(!p.matter)return 0;int n=0;for(int c=0;c<p.L*p.L;++c)if(mat[c]==1&&mlin[c]<0){kill_machine(c);++n;}
  return seed_programs(n,salt,store,cont);
}
int World::catastrophe(std::uint64_t salt,double keep){
  const int L=p.L;const int side=std::max(1,std::min(L,(int)std::lround(L*std::sqrt(std::clamp(keep,0.0,1.0)))));
  const std::uint64_t h=mix(mix(p.seed^0xca7aULL)+salt);const int x0=(int)(h%L),y0=(int)((h>>20)%L);int n=0;
  for(std::size_t s=0;s<ag.size();++s){Agent& a=ag[s];if(!a.alive)continue;const int dx=((a.x-x0)%L+L)%L,dy=((a.y-y0)%L+L)%L;
    if(dx<side&&dy<side)continue;a.alive=false;occ[a.y*L+a.x]=-1;++n;}
  return n;
}
int World::wipe(int k){int n=0;if(!p.matter)return 0;for(int c=0;c<p.L*p.L;++c)if(mat[c]==1&&mlin[c]==k){kill_machine(c);++n;}return n;}
void World::census(int min_age,int& machines,int& walls,int& tapes,int& classes) const{
  machines=walls=tapes=classes=0;if(!p.matter)return;std::vector<std::uint64_t> th;bool cls[16]={false};
  for(int c=0;c<p.L*p.L;++c){if(mat[c]==2){++walls;continue;}if(mat[c]!=1)continue;++machines;if(t-mbt[c]<min_age)continue;
    std::uint64_t h=1469598103934665603ULL;for(int j=0;j<16;++j)h=(h^tape[(std::size_t)c*16+j])*1099511628211ULL;th.push_back(h);
    const std::int32_t* fx=&mfx[(std::size_t)c*4];cls[(fx[0]>0)|((fx[1]>0)<<1)|((fx[2]>0)<<2)|((fx[3]>0)<<3)]=true;}
  std::sort(th.begin(),th.end());tapes=(int)(std::unique(th.begin(),th.end())-th.begin());for(bool b:cls)classes+=b;
}

long World::biomass(int k) const{long b=0;for(int s=0;s<p.S;++s){const Agent& a=ag[(std::size_t)k*p.S+s];if(a.alive)b+=a.energy;}return b;}
int World::count(int k) const{int n=0;for(int s=0;s<p.S;++s)n+=ag[(std::size_t)k*p.S+s].alive;return n;}
long World::resources() const{long r=0;for(auto x:res)r+=x;return r;}
namespace {
void wr(std::FILE* f,std::int64_t x){std::fwrite(&x,8,1,f);}
bool rd(std::FILE* f,std::int64_t& x){return std::fread(&x,8,1,f)==1;}
constexpr std::int64_t kExt=0x5458453257LL;  // "W2EXT": marks the W2a extension of a world record
constexpr std::int64_t kExt3=0x5458453357LL;  // "W3EXT": marks the W3 matter block
constexpr std::int64_t kExt31=0x3133584533LL;  // W3 matter block with the W3.1 substrate settings
constexpr std::int64_t kExt4=0x3245564157LL;   // wave-2 flags (kin_sig, lamarck)
template<class T> void wv(std::FILE* f,const std::vector<T>& v){wr(f,(std::int64_t)v.size());if(!v.empty())std::fwrite(v.data(),sizeof(T),v.size(),f);}
template<class T> bool rv(std::FILE* f,std::vector<T>& v){std::int64_t n=0;if(!rd(f,n)||n<0||n>(1<<26))return false;v.resize((std::size_t)n);
  return n==0||std::fread(v.data(),sizeof(T),(std::size_t)n,f)==(std::size_t)n;}
}  // namespace

void World::save(std::FILE* f) const{
  const std::int64_t q[]={p.L,p.S,p.K,p.patches,p.e0,p.ecap,p.base,p.move,p.turn,p.bite,p.repro_min,p.repro_give,p.repro_cost,p.max_age,p.regrow,
    p.stoch,p.nact,p.atk_cost,p.atk_take,(std::int64_t)p.seed,t,(std::int64_t)next_id};
  for(auto x:q){wr(f,x);}
  if(ext()){wr(f,kExt);const std::int64_t e[]={p.season,p.view,p.signals,p.rot,p.sig_cost,per};for(auto x:e){wr(f,x);}wv(f,ph);wv(f,fp);}
  if(p.matter){wr(f,(p.spawn||p.mut||p.own||p.nodes)?kExt31:kExt3);const std::int64_t e[]={p.matter,p.m_steps,p.m_decay,p.mk_cost,p.wr_cost,p.feed_amt,p.rep_cost,p.wall_cost};for(auto x:e){wr(f,x);}
    if(p.spawn||p.mut||p.own||p.nodes){wr(f,p.spawn);wr(f,p.mut);wr(f,p.own);wr(f,p.nodes);wr(f,p.node_pay);wr(f,p.node_per);wr(f,p.node_ripe);wv(f,nph);wv(f,ncyc);}
    wv(f,mat);wv(f,tape);wv(f,mip);wv(f,mh0);wv(f,mh1);wv(f,mdir);wv(f,mst);wv(f,mhalt);wv(f,mlin);wv(f,mbt);wv(f,mfx);}
  if(p.kin_sig||p.lamarck||p.wall_decay){wr(f,kExt4);wr(f,p.kin_sig);wr(f,p.lamarck);wr(f,p.wall_decay);}
  wv(f,res);wv(f,fert);wv(f,racc);wv(f,occ);wr(f,(std::int64_t)ag.size());
  for(auto& a:ag){const std::int64_t r[]={a.x,a.y,a.dir,a.energy,a.age,a.lineage,a.alive,(std::int64_t)a.id};for(auto x:r)wr(f,x);}
  if(ext())for(auto& a:ag){wr(f,a.sig);}
  if(p.matter)for(auto& a:ag){wr(f,a.wpos);}
}
bool World::load(std::FILE* f){
  std::int64_t q[22];for(auto& x:q)if(!rd(f,x))return false;
  int* ip[]={&p.L,&p.S,&p.K,&p.patches,&p.e0,&p.ecap,&p.base,&p.move,&p.turn,&p.bite,&p.repro_min,&p.repro_give,&p.repro_cost,&p.max_age,&p.regrow,
    &p.stoch,&p.nact,&p.atk_cost,&p.atk_take};
  for(int i=0;i<19;++i){*ip[i]=(int)q[i];}p.seed=(std::uint64_t)q[19];t=q[20];next_id=(std::uint64_t)q[21];
  p.season=0;p.view=2;p.signals=0;p.rot=1;p.sig_cost=0;per=0;ph.clear();fp.clear();
  {std::int64_t m=0;const long pos=std::ftell(f);if(!rd(f,m))return false;
    if(m==kExt){std::int64_t e[6];for(auto& x:e)if(!rd(f,x))return false;p.season=(int)e[0];p.view=(int)e[1];p.signals=(int)e[2];p.rot=(int)e[3];p.sig_cost=(int)e[4];per=(int)e[5];
      if(!rv(f,ph)||!rv(f,fp))return false;}
    else if(std::fseek(f,pos,SEEK_SET)!=0)return false;}  // W1 record: no extension
  p.matter=0;mat.clear();tape.clear();mip.clear();mh0.clear();mh1.clear();mdir.clear();mst.clear();mhalt.clear();mlin.clear();mbt.clear();mfx.clear();
  {std::int64_t m=0;const long pos=std::ftell(f);if(!rd(f,m))return false;
    p.spawn=0;p.mut=0;p.own=0;p.nodes=0;nph.clear();ncyc.clear();
    if(m==kExt3||m==kExt31){std::int64_t e[8];for(auto& x:e)if(!rd(f,x))return false;
      p.matter=(int)e[0];p.m_steps=(int)e[1];p.m_decay=(int)e[2];p.mk_cost=(int)e[3];p.wr_cost=(int)e[4];p.feed_amt=(int)e[5];p.rep_cost=(int)e[6];p.wall_cost=(int)e[7];
      if(m==kExt31){std::int64_t a=0,b=0,c=0,d=0,e2=0,g=0,h=0;if(!rd(f,a)||!rd(f,b)||!rd(f,c)||!rd(f,d)||!rd(f,e2)||!rd(f,g)||!rd(f,h))return false;
        p.spawn=(int)a;p.mut=(int)b;p.own=(int)c;p.nodes=(int)d;p.node_pay=(int)e2;p.node_per=(int)g;p.node_ripe=(int)h;if(!rv(f,nph)||!rv(f,ncyc))return false;}
      if(!rv(f,mat)||!rv(f,tape)||!rv(f,mip)||!rv(f,mh0)||!rv(f,mh1)||!rv(f,mdir)||!rv(f,mst)||!rv(f,mhalt)||!rv(f,mlin)||!rv(f,mbt)||!rv(f,mfx))return false;}
    else if(std::fseek(f,pos,SEEK_SET)!=0)return false;}  // no W3 block
  p.kin_sig=0;p.lamarck=0;p.wall_decay=0;
  {std::int64_t m=0;const long pos=std::ftell(f);if(!rd(f,m))return false;
    if(m==kExt4){std::int64_t a=0,b=0,c=0;if(!rd(f,a)||!rd(f,b)||!rd(f,c))return false;p.kin_sig=(int)a;p.lamarck=(int)b;p.wall_decay=(int)c;}
    else if(std::fseek(f,pos,SEEK_SET)!=0)return false;}  // no wave-2 block
  if(!rv(f,res)||!rv(f,fert)||!rv(f,racc)||!rv(f,occ)){return false;}std::int64_t n=0;if(!rd(f,n)||n<0||n>(1<<24)){return false;}ag.assign((std::size_t)n,Agent{});
  for(auto& a:ag){std::int64_t r[8];for(auto& x:r)if(!rd(f,x))return false;
    a.x=(int)r[0];a.y=(int)r[1];a.dir=(int)r[2];a.energy=(int)r[3];a.age=(int)r[4];a.lineage=(int)r[5];a.alive=r[6]!=0;a.id=(std::uint64_t)r[7];a.sig=0;}
  if(ext())for(auto& a:ag){std::int64_t x=0;if(!rd(f,x))return false;a.sig=(int)x;}
  for(auto& a:ag)a.wpos=0;
  if(p.matter)for(auto& a:ag){std::int64_t x=0;if(!rd(f,x))return false;a.wpos=(int)x;}
  return (int)res.size()==p.L*p.L&&res.size()==fert.size()&&res.size()==racc.size()&&res.size()==occ.size()&&(int)ag.size()==p.K*p.S;
}

void World::move_slot(int from,int to){Agent& a=ag[from];if(!a.alive)return;ag[to]=a;ag[to].lineage=to/p.S;occ[a.y*p.L+a.x]=to;a=Agent{};}

}  // namespace oee::w
