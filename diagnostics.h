#pragma once
#include <cstdint>
// Private diagnostics API v1, not part of the Cockos interface. Read ONLY when
// all interface calls have quiesced. No reset: earliest calls survive saturation.
constexpr int FTU_LOG_CAPACITY = 2048;
constexpr int FTU_EXT_STATUS = 0x46545501;
enum FtuMethod : uint32_t {
  FTU_CREATE=1, FTU_DESTROY, FTU_FACTORY, FTU_ENUM, FTU_SRATE, FTU_NCH,
  FTU_SHIFT, FTU_FORMANT, FTU_TEMPO, FTU_RESET, FTU_GETBUFFER,
  FTU_BUFFERDONE, FTU_FLUSH, FTU_ISRESET, FTU_GETSAMPLES, FTU_QUALITY, FTU_EXTENDED
};
struct FtuEvent {
  uint64_t sequence, object;
  uint32_t method, status;
  double argument;
  int32_t result, queued, reserved, channels;
};
using FtuReadDiagnostics = int (*)(FtuEvent *, int, uint64_t *, uint64_t *);
