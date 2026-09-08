# Live chest effect

Original CC0 source: effekseer/EffectMaterials, revision
8bf8edfedb51ac09af3d4cf788cb81746ea75a82. See UPSTREAM-README.md.

The original holy effect is retained for provenance but is no longer used by
the reward widget: its rotating ribbons did not match the chest reference.

The replacement is chest_radiance, an authored particle composition using the
CC0 glow, shock-ring, ring and star textures. It combines light from the opening,
seven rising shafts, faint expanding radiance and eighteen independently moving
sparkles. No image-generated backdrop or baked animation is used.

Rebuild the editable XML with Tools/author_chest_radiance.py, then compile with
the official Effekseer 1.80.7 tool:
`Effekseer.exe -cui -in chest_radiance.efkproj -o chest_radiance.efkefc`.
Run Tools/import_chest_radiance.py in the full Unreal editor Python runner
(-ExecutePythonScript); Interchange requires Slate. The original import script
is only needed for initial shared textures and the UI material.

Runtime: ChestRewardVFX captures live particles into a 768x512 render target and
uses an additive UI material behind the chest. Warm golden tint, scale and speed
are applied by the emitter. The actor is transient and destroyed at completion,
skip, reward reset or widget removal. This is live simulation, not a flipbook.

For review: RD.Editor.StartVisualReviewPIE starts a separate PIE window;
RD.VisualReview in the game world opens repeat controls for chest/ally/monster.
