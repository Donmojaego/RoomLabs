#include "../plugins/rack/RackDSP.h"
#include <cmath>
#include <cstdio>
#include <vector>
using namespace rl::rack;
static int fails=0; static void ok(bool c,const char*m){std::printf("%s %s\n",c?"OK  ":"FAIL",m); if(!c)++fails;}
static void run(Engine& e,std::vector<float>& L,std::vector<float>& R,int bs){ double q=0; for(size_t o=0;o<L.size();o+=bs){ int n=(int)std::min<size_t>(bs,L.size()-o); float* io[2]={L.data()+o,R.data()+o}; e.setTransport(120,q,true); e.process(io,2,n); q+=n*2.0/48000; } }
int main(int argc,char**argv){
  const int N=96000; auto sig=[&](std::vector<float>& L,std::vector<float>& R){ L.resize(N); R.resize(N); for(int i=0;i<N;++i) L[i]=R[i]=0.5f*(float)std::sin(i*0.02); };
  { Engine e; e.prepare(48000,1024); Params p; e.setParams(p); e.reset(); std::vector<float> L,R,L0; sig(L,R); L0=L; run(e,L,R,512); double d=0; for(int i=0;i<N;++i) d=std::max(d,(double)std::fabs(L[i]-L0[i])); ok(d<1e-6,"rack vide = signal intact (0 dB)"); }
  { Engine e; e.prepare(48000,1024); Params p; p.slot[0].module=Reverb; p.slot[1].module=Remix; p.slot[1].b=.7f; p.slot[2].module=Noise; p.slot[2].a=.3f; e.setParams(p); e.reset();
    std::vector<float> L,R; sig(L,R); bool fin=true; float pk=0;
    for(int pass=0;pass<6;++pass){ p.slot[pass%3].module=(pass*7)%4; p.slot[(pass+1)%3].on=pass%2; e.setParams(p); std::vector<float> a=L,b=R; run(e,a,b,333); for(float v:a){ if(!std::isfinite(v)) fin=false; pk=std::max(pk,std::fabs(v)); } }
    ok(fin,"changements de modules : sortie finie"); ok(pk<=2.f,"crête bornée"); }
  { Engine e; e.prepare(48000,1024); Params p; p.slot[0].module=Reverb; p.slot[0].b=.6f; e.setParams(p); e.reset(); std::vector<float> L(48000,0.f),R(48000,0.f); L[10]=R[10]=1; run(e,L,R,256);
    p.slot[0].on=false; e.setParams(p); std::vector<float> a(24000,0.f),b(24000,0.f); run(e,a,b,256); double t=0; for(float v:a) t+=std::fabs(v); ok(t>1e-3,"Reverb en OFF : la queue continue"); }
  { std::array<int,16> s1,s2; remixPattern(3,.6f,s1); remixPattern(3,.6f,s2); ok(s1==s2,"motif Remix reproductible"); }
  if(argc>1){ if(FILE*f=std::fopen(argv[1],"a")){ std::fprintf(f,"\n## 7 Room Rack\n%s\n",fails?"ÉCHEC":"Tous les tests passent"); std::fclose(f);} }
  return fails?1:0; }
