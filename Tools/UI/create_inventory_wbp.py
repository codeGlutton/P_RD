# @file create_inventory_wbp.py
# @brief 인벤토리(런 상태 확인) 화면 WBP를 코드로 생성하는 일회성 제작 스크립트.
# @date 2026-06-18
#
# 사용: UnrealEditor-Cmd 로 이 프로젝트를 열고 Python 실행.
#   UnrealEditor-Cmd.exe P_RD.uproject -ExecutePythonScript="Tools/UI/create_inventory_wbp.py"
#
# 기법(콤뱃 HUD 스크립트와 동일): 루트 캔버스가 있는 기존 WBP를 복제(duplicate_asset)한 뒤
# 부모 클래스를 C++ UInventoryViewWidgetBase 로 reparent 한다. 결과물 = WBP_Inventory.uasset.
# 상세 비주얼(슬롯 그리드 등)은 OnInventoryRefreshed 이벤트에서 WBP가 그린다.

import unreal

SOURCE_ASSET = "/Game/BP/UI/WBP_DicePanel"   # RootCanvas 를 가진 검증된 소스(레이아웃은 새로 얹음)
TARGET_PACKAGE = "/Game/BP/UI"
TARGET_NAME = "WBP_Inventory"
TARGET_ASSET = f"{TARGET_PACKAGE}/{TARGET_NAME}"
PARENT_CLASS_PATH = "/Script/P_RD.InventoryViewWidgetBase"


def anchors(min_x, min_y, max_x, max_y):
    return unreal.Anchors(minimum=unreal.Vector2D(min_x, min_y), maximum=unreal.Vector2D(max_x, max_y))


def find_widget(widget_name):
    asset_name = TARGET_ASSET.rsplit("/", 1)[-1]
    tree_path = f"{TARGET_ASSET}.{asset_name}:WidgetTree"
    unreal.EditorAssetLibrary.load_asset(TARGET_ASSET)
    for obj in unreal.ObjectIterator():
        try:
            path = obj.get_path_name()
            cls = obj.get_class().get_name()
        except Exception:
            continue
        if tree_path not in path or cls.endswith("Slot"):
            continue
        if obj.get_name() == widget_name:
            return obj
    return None


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


def find_root_canvas():
    """이름이 RootCanvas 인 게 있으면 그걸, 없으면 트리의 첫 CanvasPanel 을 루트로 본다."""
    canvas = find_widget("RootCanvas")
    if canvas is not None:
        return canvas
    asset_name = TARGET_ASSET.rsplit("/", 1)[-1]
    tree_path = f"{TARGET_ASSET}.{asset_name}:WidgetTree"
    for obj in unreal.ObjectIterator():
        try:
            if tree_path in obj.get_path_name() and obj.get_class().get_name() == "CanvasPanel":
                return obj
        except Exception:
            continue
    return None


def ensure_target_asset():
    if unreal.EditorAssetLibrary.does_asset_exist(TARGET_ASSET):
        unreal.EditorAssetLibrary.delete_asset(TARGET_ASSET)
    duplicated = unreal.EditorAssetLibrary.duplicate_asset(SOURCE_ASSET, TARGET_ASSET)
    if duplicated is None:
        raise RuntimeError(f"복제 실패: {SOURCE_ASSET} -> {TARGET_ASSET}")
    parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
    if parent_class is None:
        raise RuntimeError(f"부모 C++ 클래스를 찾을 수 없음: {PARENT_CLASS_PATH} (모듈 빌드 확인)")
    unreal.BlueprintEditorLibrary.reparent_blueprint(duplicated, parent_class)
    unreal.log(f"복제+리페어런트 완료: {TARGET_ASSET}")
    return duplicated


def add_label(tree, root, name, txt, a):
    w = tree.construct_widget(unreal.TextBlock, unreal.Name(name))
    w.set_text(unreal.Text.from_string(txt))
    root.modify()
    slot = root.add_child_to_canvas(w)
    slot.set_editor_property("anchors", a)
    slot.set_editor_property("offsets", unreal.Margin(0, 0, 0, 0))
    w.modify()


def apply_layout():
    tree = get_widget_tree()
    root = find_root_canvas()
    if tree is None or root is None:
        unreal.log_warning("RootCanvas 를 못 찾아 레이아웃은 건너뜀(에셋/부모는 생성됨)")
        return
    # 인벤토리 자리표시 라벨(실제 슬롯 그리드는 OnInventoryRefreshed 에서 WBP가 채움)
    add_label(tree, root, "InvTitleText", "INVENTORY", anchors(0.05, 0.04, 0.95, 0.12))
    add_label(tree, root, "InvMetaText", "Gold / Lv / HP", anchors(0.05, 0.12, 0.95, 0.18))
    add_label(tree, root, "InvDiceLabel", "DICE", anchors(0.05, 0.20, 0.95, 0.26))
    add_label(tree, root, "InvSkillLabel", "SKILL", anchors(0.05, 0.46, 0.95, 0.52))
    add_label(tree, root, "InvEquipLabel", "EQUIPMENT", anchors(0.05, 0.72, 0.95, 0.78))


asset = ensure_target_asset()
# 레이아웃 자리표시는 best-effort — API 차이로 실패해도 에셋(부모=C++ 베이스)은 반드시 저장한다.
# 실제 비주얼은 OnInventoryRefreshed 에서 WBP가 그리므로, 껍데기+올바른 부모만으로 충분하다.
try:
    apply_layout()
except Exception as e:
    unreal.log_warning(f"레이아웃 자리표시 건너뜀(에셋/부모는 생성됨): {e}")
unreal.BlueprintEditorLibrary.compile_blueprint(asset)
unreal.EditorAssetLibrary.save_asset(TARGET_ASSET, only_if_is_dirty=False)
unreal.log(f"Inventory WBP ready: {TARGET_ASSET}")
