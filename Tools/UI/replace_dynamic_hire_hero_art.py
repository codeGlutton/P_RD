"""Replace all six hire-screen hero textures with centered dynamic v03 art."""

from pathlib import Path

import unreal


SOURCE_DIR = (
    Path(unreal.Paths.project_dir())
    / "SourceArt"
    / "UI"
    / "Marchbound"
    / "Mercenaries"
    / "Illustrations"
)
DESTINATION_PATH = "/Game/UI/Art/Marchbound/Mercenaries"
MERCENARY_NAMES = ("Knight", "Mage", "Ranger", "Rogue", "Barbarian", "Druid")


def replace_texture(mercenary_name: str) -> None:
    source_version = "v04" if mercenary_name == "Knight" else "v03"
    source = SOURCE_DIR / f"T_MB_HireHero_{mercenary_name}_20260803_{source_version}.png"
    if not source.is_file():
        raise RuntimeError(f"Missing dynamic hire source: {source}")

    asset_name = f"T_MB_HireHero_{mercenary_name}"
    asset_path = f"{DESTINATION_PATH}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        if not unreal.EditorAssetLibrary.delete_asset(asset_path):
            raise RuntimeError(f"Could not delete existing hero texture: {asset_path}")

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
        raise RuntimeError(f"Replacement is not a Texture2D: {asset_path}")

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
        raise RuntimeError(f"Could not save dynamic hero texture: {asset_path}")

    unreal.log(
        f"RD_MB_DYNAMIC_HERO_REPLACE asset={asset_path} source={source} "
        f"size={texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}"
    )


for name in MERCENARY_NAMES:
    replace_texture(name)

unreal.log("RD_MB_DYNAMIC_HERO_REPLACE success count=6")
