#include "../plugins/audio_to_midi/A2MDSP.h"
#include <cmath>
#include <cstdio>
#include <vector>
using namespace rl::a2m;
static int fails=0; static void ok(bool c,const char*m){std::printf("%s %s\n",c?"OK  ":"FAIL",m); if(!c)++fails;}
static std::vector<Event> run(Engine& e,std::vector<float>& x,int bs){ std::vector<Event> all; Event ev[Engine::kMaxEvents];
  for(size_t o=0;o<x.size();o+=bs){ int n=(int)std::min<size_t>(bs,x.size()-o); const float* in[1]={x.data()+o}; int k=e.process(in,1,n,ev); for(int j=0;j<k;++j){ Event q=ev[j]; q.offset+=(int)o; all.push_back(q);} } return all; }
int main(int argc,char**argv){
  const double sr=48000;
  { std::vector<float> x; const double f[3]={220,329.63,440}; for(int k=0;k<3;++k){ for(int i=0;i<24000;++i){ double env=std::min(1.0,i/200.0)*std::min(1.0,(24000-i)/200.0); x.push_back((float)(0.4*env*std::sin(2*M_PI*f[k]*i/sr))); } for(int i=0;i<4800;++i) x.push_back(0); }
    Engine e; e.prepare(sr); e.setParams(Params{}); e.reset(); auto ev=run(e,x,512); std::vector<int> on; int offs=0; for(auto& q:ev){ if(q.vel>0) on.push_back(q.note); else ++offs; }
    std::printf("   notes:"); for(int v:on) std::printf(" %d",v); std::printf("\n");
    ok(on.size()==3 && on[0]==57 && on[1]==64 && on[2]==69,"mélodie La2 / Mi3 / La3 reconnue"); ok((int)on.size()==offs,"chaque note se termine"); }
  { std::vector<float> x(96000); uint32_t s=1; for(auto& v:x){ s=s*1664525u+1013904223u; v=(float)((s>>8)/8388608.0-1)*0.001f; }
    Engine e; e.prepare(sr); e.setParams(Params{}); e.reset(); ok(run(e,x,256).empty(),"bruit faible : aucune note"); }
  { std::vector<float> x(96000,0.f); uint32_t s=7; for(int h=0;h<8;++h){ int st=h*12000; for(int i=0;i<3000;++i){ s=s*1664525u+1013904223u; double env=std::exp(-i/400.0);
      x[st+i]+=(float)(h%2? 0.5*env*((s>>8)/8388608.0-1) : 0.8*env*std::sin(2*M_PI*55*i/sr)); } }
    Params p; p.mode=1; Engine e; e.prepare(sr); e.setParams(p); e.reset(); auto ev=run(e,x,300); int on=0,k=0,sn=0; for(auto& q:ev) if(q.vel>0){++on; if(q.note==36)++k; else ++sn;}
    std::printf("   coups %d (grosse caisse %d, autres %d)\n",on,k,sn); ok(on==8,"batterie : 8 coups détectés"); ok(k>=3&&sn>=3,"grosse caisse / caisse claire séparées"); }
  { std::vector<float> x(48000); for(int i=0;i<48000;++i) x[i]=(float)(i%2?1e30:-1e30); x[100]=NAN; Engine e; e.prepare(96000); Params p; p.threshDb=-90; e.setParams(p); e.reset(); run(e,x,64); p.mode=1; e.setParams(p); run(e,x,64); ok(true,"valeurs extrêmes sans plantage"); }
  if(argc>1){ if(FILE*f=std::fopen(argv[1],"a")){ std::fprintf(f,"\n## Audio to MIDI\n%s\n",fails?"ÉCHEC":"Tous les tests passent"); std::fclose(f);} }
  return fails?1:0; }
