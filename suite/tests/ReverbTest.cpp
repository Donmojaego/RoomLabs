#include "../plugins/reverb/ReverbDSP.h"
#include <cmath>
#include <cstdio>
#include <vector>
using namespace rl::rv;
static int fails=0; static void ok(bool c,const char*m){std::printf("%s %s\n",c?"OK  ":"FAIL",m); if(!c)++fails;}
int main(int argc,char**argv){
  const double sr=48000; Engine e; e.prepare(sr); Params p; p.mix=1; e.setParams(p); e.reset();
  std::vector<float> L(512),R(512); float* io[2]={L.data(),R.data()};
  // impulsion -> queue présente et finie
  double tail=0; bool fin=true;
  for(int b=0;b<200;++b){ std::fill(L.begin(),L.end(),0.f); std::fill(R.begin(),R.end(),0.f); if(b==0){L[0]=R[0]=1;} e.process(io,2,512);
    for(int i=0;i<512;++i){ if(!std::isfinite(L[i])) fin=false; if(b>20&&b<40) tail+=std::fabs(L[i]); } }
  ok(fin,"sortie finie"); ok(tail>1e-4,"queue de réverbération présente");
  // bypass : la queue continue
  p.bypass=true; e.setParams(p); double after=0;
  std::fill(L.begin(),L.end(),0.f); std::fill(R.begin(),R.end(),0.f); L[0]=R[0]=1; p.bypass=false; e.setParams(p); e.process(io,2,512);
  p.bypass=true; e.setParams(p);
  for(int b=0;b<10;++b){ std::fill(L.begin(),L.end(),0.f); std::fill(R.begin(),R.end(),0.f); e.process(io,2,512); for(float v:L) after+=std::fabs(v);} 
  ok(after>1e-4,"bypass : la queue continue");
  // changements de type sans explosion, valeurs extrêmes
  float peak=0; for(int t=0;t<30;++t){ p.type=t%3; p.size=(t%5)/4.f; p.decay=t%2?30.f:0.1f; p.predelayMs=t%2?250.f:0; p.bypass=false; p.dampHz=t%2?20000.f:1000.f; e.setParams(p);
    for(int i=0;i<512;++i){L[i]=R[i]=(float)std::sin(i*0.05)*0.9f;} e.process(io,2,512); for(float v:L) peak=std::max(peak,std::fabs(v)); }
  ok(std::isfinite(peak)&&peak<4.f,"types/valeurs extrêmes stables");
  // indépendance de la taille de bloc
  Engine a,b; a.prepare(sr); b.prepare(sr); Params q; a.setParams(q); b.setParams(q); a.reset(); b.reset();
  std::vector<float> x(4096),y(4096),x2(4096),y2(4096); for(int i=0;i<4096;++i) x[i]=y[i]=x2[i]=y2[i]=(float)std::sin(i*0.01);
  float* ia[2]={x.data(),y.data()}; a.process(ia,2,4096);
  for(int o=0;o<4096;o+=64){ float* ib[2]={x2.data()+o,y2.data()+o}; b.process(ib,2,64);} 
  double d=0; for(int i=0;i<4096;++i) d=std::max(d,(double)std::fabs(x[i]-x2[i])); ok(d<1e-4,"rendu indépendant des blocs");
  if(argc>1){ if(FILE*f=std::fopen(argv[1],"a")){ std::fprintf(f,"\n## Reverb\n%s\n",fails?"ÉCHEC":"Tous les tests passent"); std::fclose(f);} }
  return fails?1:0; }
