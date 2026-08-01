"""WBP_FrontendMap의 이동 전 하위 위젯 클래스 참조를 현재 경로로 교정한다."""

import unreal


MAP_ASSET = "/Game/UI/WBP_FrontendMap"
LINE_CLASS_PATH = "/Game/UI/WBP_FrontendMapLine.WBP_FrontendMapLine_C"
NODE_CLASS_PATH = "/Game/UI/WBP_FrontendMapNode.WBP_FrontendMapNode_C"


def load_class(path):
    loaded = unreal.load_object(None, path)
    if loaded is None:
        raise RuntimeError("클래스를 찾을 수 없습니다: " + path)
    return loaded


map_class = unreal.EditorAssetLibrary.load_blueprint_class(MAP_ASSET)
if map_class is None:
    raise RuntimeError("맵 Blueprint 클래스를 찾을 수 없습니다: " + MAP_ASSET)

map_cdo = unreal.get_default_object(map_class)
line_class = load_class(LINE_CLASS_PATH)
node_class = load_class(NODE_CLASS_PATH)

map_cdo.set_editor_property("map_line_widget_class", line_class)
map_cdo.set_editor_property("map_node_widget_class", node_class)

if not unreal.EditorAssetLibrary.save_asset(MAP_ASSET, only_if_is_dirty=False):
    raise RuntimeError("맵 Blueprint 저장에 실패했습니다: " + MAP_ASSET)

saved_line = map_cdo.get_editor_property("map_line_widget_class")
saved_node = map_cdo.get_editor_property("map_node_widget_class")
if saved_line != line_class or saved_node != node_class:
    raise RuntimeError("저장 후 하위 위젯 클래스 참조 검증에 실패했습니다.")

unreal.log("[FrontendMap] 하위 위젯 클래스 참조를 현재 /Game/UI 경로로 교정했습니다.")
