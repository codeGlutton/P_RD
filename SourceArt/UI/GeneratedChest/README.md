# Chest VFX V2 — 2026-09-07

Built-in image_gen was used (not the API/CLI fallback).

## References
- User-provided treasure chest light reference: gold dome, horizontal wave, white-yellow highlights and floating sparks.
- https://zy-vfx.itch.io/golden-light-aesthetic — transparent opening/loop FX and PNG sequence packaging reference; no commercial asset downloaded or purchased.
- https://docs.coregames.com/tutorials/vfx_tutorial/ — separate chest opening VFX and timed effect lifecycle reference.

## Final project assets
- SourceArt/UI/GeneratedChest/gold_dome_v2.png (1536x1024, additive RGB source).
- SourceArt/UI/GeneratedChest/chest_clean_atlas_v2.png (1537x1023, 6x6 layout, 33 occupied frames).
- /Game/UI/RewardConcept03New/Generated/T_ChestGoldDome_V2
- /Game/UI/RewardConcept03New/Generated/T_ChestCleanAtlas_V2

The light source is converted from black-background emitted color to straight alpha by the Unreal importer. The clean chest sheet is black-keyed and all frame edges receive transparent guard pixels. Existing SVN originals remain unchanged. The regenerated chest atlas has lower per-frame resolution than the original atlas; it removes the baked clipped rays without retaining those ray silhouettes. A trial using the original color detail was rejected because it retained cropped rays.

The runtime replaces the procedural ring with generated light art behind the chest, keeps the 33-frame sequence and reward flow, and clears clipping only along the chest's ancestor branch. Experience progress clips are unchanged. Source files and uassets have explicit Git ignore exceptions.

## Rebuild
Build P_RDEditor, then run UnrealEditor-Cmd with the project and:
-run=pythonscript -script=<project>/Tools/import_generated_chest_art.py -unattended -nop4 -nullrhi

## Prompt set

### Initial light design (reference: user's image; style/shape only)
Use case: stylized-concept. Asset type: high quality production game VFX sprite, transparent PNG, landscape 1536x1024. Reference image is STYLE/SHAPE reference only. Create ONLY the magical gold light effect from a treasure chest opening, with NO chest, NO coins, NO background, NO text, NO frame. A luxurious luminous hemispherical golden energy dome, a wide perspective horizontal elliptical shockwave around its lower middle, billowing silky gold light and streaks rising from a concentrated source at x50% y65%. Painterly cinematic fantasy game VFX, organic irregular brightness, broad amber bloom around white-yellow cores, wispy flowing light, varied tiny floating luminous motes. Not thin uniform geometric lines or flat neon wireframe. Keep central lower area fairly transparent so an existing chest can show through. Dome arch occupies center 60% width and60% height, horizontal wave reaches80% width. All light including bloom fully contained with generous transparent safety padding at least10% on ALL four edges; fade all outer edges smoothly to alpha0. REAL transparent alpha background, no checkerboard pixels, no black rectangle, no opaque white background, no objects. Polished premium fantasy game effect. This asset will be animated in Unreal by scale and opacity.

The first output contained an opaque checkerboard and was not used as a runtime asset.

### Final light edit
Edit target: the provided gold VFX image. Preserve the exact gold dome, horizontal wave and ethereal light design. Replace ALL checkerboard background with perfectly pure black RGB 0,0,0 throughout; no checkerboard remains anywhere, including inside the translucent effect. This is a GAME ADDITIVE VFX TEXTURE, emitted light on black, not a composited scene. Render the light as actual luminous gold against pure black with smooth glow fading to exact black. Reduce the opaque smoky fill inside the dome substantially so most of the interior is black and a chest can later be composited in front. Widen empty black safety padding to at least12% every edge, keep all bloom inside canvas. No gray/white backdrop, no checkerboard, no chest, no text.

### Clean chest atlas edit (reference: original exported 33-frame atlas)
Precise image edit for game sprite atlas. Preserve the EXACT 6 column by 6 row grid; 33 occupied cells, final row first3 occupied and final3 empty. Preserve each chest/coins/lock silhouette, pose, size, position and chronological opening animation. Change ONLY baked lighting around the objects: completely remove ALL outer rays, starbursts, glow, haze, sparks and lens flare outside the chest and coins in EVERY cell, especially the bright rays cut off at the top of each cell. Keep normal warm illumination on the wood and gold coins. All background and gaps PURE BLACK RGB0,0,0, not checkerboard. No added objects. Each cell has a clean isolated chest and coins with EMPTY black safe margins, no light reaches a cell edge. No grids/lines/text. Maintain exact canvas aspect 3:2 and output as high resolution as possible, ideally 4092x2730. Critical preserve33 frames and6x6 exact placement, no changing frame layout.

Actual generated dimensions are recorded above, not the requested ideal dimensions.
