"""Import the transparent MARCHBOUND defeat-result UI parts as UI textures."""

from pathlib import Path

import unreal


SOURCE_DIR = Path(
    r"D:\UnrealProjects\P_RD_develop_20260803\SourceArt\UI\Marchbound\DefeatParts"
)
DESTINATION_PATH = "/Game/UI/Art/Marchbound/Defeat"

IMPORTS = (
    ("MARCHBOUND_Defeat_Part_OuterFrame_20260803_v01.png", "T_MB_Defeat_OuterFrame"),
    ("MARCHBOUND_Defeat_Part_TitleBanner_Red_20260803_v01.png", "T_MB_Defeat_TitleBanner"),
    ("MARCHBOUND_Defeat_Part_MercenaryCard_Empty_20260803_v01.png", "T_MB_Defeat_MercenaryCard"),
    ("MARCHBOUND_Defeat_Part_BattleSummary_Empty_20260803_v01.png", "T_MB_Defeat_BattleSummary"),
    ("MARCHBOUND_Defeat_Part_Button_Secondary_Empty_20260803_v01.png", "T_MB_Defeat_ButtonSecondary"),
    ("MARCHBOUND_Defeat_Part_Button_Primary_Empty_20260803_v01.png", "T_MB_Defeat_ButtonPrimary"),
)


def import_texture(source_name: str, asset_name: str) -> None:
    source = SOURCE_DIR / source_name
    if not source.is_file():
        raise RuntimeError(f"Missing defeat UI source image: {source}")

    asset_path = f"{DESTINATION_PATH}/{asset_name}"
    if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        task = unreal.AssetImportTask()
        task.filename = str(source)
        task.destination_path = DESTINATION_PATH
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
        raise RuntimeError(f"Could not save defeat UI texture: {asset_path}")

    unreal.log(
        f"RD_DEFEAT_PART asset={asset_path} "
        f"size={texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}"
    )


def main() -> None:
    for source_name, asset_name in IMPORTS:
        import_texture(source_name, asset_name)


main()
