# Feedback UI art — 2026-09-15

These assets repair the English title and the incorrect rat images (issue #735).
Runtime assets live in `/Game/SVN/OutSideAsset/AICreation/UI/Feedback_20260915` and are included explicitly in cooking.
PNG source files and this provenance record are stored in SVN at
`SourceArt/UI/Feedback_20260915`. Runtime textures and the logo material are
versioned together in SVN; Git contains only their references and implementation.

| Asset | Use / reference |
|---|---|
| T_TitleLogo_English | English variant of the existing Korean castle/sword/banner logo. Exact title: MERCENARY GUILD OF THE RUINED KINGDOM. |
| T_TitleLogo_EnglishMask | Generated white emblem silhouette on black; the UI material uses its red channel as opacity. |
| M_TitleLogo_English | UI material composing the English logo and its opacity mask. |
| T_Rat_Head | Head portrait based on SKM_ms06_Rat with MI_ms06_Rat_Material_1 from SVN r442. |
| T_Rat_Action | Full-body sling pose of the same rat; used for portrait and skill cut-in. |

The PNGs were generated with the built-in image generation tool using the original
logo and an Unreal render of the actual rat model as references. Rat constraints:
cream/beige color, tall angular ears, orange-red neckerchief, teal trousers, brown
belt/boots, wooden sling, faceted fantasy game style. No skeleton or mimic imagery.
English logo constraints: preserve castle, sword, blue banners and golden lettering;
replace Korean lettering with the exact English title. The separate mask removes
background pixels while retaining the emblem in the existing menu layout.

The five enemy equipment icons reuse the project's existing art:
Werewolf Heart → PredatorsHeart; Ghoul's Fury → Claw; Rotten Regeneration → Druid Heal;
Harvest → Scythe; Bone Hardening → BonePunch. Their original credit provenance is unchanged.

## Validation

Editor build and Android arm64 Shipping compile pass. Seven focused automation
checks pass: Feedback DisplayAndScrolling, ItemLocalizationAndRatAssets,
TitleLanguage; title FoldLayoutBounds and MenuRowsClickable; SkillsAndReplacement
localization; RewardConcept03New Interaction. English and Korean archives each
pass the 1,585-entry translation audit. ADB had no connected device during validation;
a full Android cook/Play release and a physical-device check are not part of this result.
