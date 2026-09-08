# P_RD integration

Upstream: https://github.com/effekseer/EffekseerForUE4
Revision: 6af6f4b691f7db77053cde5870207c64c57fd722
License: MIT, preserved in LICENSE.

Local compatibility changes:

- Initialize FEffekseerUniformProperty::Count to zero to satisfy UE 5.7
  reflected-struct initialization checks.
- Test the Windows macro with `defined(_WIN32)` and leave optional networking
  disabled on Android; its FlatBuffers dependency is not part of this integration.
- Route M_Opaque, M_Opaque_DD and M_Lighting through EfkColorMatProcessMasked.
  These masked materials cannot sample scene depth on ES3.1, so the copied
  function selects a neutral soft-particle fade on mobile. Desktop fade and
  translucent materials retain their original graph. The reproducible migration
  is `Tools/Android/fix_effekseer_masked_materials.py` at the project root.

The reward actor uses an Effekseer system AND emitter with a private scene capture.
Alpha-only effect nodes use the engine white texture as their color input. This
adaptation is in ChestRewardVFX.cpp.
