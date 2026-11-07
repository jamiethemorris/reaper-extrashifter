# reaper-extrashifter

Source-only example of REAPER's **undocumented reserved ExtraShifter loading
hook**, with a bounded **unity-only** C++ provider and synthetic loader tests.
This is not a supported public provider SDK, arbitrary engine registration API,
or a completed Logic/Polyphonic integration.

## What the hook is (and is not)

The investigated REAPER **7.84.0_310d2afu, macOS arm64** build looks for the
specific file `Contents/Plugins/extrashifter.dylib` inside its application
bundle. It resolves the C exports `ReaperGetPitchShiftAPI` and
`EnumQualityModes`; a `ConfigureSubMode` lookup also exists, but its contract is
not implemented or guessed here. When the factory is available the reserved
main mode **12**, `ExtraShifter`, is exposed. The provider controls submode
labels, not arbitrary new top-level mode IDs. This differs from ordinary
`reaper_*.dylib` extensions and `ReaperPluginEntry` registration.

The official SDK's `IReaperPitchShift`, factory access, mode enumeration and
selection describe a **consumer interface**. Using that class here does **not**
make the reserved provider-loading path a documented/supported registration
contract. No compatibility promise from Cockos is implied. Other builds,
architectures and Windows/Linux host loading paths remain unverified.

## Minimal standalone build and tests

Requires Python 3 and a C++17 compiler on macOS or Linux:

```sh
python3 build.py
```

`CXX` can select the compiler. No downloads, SDK installation, DAW launch,
audio-device access, app-bundle edit or plug-in installation is performed.
The script verifies the pinned SDK excerpt, builds the provider and POSIX
synthetic loader, then runs all six provider test groups. macOS output is
`build/extrashifter.dylib`; Linux output is `build/extrashifter.so` for synthetic
loading only, **not** a verified Linux REAPER hook. `build/test-results.json`
contains local compiler/test receipts. All generated files are ignored.
Windows requires separate ABI/export/build work and is not supported here.

Tests exercise actual dynamic loading, version rejection, virtual deletion,
mono/stereo interleaving and bit integrity, partial reads/canaries, flush,
nonunity silence/status, invalid rate/ratio/quality/version handling, protocol
faults, reset, repeated setters, ring wrap/capacity and bounded diagnostics.
They use synthetic numbers, not private audio. They are ordinary local native
processes, **not sandbox acceptance tests or real-host render tests**.

## Reusable provider contract

- Export `ReaperGetPitchShiftAPI(int version)` with C linkage and default symbol
  visibility. Accept exactly `REAPER_PITCHSHIFT_API_VER` (`0x14`) or return null.
  The caller owns returned objects and deletes through the virtual destructor;
  all objects must be destroyed before unloading the provider.
- `EnumQualityModes(int submode)` returns the static label for submode zero and
  null for other values. This sample intentionally has one unity-only mode.
- `ReaSample` is `double`; buffers are **interleaved**, counts are **frames**.
  `GetBuffer(n)` reserves provider-owned input until commit/reset/change/fault;
  `BufferDone(k)` commits only the first k reserved frames. `GetSamples` writes
  only the returned frame count. The caller must supply adequate output storage.
- This adapter admits 1–2 channels and finite sample rates 8000–384000 Hz.
  Input/output storage is bounded to 65536 frames. Zero input requests return
  null. Negative/oversized requests, overcommit, second reservations and invalid
  positive-output pointers latch a protocol fault and discard buffered audio.
- Same-valued setters preserve pending input/output. Actual configuration changes
  discard old-domain data. Reset clears queue/reservation/fault, retaining format
  and quality. Unity has no latency/tail; flush is idempotent and never commits
  an outstanding reservation. These are **sample adapter policies**, not proven
  universal requirements for all REAPER algorithms.
- Exact tempo/shift 1 and formant 1 (or -1) are admitted; quality 0, -1 or
  `12<<16` selects the same unity behavior. Unsupported ratios/quality emit
  **silence with one output frame per committed input frame**, not correct
  nonunity stretching or a successful passthrough. Do not use this mode on real
  projects expecting DSP. Unknown extensions return zero.
- Sample-private `Extended(0x46545501)` status bits: 1 invalid format, 2 unsupported
  ratio/formant, 4 protocol fault, 8 unsupported quality. Channel-limit extension
  `0x100` returns 2. Diagnostics are not part of the public Cockos interface.
- Calls on one object must be serialized. Independent objects can log concurrently
  into a lock-free bounded first-2048-event buffer; excess calls increment a
  dropped count. `FlexUnityReadDiagnostics` is **quiescent-only**, after all
  processing threads have stopped/joined. It must not race writes. No audio,
  pointers or user identity are logged; event sequence is not wall-clock timing.

To use a real engine later, preserve the published class order/ABI and explicit
ownership contract; replace the unity behavior only after tests establish its
latency, input-demand, EOF/flush, reset, reconfiguration and error policies.
This tree deliberately does not add that integration.

## Host observations and controlled validation workflow

A prior controlled test on the investigated macOS arm64 host exercised its real
consumer wrapper directly through reserved mode 12: **143 mono frames at 48 kHz**
passed exact synthetic-value/canary checks, drain/reset and factory/destruction
controls. The provider recorded **248 events**, zero dropped
and zero live objects at snapshot. The wrapper reapplied each format/ratio
setter **40 times** without losing queued data. These are summarized historical
findings, not results of `build.py`, and raw private logs are not distributed.

That test was **not REAPER playback, item processing, rendering, nonunity DSP,
stereo host validation or MADSP execution**. It does not establish full host
scheduling, maximum real render demand, concurrency, fallback, all settings, or
quality. Portable synthetic tests are rerun here; the historical host harness is
intentionally excluded because its machine ownership/launch guards are not a
portable product interface.

For a separately authorized host experiment:

1. Use your own normally installed/authorized matching REAPER and a fresh
   disposable portable copy with isolated config/projects/plugins. Never replace
   an existing provider or modify the normal app/profile as part of this build.
2. Review platform signing/loading restrictions before any install or launch.
   Stop at OS prompts or incompatibility; do not disable protections, bypass
   authorization or silently re-sign the app.
3. Place the matching-architecture macOS provider only in the disposable copy's
   `Contents/Plugins/`, preserving the exact filename. This is a manual, opt-in
   experiment; no installer is supplied. Set Dummy Audio, no MIDI inputs/outputs,
   no hardware sends, a stopped empty muted project and no restore/autoplay.
4. First verify the ExtraShifter main entry and unique unity-only submode label.
   Recognition alone is not execution. Under separately reviewed ownership and
   device/network controls, consume the host factory at version 0x14, select
   `(12<<16)|0`, configure format/unity, run a bounded synthetic frame driver and
   verify values/canaries plus quiescent diagnostics after virtual deletion.
   No general host driver or one-command safe-launch claim is supplied here.
5. Only a later explicitly scoped render experiment can validate real item/render
   call order, EOF/tails and buffering. Keep normal projects/audio outside it.

The investigated host's exact hook may change or disappear. Ask Cockos for a
supported provider contract before depending on it in a distributable product.
See `SDK-PROVENANCE.md` and `LICENSING.md` for source origin and rights.
