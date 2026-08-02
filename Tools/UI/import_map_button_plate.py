"""지도 UI 버튼 판(BACK 단추 바탕) 텍스처를 임포트한다."""

from pathlib import Path

import unreal

SOURCE = Path(
    r"D:\UnrealProjects\P_RD_tabletop_map_merc_detail\시안4_7\지도_final\ui_button_plate_trimmed.png"
)
DESTINATION_PATH = "/Game/UI/Art/RunFlow"
ASSET_NAME = "T_MapUI_ButtonPlate_V1"


def main() -> None:
    if not SOURCE.is_file():
        raise RuntimeError(f"Missing source image: {SOURCE}")

    task = unreal.AssetImportTask()
    task.filename = str(SOURCE)
    task.destination_path = DESTINATION_PATH
    task.destination_name = ASSET_NAME
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = False
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    asset_path = f"{DESTINATION_PATH}/{ASSET_NAME}"
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
        raise RuntimeError(f"Could not save texture: {asset_path}")

    unreal.log(
        f"RD_MAP_BUTTON {asset_path} "
        f"size={texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}"
    )


main()
