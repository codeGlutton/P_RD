"""룸 아이콘 v2 6종을 임포트하고 노드 WBP 클래스 디폴트·지도 범례 참조를 교체한다.

기존 아이콘은 /Game/SVN(정션·공유 폴더) 쪽 에셋이라 덮어쓰지 않는다 —
새 텍스처를 git 관리 폴더에 두고 WBP 참조만 이쪽으로 돌린다.
"""

from pathlib import Path

import unreal

SOURCE_DIR = Path(r"D:\UnrealProjects\P_RD_tabletop_map_merc_detail\시안4_7\지도_final")
DESTINATION_PATH = "/Game/UI/Art/RunFlow"

# (소스 파일 접미사, 에셋 이름, 노드 CDO 프로퍼티, 옛 경로 키워드)
ICONS = [
    ("monster", "T_MapNode_Monster_V2", "m_icon_monster_texture", "monster"),
    ("elite", "T_MapNode_Elite_V2", "m_icon_elite_texture", "elite"),
    ("boss", "T_MapNode_Boss_V2", "m_icon_boss_texture", "boss"),
    ("shop", "T_MapNode_Shop_V2", "m_icon_shop_texture", "shop"),
    ("treasure", "T_MapNode_Treasure_V2", "m_icon_treasure_texture", "treasure"),
    ("rest", "T_MapNode_Rest_V2", None, "rest"),
]


def import_icon(suffix: str, asset_name: str) -> unreal.Texture2D:
    source = SOURCE_DIR / f"room_icon_{suffix}_v2.png"
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
    unreal.log(f"RD_ICON_V2 imported {asset_path}")
    return texture


def keyword_of(path_name: str):
    lowered = path_name.lower()
    # boss 아이콘 계열 이름에 monster 가 섞이지 않도록 구체적인 것부터 본다.
    for keyword in ("elite", "boss", "shop", "treasure", "rest", "camp", "monster"):
        if keyword in lowered:
            return "rest" if keyword == "camp" else keyword
    return None


def import_over(source_name: str, asset_name: str) -> None:
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
        f"RD_ICON_V2 imported {asset_path} "
        f"size={texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}"
    )


def main() -> None:
    # 코덱스 v2 본체 이미지: 평평한 지도(리테이너가 원근을 건다) + 빈 책상 배경
    import_over("map_scroll_flat_v2.png", "T_StageMap_Scroll_Flat")
    import_over("world_map_desk_v2.png", "T_StageMap_Desk_Final")

    textures = {}
    for suffix, asset_name, _prop, _keyword in ICONS:
        textures[suffix] = import_icon(suffix, asset_name)

    # 1) 노드 WBP 클래스 디폴트 교체
    node_class = unreal.EditorAssetLibrary.load_blueprint_class("/Game/UI/WBP_FrontendMapNode")
    if node_class is None:
        raise RuntimeError("WBP_FrontendMapNode 없음")
    cdo = unreal.get_default_object(node_class)
    for suffix, _asset, prop, _keyword in ICONS:
        if prop is None:
            continue
        cdo.set_editor_property(prop, textures[suffix])
        unreal.log(f"RD_ICON_V2 node.{prop} -> {textures[suffix].get_path_name()}")
    if not unreal.EditorAssetLibrary.save_asset("/Game/UI/WBP_FrontendMapNode", only_if_is_dirty=False):
        raise RuntimeError("WBP_FrontendMapNode 저장 실패")

    # 2) 지도 WBP 안 이미지(범례 등)가 옛 아이콘을 물고 있으면 같은 종류의 v2 로 교체
    map_bp = unreal.load_asset("/Game/UI/WBP_FrontendMap")
    tree = map_bp.get_editor_property("widget_tree")
    swapped = [0]

    def walk(widget):
        if widget is None:
            return
        if isinstance(widget, unreal.Image):
            brush = widget.get_editor_property("brush")
            resource = brush.get_editor_property("resource_object")
            if resource is not None:
                path_name = resource.get_path_name()
                unreal.log(f"RD_ICON_V2 image {widget.get_name()} = {path_name}")
                if "MapNode" in path_name or "legend" in path_name.lower():
                    keyword = keyword_of(path_name)
                    if keyword in textures:
                        brush.set_editor_property("resource_object", textures[keyword])
                        widget.set_editor_property("brush", brush)
                        swapped[0] += 1
                        unreal.log(
                            f"RD_ICON_V2 swapped {widget.get_name()} -> {keyword}"
                        )
        if isinstance(widget, unreal.PanelWidget):
            for index in range(widget.get_children_count()):
                walk(widget.get_child_at(index))
        elif isinstance(widget, unreal.ContentWidget):
            walk(widget.get_content())

    walk(tree.get_editor_property("root_widget"))
    if swapped[0] > 0:
        if not unreal.EditorAssetLibrary.save_asset("/Game/UI/WBP_FrontendMap", only_if_is_dirty=False):
            raise RuntimeError("WBP_FrontendMap 저장 실패")
    unreal.log(f"RD_ICON_V2 done swapped={swapped[0]}")


main()
