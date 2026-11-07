#include "pitchshift_abi.h"
#include "diagnostics.h"
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <vector>
static int state(IReaperPitchShift *p) { return p->Extended(0x46545501,nullptr,nullptr,nullptr); }
static void configure(IReaperPitchShift *p) {
  p->set_srate(48000); p->set_nch(2); p->set_tempo(1); p->set_shift(1); p->set_formant_shift(1); p->SetQualityParameter(0); p->Reset();
}
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c); std::exit(1); } } while (0)
int main(int argc, char **argv) {
  CHECK(argc == 2);
  void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (!lib) { std::fprintf(stderr,"%s\n",dlerror()); return 2; }
  auto factory = reinterpret_cast<IReaperPitchShift *(*)(int)>(dlsym(lib,"ReaperGetPitchShiftAPI"));
  CHECK(factory);
  CHECK(factory(0x13) == nullptr);
  auto object = factory(REAPER_PITCHSHIFT_API_VER);
  CHECK(object != nullptr);
  delete object;
  for (int nch : {1, 2}) {
    object = factory(REAPER_PITCHSHIFT_API_VER);
    object->set_srate(48000); object->set_nch(nch);
    object->set_tempo(1); object->set_shift(1); object->set_formant_shift(1);
    object->SetQualityParameter(0);
    CHECK(object->IsReset());
    double expected[34];
    for (int i=0;i<17*nch;i++) expected[i] = (i%2 ? -1.25 : 0.5) * (i+1);
    expected[0] = -0.0;
    auto input = object->GetBuffer(17);
    CHECK(input != nullptr);
    std::memcpy(input,expected,17*nch*sizeof(double));
    object->BufferDone(17);
    CHECK(!object->IsReset());
    double out[40]; for (auto &v:out) v=123456;
    CHECK(object->GetSamples(5,out+1) == 5);
    CHECK(out[0] == 123456 && out[1+5*nch] == 123456);
    CHECK(std::memcmp(out+1,expected,5*nch*sizeof(double)) == 0);
    CHECK(object->GetSamples(30,out+1) == 12);
    CHECK(std::memcmp(out+1,expected+5*nch,12*nch*sizeof(double)) == 0);
    CHECK(out[1+12*nch] == 123456);
    CHECK(object->GetSamples(1,out) == 0);
    CHECK(object->IsReset());
    object->FlushSamples(); object->FlushSamples();
    CHECK(object->GetSamples(1,out) == 0);
    delete object;
  }
  std::puts("PASS FTU-02 mono/stereo interleaved bit integrity/partial frame counts/flush");
  // Nonunity must be observably unsupported, never a successful passthrough.
  object = factory(REAPER_PITCHSHIFT_API_VER);
  object->set_srate(44100); object->set_nch(2); object->set_tempo(2);
  CHECK(object->Extended(0x46545501,nullptr,nullptr,nullptr) & 2);
  auto in = object->GetBuffer(8); CHECK(in);
  for (int i=0;i<16;i++) in[i]=0.75;
  object->BufferDone(8); object->FlushSamples();
  double silent[20]; for (auto &v:silent) v=99;
  CHECK(object->GetSamples(8,silent+1) == 8);
  for (int i=1;i<=16;i++) CHECK(silent[i] == 0);
  CHECK(silent[0]==99 && silent[17]==99);
  object->Reset();
  CHECK(object->Extended(0x46545501,nullptr,nullptr,nullptr) & 2);
  object->set_tempo(1); object->set_shift(1); object->set_formant_shift(-1);
  CHECK(object->Extended(0x46545501,nullptr,nullptr,nullptr) == 0);
  CHECK(object->Extended(0x100,nullptr,nullptr,nullptr) == 2);
  CHECK(object->Extended(0x201,nullptr,nullptr,nullptr) == 0);
  CHECK(object->Extended(1234,nullptr,nullptr,nullptr) == 0);
  delete object;
  std::puts("PASS FTU-03 nonunity silence/status/reset retains configuration");
  object = factory(REAPER_PITCHSHIFT_API_VER);
  const double nan = std::numeric_limits<double>::quiet_NaN(), inf = std::numeric_limits<double>::infinity();
  for (double rate : {0.0,-1.0,nan,inf,7999.0,384001.0}) {
    configure(object); object->set_srate(rate);
    CHECK(state(object) & 1); CHECK(!object->GetBuffer(1));
    CHECK(object->GetSamples(1,silent) == 0);
  }
  for (int channels : {0,-1,3,2147483647}) {
    configure(object); object->set_nch(channels);
    CHECK(state(object) & 1); CHECK(!object->GetBuffer(1));
  }
  for (double rate : {8000.0,44100.0,48000.0,96000.0,384000.0}) {
    configure(object); object->set_srate(rate); CHECK(state(object)==0);
  }
  for (int setter=0;setter<3;setter++) for (double ratio : {0.0,-2.0,nan,inf,0.5,2.0}) {
    configure(object);
    if (setter==0) object->set_tempo(ratio);
    if (setter==1) object->set_shift(ratio);
    if (setter==2) object->set_formant_shift(ratio);
    CHECK(state(object) & 2);
    in = object->GetBuffer(1); CHECK(in); in[0]=in[1]=7;
    object->BufferDone(1); CHECK(object->GetSamples(1,silent)==1);
    CHECK(silent[0]==0 && silent[1]==0);
  }
  configure(object); object->SetQualityParameter(99); CHECK(state(object) & 8);
  in=object->GetBuffer(1); CHECK(in); in[0]=in[1]=7;
  object->BufferDone(1); CHECK(object->GetSamples(1,silent)==1 && silent[0]==0 && silent[1]==0);
  for (int quality : {-1,0,12<<16}) { object->SetQualityParameter(quality); CHECK(state(object)==0); }
  delete object;
  CHECK(!factory(-1) && !factory(0) && !factory(0x15));
  std::puts("PASS FTU-04 invalid format/ratios/quality/version, supported sample-rate boundaries");
  object=factory(0x14); configure(object);
  object->BufferDone(1); CHECK(state(object)&4); object->Reset(); CHECK(state(object)==0);
  CHECK(!object->GetBuffer(-1)); CHECK(state(object)&4); object->Reset();
  CHECK(!object->GetBuffer(65537)); CHECK(state(object)&4); object->Reset();
  CHECK(!object->GetBuffer(0)); CHECK(state(object)==0);
  CHECK(object->GetBuffer(4)); object->BufferDone(5); CHECK(state(object)&4); object->Reset();
  CHECK(object->GetBuffer(4)); CHECK(!object->GetBuffer(4)); CHECK(state(object)&4); object->Reset();
  CHECK(object->GetBuffer(4)); object->BufferDone(-1); CHECK(state(object)&4); object->Reset();
  CHECK(object->GetSamples(-1,silent)==0); CHECK(state(object)&4); object->Reset();
  CHECK(object->GetSamples(1,nullptr)==0); CHECK(state(object)&4); object->Reset();
  CHECK(object->GetSamples(0,nullptr)==0 && state(object)==0);
  // Partial commits, repeated same format setters, Flush without a hidden commit.
  in=object->GetBuffer(8); CHECK(in); for(int i=0;i<16;i++) in[i]=i+0.25;
  object->set_srate(48000); object->set_nch(2); object->set_tempo(1);
  object->BufferDone(3); object->FlushSamples(); object->FlushSamples();
  CHECK(object->GetSamples(8,silent)==3); for(int i=0;i<6;i++) CHECK(silent[i]==i+0.25);
  CHECK(object->GetBuffer(4)); object->BufferDone(0); CHECK(object->IsReset());
  in=object->GetBuffer(4); CHECK(in); in[0]=in[1]=42; object->BufferDone(1);
  CHECK(object->GetBuffer(3)); object->Reset();
  CHECK(object->IsReset() && object->GetSamples(1,silent)==0);
  object->FlushSamples(); CHECK(object->GetSamples(1,silent)==0);
  object->BufferDone(1); CHECK(state(object)&4); object->Reset();
  // Full ring, capacity refusal, wrap, drain: exact frame counts and channel order.
  in=object->GetBuffer(65536); CHECK(in);
  for(int i=0;i<131072;i++) in[i]=i;
  object->BufferDone(65536);
  CHECK(!object->GetBuffer(1)); CHECK(state(object)&4);
  object->Reset(); CHECK(state(object)==0);
  in=object->GetBuffer(65536); CHECK(in); for(int i=0;i<131072;i++) in[i]=i;
  object->BufferDone(65536);
  std::vector<double> large(131074, -123);
  CHECK(object->GetSamples(60000,large.data()+1)==60000);
  for(int i=0;i<120000;i++) CHECK(large[i+1]==i);
  in=object->GetBuffer(60000); CHECK(in); for(int i=0;i<120000;i++) in[i]=-i-1;
  object->BufferDone(60000);
  CHECK(object->GetSamples(65536,large.data()+1)==65536);
  for(int i=0;i<11072;i++) CHECK(large[i+1]==120000+i);
  for(int i=0;i<120000;i++) CHECK(large[i+11073]==-i-1);
  CHECK(large.front()==-123 && large.back()==-123 && object->IsReset());
  // Format/ratio changes discard pending old-domain frames, not reinterpret them.
  CHECK(object->GetBuffer(1)); object->BufferDone(1); object->set_nch(1);
  CHECK(object->GetSamples(1,silent)==0);
  CHECK(object->GetBuffer(1)); object->BufferDone(1); object->set_shift(2);
  CHECK(object->GetSamples(1,silent)==0);
  delete object;
  std::puts("PASS FTU-05 protocol faults/reset/partial commit/reconfigure/bounded queue/wrap");
  auto readlog=reinterpret_cast<FtuReadDiagnostics>(dlsym(lib,"FlexUnityReadDiagnostics")); CHECK(readlog);
  FtuEvent log[FTU_LOG_CAPACITY]; uint64_t dropped=0,live=99;
  int records=readlog(log,FTU_LOG_CAPACITY,&dropped,&live);
  CHECK(records>0 && records<FTU_LOG_CAPACITY && dropped==0 && live==0);
  bool methods[18]={}; uint64_t creates=0,destroys=0;
  for(int i=0;i<records;i++) {
    CHECK(log[i].sequence==static_cast<uint64_t>(i));
    CHECK(log[i].method>=1 && log[i].method<=17); methods[log[i].method]=true;
    creates+=log[i].method==FTU_CREATE; destroys+=log[i].method==FTU_DESTROY;
  }
  for(int m=1;m<=17;m++) if(m!=FTU_ENUM) CHECK(methods[m]);
  CHECK(creates==destroys);
  auto enumerate=reinterpret_cast<const char *(*)(int)>(dlsym(lib,"EnumQualityModes")); CHECK(enumerate);
  CHECK(std::strstr(enumerate(0),"unity-only") && !enumerate(-1) && !enumerate(1));
  object=factory(0x14); CHECK(object);
  for(int i=0;i<FTU_LOG_CAPACITY*2;i++) object->IsReset();
  delete object;
  records=readlog(log,FTU_LOG_CAPACITY,&dropped,&live);
  CHECK(records==FTU_LOG_CAPACITY && dropped>0 && live==0);
  FtuEvent first=log[0]; auto old_dropped=dropped;
  CHECK(readlog(log,1,&dropped,&live)==1 && std::memcmp(&first,log,sizeof(first))==0 && dropped==old_dropped);
  CHECK(readlog(nullptr,0,&dropped,&live)==0);
  std::printf("PASS FTU-06 bounded diagnostics: retained=%d dropped=%llu live=%llu\n",records,(unsigned long long)dropped,(unsigned long long)live);
  CHECK(dlclose(lib) == 0);
  std::puts("PASS FTU-01 factory/version/virtual-delete/dlclose");
}
