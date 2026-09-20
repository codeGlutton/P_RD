# Status icons — 2026-09-07

Generated with the built-in imagegen tool, one standalone image per prompt, no reference image. Original PNG sources are preserved with alpha. Runtime textures are imported by Tools/import_status_icons.py into /Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons with UI compression, no mipmaps, sRGB, and a maximum texture size of 512.

Shared prompt: Create ONE production game UI status effect icon (do not write text). Polished hand-painted 2D fantasy RPG inventory icon, bold chunky simplified silhouette, faceted painterly shading, crisp dark outline and bright highlights, readable at 40px. Single centered emblem occupying 78% of a square canvas, generous empty margin on every edge. TRANSPARENT BACKGROUND with real alpha. No badge frame, no square background, no words, no letters, no numbers, no grid, no additional icons, no floor or cast shadow. Standalone status icon asset for a medieval fantasy combat HUD.

Subjects:
- Strength: a clenched armored fist, warm gold and steel, strength
- Dexterity: a nimble hand catching a curved feather, teal and gold, dexterity
- Acumeny: a sharp watchful eye inside a faceted violet crystal, insight
- Haste: a winged boot with three sweeping cyan speed streaks, haste
- Exhaustion: a slumped cracked hourglass with a nearly empty sand chamber, desaturated amber and purple, exhaustion
- Slow: a boot weighed down by a heavy iron ball and short chain, cold blue steel, slow movement
- Frail: a cracked breastplate with a broken protective rim, muted red and dark steel, frailty
- Root: a boot tightly bound by twisting thorny roots, purple brown and moss green, immobilized

The three infinite stats share artwork between Buff and Debuff; status classification and floating-log colors remain separate. All status textures are managed in SVN; the PNG import sources remain in SourceArt. The shared C++ lookup owns the mapping for status lists, detail panels, and floating logs.
