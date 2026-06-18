# @file create_inventory_wbp.py
# @brief 인벤토리(런 상태 확인) 화면 WBP를 코드로 생성/배치하는 일회성 제작 스크립트.
# @date 2026-06-18
#
# 사용: UnrealEditor-Cmd 로 이 프로젝트를 열고 Python 으로 실행한다.
#   UnrealEditor-Cmd.exe P_RD.uproject -run=pythonscript -script="Tools/UI/create_inventory_wbp.py"
# 또는 에디터 Output Log 의 Python 콘솔에서 exec(open(path).read()).
#
# 결과물은 WBP_Inventory.uasset 이며, 부모 클래스는 C++ UInventoryViewWidgetBase 다.
# 비주얼 디테일(슬롯 그리드 채우기 등)은 OnInventoryRefreshed 이벤트에서 WBP가 그린다 —
# 이 스크립트는 "유효한 WBP 껍데기 + 올바른 부모 + 기본 레이아웃 앵커"까지만 찍는다.

import unreal

TARGET_PACKAGE = "/Game/BP/UI"
TARGET_NAME = "WBP_Inventory"
TARGET_ASSET = f"{TARGET_PACKAGE}/{TARGET_NAME}"
PARENT_CLASS_PATH = "/Script/P_RD.InventoryViewWidgetBase"


def anchors(min_x, min_y, max_x, max_y):
    return unreal.Anchors(minimum=unreal.Vector2D(min_x, min_y), maximum=unreal.Vector2D(max_x, max_y))


def ensure_asset():
    """없으면 WidgetBlueprint 를 새로 만들고, C++ 베이스로 reparent 한다."""
    if not unreal.EditorAssetLibrary.does_asset_exist(TARGET_ASSET):
        factory = unreal.WidgetBlueprintFactory()
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        asset_tools.create_asset(TARGET_NAME, TARGET_PACKAGE, unreal.WidgetBlueprint, factory)
    asset = unreal.EditorAssetLibrary.load_asset(TARGET_ASSET)
    parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
    if parent_class is None:
        raise RuntimeError(f"부모 C++ 클래스를 찾을 수 없음: {PARENT_CLASS_PATH} (모듈 빌드 확인)")
    unreal.BlueprintEditorLibrary.reparent_blueprint(asset, parent_class)
    return asset


def get_widget_tree():
    asset_name = TARGET_ASSET.rsplit("/", 1)[-1]
    tree_path = f"{TARGET_ASSET}.{asset_name}:WidgetTree"
    unreal.EditorAssetLibrary.load_asset(TARGET_ASSET)
    for obj in unreal.ObjectIterator():
        try:
            if tree_path in obj.get_path_name() and obj.get_class().get_name() == "WidgetTree":
                return obj
        except Exception:
            continue
    return None


def build_layout():
    """루트 캔버스에 제목/메타/세 영역(다이스·스킬·장비) 자리표시 컨테이너를 올린다."""
    tree = get_widget_tree()
    if tree is None:
        raise RuntimeError("WidgetTree 를 찾지 못함")

    root = tree.get_editor_property("root_widget")
    if root is None or root.get_class().get_name() != "CanvasPanel":
        root = tree.construct_widget(unreal.CanvasPanel)
        tree.set_editor_property("root_widget", root)

    def add_text(name, txt, a):
        w = tree.construct_widget(unreal.TextBlock, unreal.Name(name))
        w.set_text(unreal.Text.from_string(txt))
        slot = root.add_child_to_canvas(w)
        slot.set_editor_property("anchors", a)
        slot.set_editor_property("offsets", unreal.Margin(0, 0, 0, 0))
        return w

    # 상단 메타 / 제목 + 세 영역 라벨(실제 슬롯은 OnInventoryRefreshed 에서 WBP가 채움)
    add_text("TitleText", "INVENTORY", anchors(0.05, 0.04, 0.95, 0.12))
    add_text("MetaText", "Gold / Lv / HP", anchors(0.05, 0.12, 0.95, 0.18))
    add_text("DiceSectionLabel", "DICE", anchors(0.05, 0.20, 0.95, 0.26))
    add_text("SkillSectionLabel", "SKILL", anchors(0.05, 0.46, 0.95, 0.52))
    add_text("EquipSectionLabel", "EQUIPMENT", anchors(0.05, 0.72, 0.95, 0.78))


asset = ensure_asset()
build_layout()
unreal.BlueprintEditorLibrary.compile_blueprint(asset)
unreal.EditorAssetLibrary.save_asset(TARGET_ASSET, only_if_is_dirty=False)
unreal.log(f"Inventory WBP ready: {TARGET_ASSET}")
