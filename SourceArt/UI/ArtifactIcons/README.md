# Artifact icon delivery — 2026-09-07

Mode: built-in imagegen, one independent prompt per asset, no reference image. Original PNGs preserved with alpha; copies saved to F:\코덱스이미지생성폴더 using unique names. Runtime UTexture2D assets live in /Game/SVN/OutSideAsset/AICreation/UI/Artifacts. The corresponding /Game/BP/DataAsset/Artifact data assets bind each texture through mIcon, shared by reward, shop, inventory and detail views.

The user confirmed Rare_Supplement is Shoes. Its existing display name is preserved. Three DA_TestArtifact assets remain test placeholders and are excluded.

Shared prompt: ONE standalone 2D fantasy RPG artifact inventory icon. Polished hand painted game art, chunky legible silhouette, faceted rich shading, crisp dark outlines, high quality material highlights. Centered isolated object, entire object visible with 12 percent empty margin on all sides. Real transparent alpha background, square image. Readable at 48px. No text, no badge, no border, no square background, no floor shadow, no characters, no additional item.

Subjects:
- Common_BrokenBow → BrokenBow: a weathered wooden bow with a visibly snapped upper limb and loose bowstring.
- Common_Dagger → Dagger: a simple short steel dagger, leather wrapped grip, modest brass guard.
- Common_Shoes → RunningShoes: a pair of light brown medieval leather running shoes, soft soles, green laces.
- Common_Sword → Sword: a straight steel arming sword with plain leather grip and brass crossguard.
- Epic_LeftOver → Leftovers: a rustic wooden bowl holding leftover roast meat, bread crust and a green herb, a tiny restorative golden sparkle.
- Epic_ObsidianDagger → ObsidianDagger: a jagged black obsidian dagger with violet glowing cracks and an ornate dark gold handle.
- Epic_SpringShoes → SpringShoes: a whimsical medieval leather boot fitted with a large clearly visible coiled steel spring beneath its sole, emerald accents.
- Rare_Byrnie → Chainmail: a short sleeved silver chainmail shirt, visible interlinked metal rings, leather collar and bronze edging.
- Rare_Oobleck → Oobleck: a corked wide glass jar of thick turquoise non Newtonian magical slime, a dense glossy gel crest curling above its open rim.
- Rare_Seasoning → Seasoning: a squat glass spice shaker filled with red golden seasoning, perforated bronze lid, tiny herbs tied around its neck.
- Rare_Supplement → Shoes: a pair of sturdy dark burgundy medieval leather shoes with prominent brass buckles and reinforced toes, amber highlights; no springs, no wings.

Source PNGs: SourceArt/UI/ArtifactIcons/*_v1.png; import mapping: manifest.json; reimport tool: Tools/import_artifact_icons.py. UI compression, sRGB, no mipmaps, max texture size 512.

