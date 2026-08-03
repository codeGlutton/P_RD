"""Import the Marchbound combat mercenary-tab base and reusable card states."""

from pathlib import Path

import unreal


SOURCE_DIRECTORY = (
    Path(unreal.Paths.project_dir())
    / "SourceArt"
    / "UI"
    / "Marchbound"
    / "MercenaryTab"
)
DESTINATION_PATH = "/Game/UI/Art/Combat"
IMPORTS = (
    (
        "T_MB_MercenaryTab_Base_20260803_v03.png",
        "T_MercenaryRoster_Shell",
    ),
    (
        "T_MB_MercenaryCard_Normal_20260803_v01.png",
        "T_MB_MercenaryCard_Normal",
    ),
    (
        "T_MB_MercenaryCard_Selected_20260803_v01.png",
        "T_MB_MercenaryCard_Selected",
    ),
)


def import_texture(source_name: str, asset_name: str) -> None:
    source_path = SOURCE_DIRECTORY / source_name
    asset_path = f"{DESTINATION_PATH}/{asset_name}"
    if not source_path.is_file():
        raise RuntimeError(f"Missing mercenary-tab source: {source_path}")

    task = unreal.AssetImportTask()
    task.filename = str(source_path)
    task.destination_path = DESTINATION_PATH
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
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
        raise RuntimeError(f"Could not save imported texture: {asset_path}")

    unreal.log(
        "RD_MERCENARY_TAB_TEXTURE "
        f"source={source_path} asset={asset_path} "
        f"size={texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}"
    )


for source_name, asset_name in IMPORTS:
    import_texture(source_name, asset_name)
