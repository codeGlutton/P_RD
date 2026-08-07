"""몬스터 탭 닫기 단추·목록 줄 구조를 본다."""
from pathlib import Path

import unreal

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/monster_state.txt")
LINES = []
blueprint = unreal.EditorAssetLibrary.load_asset(
    "/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound")
tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")


def chain(widget):
    parts = []
    node = widget
    while node is not None:
        slot = node.get_editor_property("slot")
        box = ""
        if isinstance(slot, unreal.CanvasPanelSlot):
            offsets = slot.get_offsets()
            box = (f"[{offsets.left:.0f},{offsets.top:.0f},"
                   f"{offsets.right:.0f},{offsets.bottom:.0f} z{slot.get_z_order()}]")
        parts.append(f"{node.get_name()}({type(node).__name__}{box})")
        node = node.get_parent()
    return " <- ".join(parts)


for name in ("MonsterBackButton", "MonsterBackText", "MonsterBackText_Center",
             "MonsterBackArt", "MonsterRowName_0", "MonsterRowName_0_Center",
             "MonsterRowNormal_0", "MonsterRowSelected_0", "MonsterRowLevel_0",
             "MonsterSkillHeading", "MonsterStatusText_0"):
    widget = unreal.find_object(None, f"{tree.get_path_name()}.{name}")
    if widget is None:
        LINES.append(f"{name}: 없음")
        continue
    extra = ""
    if isinstance(widget, unreal.TextBlock):
        extra = f" text='{widget.get_editor_property('text')}'"
    LINES.append(f"{name}: vis={widget.get_editor_property('visibility')}{extra}")
    LINES.append(f"    {chain(widget)}")

root = unreal.find_object(None, f"{tree.get_path_name()}.MonsterTabCanvas")
if root is not None:
    LINES.append(f"MonsterTabCanvas 부모사슬: {chain(root)}")

RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")
