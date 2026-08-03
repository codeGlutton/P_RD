"""Import generated transparent combat HUD icons."""

from pathlib import Path

import unreal


SOURCE_DIR = (
    Path(unreal.Paths.project_dir())
    / "SourceArt"
    / "UI"
    / "Marchbound"
    / "Combat"
)
DESTINATION = "/Game/UI/Art/Marchbound/Combat"
IMPORTS = (
    ("T_MB_CombatTab_Mercenary_20260803_v02.png", "T_MB_CombatTab_Mercenary"),
    ("T_MB_CombatTab_Monster_20260803_v03.png", "T_MB_CombatTab_Monster"),
    ("T_MB_Icon_Speed_20260803_v01.png", "T_MB_Icon_Speed"),
    ("T_MB_TurnToken_Frame_20260803_v01.png", "T_MB_TurnToken_Frame"),
    ("T_MB_OptionsRail_Frame_20260803_v02.png", "T_MB_OptionsRail_Frame"),
    ("T_MB_OptionsIcon_Map_20260803_v01.png", "T_MB_OptionsIcon_Map"),
    ("T_MB_OptionsIcon_Settings_20260803_v01.png", "T_MB_OptionsIcon_Settings"),
    (
        "T_MB_OptionsIcon_MercenarySymbol_20260803_v01.png",
        "T_MB_OptionsIcon_MercenarySymbol",
    ),
    (
        "T_MB_OptionsIcon_MonsterSymbol_20260803_v01.png",
        "T_MB_OptionsIcon_MonsterSymbol",
    ),
    (
        "T_MB_ArtifactTray_Frame_20260803_v01_trimmed.png",
        "T_MB_ArtifactTray_Frame",
    ),
    (
        "T_MB_StatusTray_Frame_20260803_v01_trimmed.png",
        "T_MB_StatusTray_Frame",
    ),
    (
        "T_MB_RoundBadge_Frame_20260803_v01_trimmed.png",
        "T_MB_RoundBadge_Frame",
    ),
    (
        "T_MB_OptionsIcon_MercenaryGlyph_20260803_v02_trimmed.png",
        "T_MB_OptionsIcon_MercenaryGlyph",
    ),
    (
        "T_MB_OptionsIcon_MonsterGlyph_20260803_v02_trimmed.png",
        "T_MB_OptionsIcon_MonsterGlyph",
    ),
    (
        "T_MB_Icon_SpeedGlyph_20260803_v02_trimmed.png",
        "T_MB_Icon_SpeedGlyph",
    ),
    (
        "T_MB_ArtifactSlot_Frame_20260803_v01.png",
        "T_MB_ArtifactSlot_Frame",
    ),
    (
        "T_MB_StatusSlot_Frame_20260803_v01.png",
        "T_MB_StatusSlot_Frame",
    ),
)


for source_name, asset_name in IMPORTS:
    source = SOURCE_DIR / source_name
    if not source.is_file():
        raise RuntimeError(f"Missing combat HUD icon source: {source}")
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = DESTINATION
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    imported = unreal.load_asset(f"{DESTINATION}/{asset_name}")
    if not isinstance(imported, unreal.Texture2D):
        raise RuntimeError(f"Could not import {asset_name}")
    imported.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    imported.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    imported.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(imported, only_if_is_dirty=False)
    unreal.log(f"RD_COMBAT_HUD_ICON_IMPORT asset={imported.get_path_name()}")

unreal.log(f"RD_COMBAT_HUD_ICON_IMPORT success count={len(IMPORTS)}")
