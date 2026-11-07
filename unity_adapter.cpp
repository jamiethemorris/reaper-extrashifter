// Unity-only interface experiment; no third-party DSP, devices, or file I/O.
#include "pitchshift_abi.h"
#include "diagnostics.h"
#include <new>
#include <array>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <cmath>
#define EXPORT extern "C" __attribute__((visibility("default")))
namespace {
std::array<FtuEvent, FTU_LOG_CAPACITY> events{};
std::atomic<uint64_t> calls{0}, next_id{1}, live{0};
static_assert(std::atomic<uint64_t>::is_always_lock_free, "diagnostics require lock-free atomics");
void record(uint64_t id, FtuMethod method, double arg, int result, int status=0,
            int queued=0, int reserved=0, int channels=0) {
  auto seq=calls.fetch_add(1,std::memory_order_relaxed);
  if (seq<events.size()) events[seq]={seq,id,method,static_cast<uint32_t>(status),arg,result,queued,reserved,channels};
}
class Unity final : public IReaperPitchShift {
  static constexpr int capacity = 65536;
  uint64_t id=next_id.fetch_add(1,std::memory_order_relaxed);
  int nch=2, reserved=0, head=0, queued=0;
  double srate=0, tempo=1, shift=1, formant=1;
  int quality=0;
  bool fault=false;
  std::array<ReaSample,capacity*2> input{}, output{};
  void discard() { reserved=head=queued=0; }
  void fail() { discard(); fault=true; }
  bool format_ok() const { return std::isfinite(srate) && srate>=8000 && srate<=384000 && nch>=1 && nch<=2; }
  int status() const {
    return (format_ok()?0:1) |
      (tempo==1 && shift==1 && (formant==1 || formant==-1)?0:2) |
      (quality==-1 || quality==0 || quality==(12<<16)?0:8) | (fault?4:0);
  }
  int log(FtuMethod method, double arg=0, int result=0) const {
    record(id,method,arg,result,status(),queued,reserved,nch);
    return result;
  }
  void set(double &target, double value, FtuMethod method) {
    if (target!=value) { discard(); target=value; }
    log(method,value);
  }
 public:
  Unity() { live.fetch_add(1,std::memory_order_relaxed); log(FTU_CREATE); }
  ~Unity() override { log(FTU_DESTROY); live.fetch_sub(1,std::memory_order_relaxed); }
  uint64_t identity() const { return id; }
  void set_srate(double v) override { set(srate,v,FTU_SRATE); }
  void set_nch(int n) override { if (nch!=n) { discard(); nch=n; } log(FTU_NCH,n); }
  void set_shift(double v) override { set(shift,v,FTU_SHIFT); }
  void set_formant_shift(double v) override { set(formant,v,FTU_FORMANT); }
  void set_tempo(double v) override { set(tempo,v,FTU_TEMPO); }
  void Reset() override { discard(); fault=false; log(FTU_RESET); }
  ReaSample *GetBuffer(int n) override {
    ReaSample *result=nullptr;
    if (!fault && format_ok() && n!=0) {
      if (n<0 || n>capacity-queued || reserved) fail();
      else { reserved=n; result=input.data(); }
    }
    log(FTU_GETBUFFER,n,result?1:0);
    return result;
  }
  void BufferDone(int n) override {
    if (!fault && format_ok()) {
      if (n<0 || n>reserved) fail();
      else {
        const bool silence=status()!=0;
        for (int f=0;f<n;f++) {
          auto dest=output.data()+((head+queued+f)%capacity)*nch;
          if (silence) std::fill_n(dest,nch,0.0);
          else std::memcpy(dest,input.data()+f*nch,nch*sizeof(ReaSample));
        }
        queued+=n; reserved=0;
      }
    }
    log(FTU_BUFFERDONE,n);
  }
  // Unity has zero latency/tail; committed output is already available.
  // An outstanding reservation is NOT implicitly committed by flush.
  void FlushSamples() override { log(FTU_FLUSH); }
  bool IsReset() override { return log(FTU_ISRESET,0,queued==0 && reserved==0)!=0; }
  int GetSamples(int n, ReaSample *out) override {
    int count=0;
    if (!fault && format_ok() && n!=0) {
      if (n<0 || !out) fail();
      else {
        count=std::min(n,queued);
        for (int f=0;f<count;f++) std::memcpy(out+f*nch,output.data()+((head+f)%capacity)*nch,nch*sizeof(ReaSample));
        head=(head+count)%capacity; queued-=count;
      }
    }
    return log(FTU_GETSAMPLES,n,count);
  }
  void SetQualityParameter(int v) override { if (quality!=v) { discard(); quality=v; } log(FTU_QUALITY,v); }
  int Extended(int call, void *, void *, void *) override {
    int result=call==REAPER_PITCHSHIFT_EXT_GET_CHAN_LIMIT?2:call==FTU_EXT_STATUS?status():0;
    return log(FTU_EXTENDED,call,result);
  }
};
}
EXPORT IReaperPitchShift *ReaperGetPitchShiftAPI(int version) {
  auto object=version==REAPER_PITCHSHIFT_API_VER?new (std::nothrow) Unity:nullptr;
  record(object?object->identity():0,FTU_FACTORY,version,object?1:0);
  return object;
}
EXPORT const char *EnumQualityModes(int submode) {
  record(0,FTU_ENUM,submode,submode==0?1:0);
  return submode==0?"ExtraShifter unity-only adapter (unsupported ratios = SILENCE)":nullptr;
}
// Quiescent-only snapshot: host stops/joins processing before calling this.
// The method does not clear/rearm logging, allocate, or perform I/O.
EXPORT int FlexUnityReadDiagnostics(FtuEvent *out, int capacity_out, uint64_t *dropped, uint64_t *objects) {
  const auto total=calls.load(std::memory_order_relaxed);
  if (dropped) *dropped=total>events.size()?total-events.size():0;
  if (objects) *objects=live.load(std::memory_order_relaxed);
  int n=out && capacity_out>0?static_cast<int>(std::min<uint64_t>(total,std::min(capacity_out,FTU_LOG_CAPACITY))):0;
  if (n) std::memcpy(out,events.data(),n*sizeof(FtuEvent));
  return n;
}
