"""WBP_FrontendMap 범례 아이콘 이미지가 물고 있는 옛 노드 아이콘을 v2 로 교체한다.

WidgetBlueprint.widget_tree 는 5.7 파이썬에 노출되지 않아 서브오브젝트 경로로 접근한다.
"""

import unreal

DESTINATION_PATH = "/Game/UI/Art/RunFlow"
KEYWORD_TO_ASSET = {
    "elite": "T_MapNode_Elite_V2",
    "boss": "T_MapNode_Boss_V2",
    "shop": "T_MapNode_Shop_V2",
    "treasure": "T_MapNode_Treasure_V2",
    "rest": "T_MapNode_Rest_V2",
    "camp": "T_MapNode_Rest_V2",
    "monster": "T_MapNode_Monster_V2",
}


def keyword_of(path_name: str):
    lowered = path_name.lower()
    for keyword in ("elite", "boss", "shop", "treasure", "rest", "camp", "monster"):
        if keyword in lowered:
            return keyword
    return None


def main() -> None:
    map_bp = unreal.load_asset("/Game/UI/WBP_FrontendMap")
    if map_bp is None:
        raise RuntimeError("WBP_FrontendMap 없음")

    tree = unreal.find_object(None, "/Game/UI/WBP_FrontendMap.WBP_FrontendMap:WidgetTree")
    if tree is None:
        # 이름이 WidgetTree_0 같은 변형일 수 있어 패키지 안을 뒤진다.
        for candidate_name in ("WidgetTree", "WidgetTree_0", "WidgetTree_1"):
            tree = unreal.find_object(
                None, f"/Game/UI/WBP_FrontendMap.WBP_FrontendMap:{candidate_name}"
            )
            if tree is not None:
                break
    if tree is None:
        raise RuntimeError("WidgetTree 서브오브젝트를 찾지 못함")

    root = tree.get_editor_property("root_widget")
    swapped = [0]

    def walk(widget):
        if widget is None:
            return
        if isinstance(widget, unreal.Image):
            brush = widget.get_editor_property("brush")
            resource = brush.get_editor_property("resource_object")
            if resource is not None:
                path_name = resource.get_path_name()
                unreal.log(f"RD_LEGEND image {widget.get_name()} = {path_name}")
                if "MapNode" in path_name and "_V2" not in path_name:
                    keyword = keyword_of(path_name)
                    if keyword is not None:
                        new_texture = unreal.load_asset(
                            f"{DESTINATION_PATH}/{KEYWORD_TO_ASSET[keyword]}"
                        )
                        if new_texture is not None:
                            brush.set_editor_property("resource_object", new_texture)
                            widget.set_editor_property("brush", brush)
                            swapped[0] += 1
                            unreal.log(
                                f"RD_LEGEND swapped {widget.get_name()} -> {KEYWORD_TO_ASSET[keyword]}"
                            )
        if isinstance(widget, unreal.PanelWidget):
            for index in range(widget.get_children_count()):
                walk(widget.get_child_at(index))
        elif isinstance(widget, unreal.ContentWidget):
            walk(widget.get_content())

    walk(root)

    if swapped[0] > 0:
        if not unreal.EditorAssetLibrary.save_asset(
            "/Game/UI/WBP_FrontendMap", only_if_is_dirty=False
        ):
            raise RuntimeError("WBP_FrontendMap 저장 실패")
    unreal.log(f"RD_LEGEND done swapped={swapped[0]}")


main()
