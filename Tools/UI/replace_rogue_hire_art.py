"""Replace the incorrect unmasked Rogue hire art with Rogue_Hooded v02 art."""

from pathlib import Path

import unreal


SOURCE_ROOT = (
    Path(unreal.Paths.project_dir())
    / "SourceArt"
    / "UI"
    / "Marchbound"
    / "Mercenaries"
)
DESTINATION_PATH = "/Game/UI/Art/Marchbound/Mercenaries"
IMPORTS = (
    (
        SOURCE_ROOT / "Icons" / "T_MB_HireIcon_Rogue_20260803_v02.png",
        "T_MB_HireIcon_Rogue",
    ),
    (
        SOURCE_ROOT
        / "Illustrations"
        / "T_MB_HireHero_Rogue_20260803_v02.png",
        "T_MB_HireHero_Rogue",
    ),
)


def replace_texture(source: Path, asset_name: str) -> None:
    if not source.is_file():
        raise RuntimeError(f"Missing corrected Rogue source: {source}")

    asset_path = f"{DESTINATION_PATH}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        if not unreal.EditorAssetLibrary.delete_asset(asset_path):
            raise RuntimeError(f"Could not delete incorrect Rogue texture: {asset_path}")

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
        raise RuntimeError(f"Could not save corrected Rogue texture: {asset_path}")

    unreal.log(
        f"RD_MB_ROGUE_REPLACE asset={asset_path} source={source} "
        f"size={texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}"
    )


for source_file, destination_name in IMPORTS:
    replace_texture(source_file, destination_name)

unreal.log("RD_MB_ROGUE_REPLACE success")
