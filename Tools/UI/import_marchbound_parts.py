"""Marchbound 모바일 UI 공용 파츠를 프로젝트 로컬 Texture2D로 임포트한다."""

from pathlib import Path

import unreal


SOURCE_DIR = Path(r"D:\UnrealProjects\P_RD_develop_20260803\SourceArt\UI\Marchbound")
DESTINATION_PATH = "/Game/UI/Art/Marchbound"

IMPORTS = (
    ("T_MB_ActionButtonFrame_20260803.png", "T_MB_ActionButtonFrame"),
    ("T_MB_TurnTokenSpeed_20260803.png", "T_MB_TurnTokenSpeed"),
    ("T_MB_StatusSocket_20260803.png", "T_MB_StatusSocket"),
    ("T_MB_GenericDetailPanel_20260803.png", "T_MB_GenericDetailPanel"),
    ("T_MB_KnightSelectionSquare_20260803.png", "T_MB_KnightSelectionSquare"),
    (
        "T_MB_KnightSelectionIllustration2D_20260803_v01.png",
        "T_MB_KnightSelectionIllustration2D",
    ),
    (
        "T_MB_KnightSelectionIllustration2D_20260803_v02.png",
        "T_MB_KnightSelectionIllustration2D_v02",
    ),
)

HIRE_SOURCE_DIR = SOURCE_DIR / "Hire"
HIRE_DESTINATION_PATH = f"{DESTINATION_PATH}/Hire"
HIRE_IMPORTS = (
    ("T_MB_HireListFrame_20260803.png", "T_MB_HireListFrame"),
    ("T_MB_HireRowNormal_20260803.png", "T_MB_HireRowNormal"),
    ("T_MB_HireRowSelected_20260803.png", "T_MB_HireRowSelected"),
    ("T_MB_HireBackButton_20260803.png", "T_MB_HireBackButton"),
    ("T_MB_HireTitlePlate_20260803.png", "T_MB_HireTitlePlate"),
    ("T_MB_HirePartyFrame_20260803.png", "T_MB_HirePartyFrame"),
    ("T_MB_HirePartyRowPlus_20260803.png", "T_MB_HirePartyRowPlus"),
    ("T_MB_HirePartyRowEmpty_20260803.png", "T_MB_HirePartyRowEmpty"),
    ("T_MB_HireDepartButton_20260803.png", "T_MB_HireDepartButton"),
    ("T_MB_HireNamePlate_20260803.png", "T_MB_HireNamePlate"),
    ("T_MB_HireStatsStrip_20260803.png", "T_MB_HireStatsStrip"),
    ("T_MB_HireSkillButtonFrame_20260803.png", "T_MB_HireSkillButtonFrame"),
)

MERCENARY_SOURCE_DIR = SOURCE_DIR / "Mercenaries"
MERCENARY_DESTINATION_PATH = f"{DESTINATION_PATH}/Mercenaries"
MERCENARY_NAMES = ("Knight", "Mage", "Ranger", "Rogue", "Barbarian", "Druid")
MERCENARY_IMPORTS = tuple(
    (
        MERCENARY_SOURCE_DIR
        / "Icons"
        / f"T_MB_HireIcon_{name}_20260803_{'v02' if name == 'Rogue' else 'v01'}.png",
        f"T_MB_HireIcon_{name}",
    )
    for name in MERCENARY_NAMES
) + tuple(
    (
        MERCENARY_SOURCE_DIR
        / "Illustrations"
        / f"T_MB_HireHero_{name}_20260803_{'v04' if name == 'Knight' else 'v03'}.png",
        f"T_MB_HireHero_{name}",
    )
    for name in MERCENARY_NAMES
)


def import_texture(
    source_dir: Path,
    destination_path: str,
    source_name: str,
    asset_name: str,
) -> None:
    source = source_dir / source_name
    if not source.is_file():
        raise RuntimeError(f"Missing Marchbound source image: {source}")

    asset_path = f"{destination_path}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log(f"RD_MARCHBOUND_PART reuse={asset_path}")
        return

    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = destination_path
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = False
    task.replace_existing_settings = False
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Imported asset is not a Texture2D: {asset_path}")

    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("never_stream", True)
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        texture, only_if_is_dirty=False
    ):
        raise RuntimeError(f"Could not save Marchbound texture: {asset_path}")

    unreal.log(
        f"RD_MARCHBOUND_PART asset={asset_path} "
        f"size={texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}"
    )


def import_texture_path(source: Path, destination_path: str, asset_name: str) -> None:
    import_texture(source.parent, destination_path, source.name, asset_name)


def main() -> None:
    for source_name, asset_name in IMPORTS:
        import_texture(SOURCE_DIR, DESTINATION_PATH, source_name, asset_name)
    for source_name, asset_name in HIRE_IMPORTS:
        import_texture(
            HIRE_SOURCE_DIR,
            HIRE_DESTINATION_PATH,
            source_name,
            asset_name,
        )
    for source, asset_name in MERCENARY_IMPORTS:
        import_texture_path(source, MERCENARY_DESTINATION_PATH, asset_name)


main()
