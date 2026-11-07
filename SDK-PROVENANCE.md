# SDK provenance

Upstream: <https://github.com/justinfrankel/reaper-sdk>

Pinned commit: `b2ca1ec92c9a7864fb72346037986a384971ed03`

Original header:
<https://raw.githubusercontent.com/justinfrankel/reaper-sdk/b2ca1ec92c9a7864fb72346037986a384971ed03/sdk/reaper_plugin.h>

Original complete header SHA-256:
`e94b69bf3dd5573da6c34ac8f35af6911b302c4f79a33be424d2a8e49ba2651b`

Included altered excerpt: `include/pitchshift_abi.h`, SHA-256:
`e86e28b3f719119c30c1068c63a58b33a01c92b88fc560b77dd6bf287b7d8ae7`

The excerpt preserves the original first 24 lines, including the entire Cockos
copyright/license notice, adds an explicit ALTERED marker and `#pragma once`,
uses the upstream default `typedef double ReaSample`, and retains the pitchshift
API version, class/method order, signatures and extension constants unchanged.
It omits unrelated SDK declarations and Windows conditional plumbing. It is
not the full SDK and is not represented as an unmodified upstream file.

For reproducibility, the retained extraction is the concatenation of:

1. Original header lines 1–24 (with original line endings).
2. `\n// ALTERED: focused non-Windows ABI excerpt; class and constants unmodified.\n#pragma once\ntypedef double ReaSample;\n`
3. The original text from `#define REAPER_PITCHSHIFT_API_VER` through the next
   section separator following `#define REAPER_PITCHSHIFT_EXT_GETMINMAXPRODUCTS`
   (separator excluded).

The associated consumer-function documentation is in the same pinned upstream
commit's `sdk/reaper_plugin_functions.h`, SHA-256
`4b77a85d933725721c0731cbfc041c00fa9bc7e4e2e5e257c2ac780a8e8f6c3b`.
It is **not vendored** here. SDK landing page:
<https://www.reaper.fm/sdk/plugin/plugin.php>.

This provenance establishes the class source, not public support for the
undocumented ExtraShifter provider-loading hook. Host findings are summarized
separately in README; no host binary, disassembly or raw private trace is included.
