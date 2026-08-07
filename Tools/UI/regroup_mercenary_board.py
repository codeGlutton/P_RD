"""용병 판(MercenaryBoard)을 **구역별 하위 캔버스**로 재편한다.

왜
--
판 하나에 위젯 서른 개가 절대좌표로 깔려 있으면, 에디터에서 하나를 집으려다
겹친 열 개가 걸리고 구역을 옮기려면 낱개를 다 끌어야 한다. "하나에 다
뭉쳐있으니까 수정을 못해" -- 맞는 말이다.

구역 셋으로 나눈다. 구역을 집으면 안엣것이 통째로 따라온다.

    MercHeaderSection   골드 · 제목 · 뒤로
    MercRosterSection   왼쪽 용병 카드 3장
    MercDetailSection   오른쪽 이름 · 스탯 고리 · 스킬판

판 전체를 덮는 판때기들(BoardPlate 등 앵커 늘림)은 그대로 둔다 -- 구역에
넣으면 늘림 기준이 구역으로 바뀌어 자리가 틀어진다. 이름은 안 바꾸므로
C++ 배선(FindWidget)은 그대로 산다.

HUD 안의 탭과 따로 뗀 WBP_MercenaryPanel 둘 다 같은 이름 구조라 함께 손본다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/regroup_mercenary_board.py"
"""

import unreal
from pathlib import Path

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/regroup_board.txt")

ASSETS = [
    "/Game/UI/CombatLayouts/WBP_CombatHUD04",
    "/Game/UI/CombatLayouts/WBP_MercenaryPanel",
]

# (구역 이름, z, 자리(x,y,w,h), 들어갈 위젯들)
SECTIONS = [
    ("MercHeaderSection", 5, (40, 30, 1840, 130),
     ["MercenaryGoldLabel_Center", "MercenaryGoldText_Center",
      "MercenaryTitleText_Center", "MercenaryBackArtMount"]),
    ("MercRosterSection", 3, (18, 196, 432, 771),
     ["MercenaryCardScale_0", "MercenaryCardScale_1", "MercenaryCardScale_2"]),
    ("MercDetailSection", 8, (1090, 200, 740, 810),
     ["MercenaryDetailName_Center",
      "MercenaryChip0FrameMount", "MercenaryChip1FrameMount",
      "MercenaryChip2FrameMount", "MercenarySkillHeading_Center"]
     + [f"MercenarySkillFrame_{i}Mount" for i in range(6)]
     + [f"MercenarySkillIcon_{i}Mount" for i in range(6)]),
]

LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def find(tree, name):
    return unreal.find_object(None, f"{tree.get_path_name()}.{name}")


def canvas_box(widget):
    slot = widget.get_editor_property("slot") if widget else None
    if not isinstance(slot, unreal.CanvasPanelSlot):
        return None
    offsets = slot.get_offsets()
    align = slot.get_alignment()
    anchors = slot.get_anchors()
    if (anchors.minimum.x != anchors.maximum.x
            or anchors.minimum.y != anchors.maximum.y):
        return None          # 앵커 늘림 -- 건드리면 안 된다
    return (offsets.left - align.x * offsets.right,
            offsets.top - align.y * offsets.bottom,
            offsets.right, offsets.bottom, slot.get_z_order())


def regroup(asset):
    blueprint = unreal.EditorAssetLibrary.load_asset(asset)
    if blueprint is None:
        say(f"{asset}: 판 없음")
        return
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    board = find(tree, "MercenaryBoard")
    if not isinstance(board, unreal.CanvasPanel):
        say(f"{asset}: MercenaryBoard 없음 -- 건너뜀")
        return
    say(asset.rsplit("/", 1)[-1])

    for section_name, z, (sx, sy, sw, sh), members in SECTIONS:
        section = find(tree, section_name)
        if section is None:
            section = unreal.new_object(unreal.CanvasPanel, outer=tree,
                                        name=section_name)
        blueprint.modify()
        section.modify()
        if section.get_parent() is None:
            board.add_child(section)
        slot = section.get_editor_property("slot")
        if isinstance(slot, unreal.CanvasPanelSlot):
            slot.set_alignment(unreal.Vector2D(0.0, 0.0))
            slot.set_auto_size(False)
            slot.set_offsets(unreal.Margin(float(sx), float(sy),
                                           float(sw), float(sh)))
            slot.set_z_order(z)

        moved = 0
        for name in members:
            widget = find(tree, name)
            if widget is None or widget.get_parent() == section:
                continue
            box = canvas_box(widget)
            if box is None:
                say(f"    {name}: 점 앵커가 아님 -- 건너뜀")
                continue
            widget.modify()
            widget.get_parent().remove_child(widget)
            section.add_child(widget)
            child_slot = widget.get_editor_property("slot")
            if isinstance(child_slot, unreal.CanvasPanelSlot):
                child_slot.set_alignment(unreal.Vector2D(0.0, 0.0))
                child_slot.set_auto_size(False)
                child_slot.set_offsets(unreal.Margin(
                    box[0] - sx, box[1] - sy, box[2], box[3]))
                child_slot.set_z_order(box[4])
            moved += 1
        say(f"    {section_name}: {moved}개 들임 ({sx},{sy} {sw}x{sh})")

    unreal.SystemLibrary.execute_console_command(
        None, f"RD.Editor.CleanWidgetVariables {asset}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    say(f"    저장={unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)}")


def main():
    for asset in ASSETS:
        regroup(asset)
    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
