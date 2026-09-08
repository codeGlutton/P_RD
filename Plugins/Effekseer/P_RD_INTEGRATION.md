# P_RD integration

Upstream: https://github.com/effekseer/EffekseerForUE4
Revision: 6af6f4b691f7db77053cde5870207c64c57fd722
License: MIT, preserved in LICENSE.

Local compatibility change: initialize FEffekseerUniformProperty::Count to zero
to satisfy UE 5.7 reflected-struct initialization checks.

The reward actor uses an Effekseer system AND emitter with a private scene capture.
Alpha-only effect nodes use the engine white texture as their color input. This
adaptation is in ChestRewardVFX.cpp; upstream runtime code is otherwise unchanged.
