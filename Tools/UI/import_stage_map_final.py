"""눕힌 지도 최종안 텍스처 3장(빈 책상/원근 지도/룸 아이콘)을 임포트한다."""

from pathlib import Path

import unreal

SOURCE_DIR = Path(r"D:\UnrealProjects\P_RD_tabletop_map_merc_detail\시안4_7\지도_final")
DESTINATION_PATH = "/Game/UI/Art/RunFlow"

IMPORTS = [
    ("world_map_tabletop_empty_desk_rolls_final.png", "T_StageMap_Desk_Final"),
    ("map_scroll_alpha.png", "T_StageMap_Scroll_Final"),
    ("room_icons_alpha.png", "T_StageMap_RoomIcons_Final"),
]


def import_texture(source_name: str, asset_name: str) -> None:
    source = SOURCE_DIR / source_name
    if not source.is_file():
        raise RuntimeError(f"Missing source image: {source}")

    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = DESTINATION_PATH
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = False
    task.save = False

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    asset_path = f"{DESTINATION_PATH}/{asset_name}"
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

    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save imported texture: {asset_path}")

    unreal.log(
        f"RD_STAGE_MAP_FINAL_IMPORT asset={asset_path} "
        f"size={texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}"
    )


def main() -> None:
    for source_name, asset_name in IMPORTS:
        import_texture(source_name, asset_name)


main()
