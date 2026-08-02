"""지도 톤 통일 패스(v3 지도 + 톤 맞춘 아이콘/연결선/범례) 임포트와 참조 배선.

아이콘은 기존 T_MapNode_*_V2 자산을 **같은 이름으로 덮어쓴다** —
WBP_FrontendMapNode 클래스 디폴트가 이미 그 이름을 물고 있어 재배선이 필요 없다.
연결선은 SVN 공유 폴더 자산을 물고 있어 덮어쓰지 않고 새로 만든 뒤 CDO만 돌린다.
"""

from pathlib import Path

import unreal

SOURCE_DIR = Path(r"D:\UnrealProjects\P_RD_tabletop_map_merc_detail\시안4_7\지도_final")
DESTINATION_PATH = "/Game/UI/Art/RunFlow"

# (소스 파일명, 에셋 이름)
IMPORTS = [
    ("map_scroll_flat_v3.png", "T_StageMap_Scroll_Flat"),
    ("room_icon_monster_tone.png", "T_MapNode_Monster_V2"),
    ("room_icon_elite_tone.png", "T_MapNode_Elite_V2"),
    ("room_icon_boss_tone.png", "T_MapNode_Boss_V2"),
    ("room_icon_shop_tone.png", "T_MapNode_Shop_V2"),
    ("room_icon_treasure_tone.png", "T_MapNode_Treasure_V2"),
    ("room_icon_rest_tone.png", "T_MapNode_Rest_V2"),
    ("map_path_solid_map_tone_v1.png", "T_MapPath_Solid_V2"),
    ("map_path_locked_map_tone_v1.png", "T_MapPath_Locked_V2"),
    ("room_icon_selected_map_tone_v1.png", "T_MapNode_RingSelected_V2"),
    ("map_legend_map_tone_v1.png", "T_StageMap_Legend_V2"),
]


def import_texture(source_name: str, asset_name: str) -> unreal.Texture2D:
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
        raise RuntimeError(f"Could not save texture: {asset_path}")

    unreal.log(
        f"RD_MAP_TONE imported {asset_path} "
        f"size={texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}"
    )
    return texture


def main() -> None:
    textures = {}
    for source_name, asset_name in IMPORTS:
        textures[asset_name] = import_texture(source_name, asset_name)

    # 연결선만 CDO 재배선이 필요하다(기존 참조가 SVN 공유 폴더라 덮어쓰지 않았다).
    line_class = unreal.EditorAssetLibrary.load_blueprint_class("/Game/UI/WBP_FrontendMapLine")
    if line_class is None:
        raise RuntimeError("WBP_FrontendMapLine 없음")
    line_cdo = unreal.get_default_object(line_class)
    line_cdo.set_editor_property("m_solid_texture", textures["T_MapPath_Solid_V2"])
    line_cdo.set_editor_property("m_dashed_texture", textures["T_MapPath_Locked_V2"])
    if not unreal.EditorAssetLibrary.save_asset(
        "/Game/UI/WBP_FrontendMapLine", only_if_is_dirty=False
    ):
        raise RuntimeError("WBP_FrontendMapLine 저장 실패")
    unreal.log("RD_MAP_TONE line CDO -> T_MapPath_Solid_V2 / T_MapPath_Locked_V2")

    unreal.log("RD_MAP_TONE done")


main()
