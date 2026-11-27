#include "../plugins/loop_remix_lab/RemixDSP.h"
#include <cmath>
#include <cstdio>
#include <vector>
using namespace rl::rx;
static int fails=0; static void ok(bool c,const char*m){std::printf("%s %s\n",c?"OK  ":"FAIL",m); if(!c)++fails;}
static void run(Engine& e,std::vector<float>& L,std::vector<float>& R,int bs,double bpm){ double q=0; for(size_t o=0;o<L.size();o+=bs){ int n=(int)std::min<size_t>(bs,L.size()-o); float* io[2]={L.data()+o,R.data()+o}; e.setTransport(bpm,q,true); e.process(io,2,n); q+=n*bpm/60.0/48000; } }
int main(int argc,char**argv){
  const double sr=48000; const int N=48000*4;
  auto sig=[&](std::vector<float>& L,std::vector<float>& R){ L.resize(N); R.resize(N); for(int i=0;i<N;++i) L[i]=R[i]=0.5f*(float)std::sin(i*0.031)+0.2f*(float)std::sin(i*0.0071); };
  { Engine e; e.prepare(sr); Params p; e.setParams(p); e.reset(); std::vector<float> L,R,L0,R0; sig(L,R); L0=L; run(e,L,R,256,120); double d=0; for(int i=0;i<N;++i) d=std::max(d,(double)std::fabs(L[i]-L0[i])); ok(d<1e-5,"pas neutres = signal intact (0 dB)"); }
  bool fin=true; float pk=0;
  for(int fx=0;fx<NumFx;++fx){ Engine e; e.prepare(sr); Params p; p.steps.fill(fx); p.lfo[0]={fx%5,fx%7,1.f,fx%6}; p.lfo[1]={(fx+2)%5,(fx+3)%7,1.f,(fx+1)%6}; p.reso=0.97f; p.cutoff=fx%2?30.f:20000.f; e.setParams(p); e.reset();
    std::vector<float> L,R; sig(L,R); run(e,L,R,333,fx%2?300:30); for(float v:L){ if(!std::isfinite(v)) fin=false; pk=std::max(pk,std::fabs(v)); } }
  ok(fin,"16 effets x LFO extrêmes : sortie finie"); ok(pk<=2.f,"crête bornée");
  { Params p; for(int i=0;i<16;++i) p.steps[i]=i; p.lfo[0]={0,3,.6f,0}; Engine a,b; a.prepare(sr); b.prepare(sr); a.setParams(p); b.setParams(p); a.reset(); b.reset();
    std::vector<float> L,R,L2,R2; sig(L,R); L2=L; R2=R; run(a,L,R,4096,128); run(b,L2,R2,64,128); double d=0; int fi=-1; for(int i=0;i<N;++i){ double x=std::fabs(L[i]-L2[i]); if(x>1e-3&&fi<0) fi=i; d=std::max(d,x);} std::printf("fi=%d ",fi); std::printf("d=%g\n",d); ok(d<1e-3,"rendu indépendant des blocs"); }
  { Params p; p.steps.fill(0); p.steps[3]=Mute; Engine e; e.prepare(sr); e.setParams(p); e.reset(); std::vector<float> L,R; sig(L,R); run(e,L,R,512,120);
    double mx=0; for(int i=0;i<N-1;++i) mx=std::max(mx,(double)std::fabs(L[i+1]-L[i])); ok(mx<0.06,"pas de clic aux changements de pas"); }
  if(argc>1){ if(FILE*f=std::fopen(argv[1],"a")){ std::fprintf(f,"\n## Loop Remix Lab\n%s\n",fails?"ÉCHEC":"Tous les tests passent"); std::fclose(f);} }
  return fails?1:0; }
